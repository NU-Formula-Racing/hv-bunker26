#include "esp_log.h"
#include "string.h"
#include "usb/usb_helpers.h"
#include "usb_acm.h"

static const char *TAG = "USBacm";

void IRAM_ATTR usb_ctrl_cb(usb_transfer_t *transfer)
{
    USBacmDevice *dev = (USBacmDevice *)transfer->context;
    if (transfer->data_buffer[0] == SET_VALUE && transfer->data_buffer[1] == SET_LINE_CODING) {
        dev->_callback(CDC_CTRL_SET_LINE_CODING, transfer);
    } else if (transfer->data_buffer[0] == SET_VALUE && transfer->data_buffer[1] == SET_CONTROL_LINE_STATE) {
        dev->_callback(CDC_CTRL_SET_CONTROL_LINE_STATE, transfer);
    }
}

void IRAM_ATTR usb_read_cb(usb_transfer_t *transfer)
{
    USBacmDevice *dev = (USBacmDevice *)transfer->context;
    dev->_callback(CDC_DATA_IN, transfer);
    dev->deallocate(transfer);
}

void IRAM_ATTR usb_write_cb(usb_transfer_t *transfer)
{
    USBacmDevice *dev = (USBacmDevice *)transfer->context;
    dev->_callback(CDC_DATA_OUT, transfer);
    dev->deallocate(transfer);
}

USBacmDevice::USBacmDevice(const usb_config_desc_t *config_desc, USBhost *host)
{
    _host = host;
    ep_int = nullptr;
    ep_in = nullptr;
    ep_out = nullptr;
    connected = false;

    int offset = 0;
    for (size_t n = 0; n < config_desc->bNumInterfaces; n++) {
        const usb_intf_desc_t *intf = usb_parse_interface_descriptor(config_desc, n, 0, &offset);
        const usb_ep_desc_t *ep = nullptr;

        if (intf->bInterfaceClass == 0x02) {
            if (intf->bNumEndpoints != 1)
                continue;
            int _offset = 0;
            ep = usb_parse_endpoint_descriptor_by_index(intf, 0, config_desc->wTotalLength, &_offset);
            ep_int = ep;
            ESP_LOGI(TAG, "CDC comm interface %d", n);
            if (ep)
                ESP_LOGI(TAG, "  EP addr: 0x%02x, max_pkt: %d, dir: %s",
                         ep->bEndpointAddress, ep->wMaxPacketSize,
                         (ep->bEndpointAddress & 0x80) ? "IN" : "OUT");

            esp_err_t err = usb_host_interface_claim(_host->clientHandle(), _host->deviceHandle(), n, 0);
            ESP_LOGI(TAG, "  interface claim: %s", esp_err_to_name(err));
            itf_num = 0;
        } else if (intf->bInterfaceClass == 0x0a) {
            if (intf->bNumEndpoints != 2)
                continue;
            ESP_LOGI(TAG, "CDC data interface %d", n);
            for (size_t i = 0; i < intf->bNumEndpoints; i++) {
                int _offset = 0;
                ep = usb_parse_endpoint_descriptor_by_index(intf, i, config_desc->wTotalLength, &_offset);
                if (ep->bEndpointAddress & 0x80) {
                    ep_in = ep;
                } else {
                    ep_out = ep;
                }
                if (ep)
                    ESP_LOGI(TAG, "  EP addr: 0x%02x, max_pkt: %d, dir: %s",
                             ep->bEndpointAddress, ep->wMaxPacketSize,
                             (ep->bEndpointAddress & 0x80) ? "IN" : "OUT");
            }
            esp_err_t err = usb_host_interface_claim(_host->clientHandle(), _host->deviceHandle(), n, 0);
            ESP_LOGI(TAG, "  interface claim: %s", esp_err_to_name(err));
        }
    }
}

USBacmDevice::~USBacmDevice()
{
    deallocate(xfer_ctrl);
}

bool USBacmDevice::init()
{
    usb_device_info_t info = _host->getDeviceInfo();
    USBhostDevice::init(info.bMaxPacketSize0);
    xfer_ctrl->callback = usb_ctrl_cb;
    return true;
}

void USBacmDevice::setControlLine(bool dtr, bool rts)
{
    USB_CTRL_REQ_CDC_SET_CONTROL_LINE_STATE((usb_setup_packet_t *)xfer_ctrl->data_buffer, 0, dtr, rts);
    xfer_ctrl->num_bytes = sizeof(usb_setup_packet_t) + ((usb_setup_packet_t *)xfer_ctrl->data_buffer)->wLength;
    esp_err_t err = usb_host_transfer_submit_control(_host->clientHandle(), xfer_ctrl);
    if (err) ESP_LOGW(TAG, "setControlLine: 0x%02x", err);
}

void USBacmDevice::setLineCoding(uint32_t bitrate, uint8_t cf, uint8_t parity, uint8_t bits)
{
    line_coding_t data;
    data.dwDTERate = bitrate;
    data.bCharFormat = cf;
    data.bParityType = parity;
    data.bDataBits = bits;
    USB_CTRL_REQ_CDC_SET_LINE_CODING((usb_setup_packet_t *)xfer_ctrl->data_buffer, 0, bitrate, cf, parity, bits);
    memcpy(xfer_ctrl->data_buffer + sizeof(usb_setup_packet_t), &data, sizeof(line_coding_t));
    xfer_ctrl->num_bytes = sizeof(usb_setup_packet_t) + 7;
    esp_err_t err = usb_host_transfer_submit_control(_host->clientHandle(), xfer_ctrl);
    if (err) ESP_LOGW(TAG, "setLineCoding: 0x%02x", err);
}

void USBacmDevice::INDATA(size_t len)
{
    if (!connected)
        return;

    size_t _len = usb_round_up_to_mps(len, ep_out->wMaxPacketSize);

    usb_transfer_t *xfer_read = allocate(_len);
    xfer_read->num_bytes = _len;
    xfer_read->callback = usb_read_cb;
    xfer_read->context = this;
    xfer_read->bEndpointAddress = ep_in->bEndpointAddress;

    esp_err_t err = usb_host_transfer_submit(xfer_read);
    if (err) {
        ESP_LOGW(TAG, "INDATA submit: 0x%02x", err);
    }
}

void USBacmDevice::OUTDATA(uint8_t *data, size_t len)
{
    if (!connected)
        return;
    if (!len)
        return;

    usb_transfer_t *xfer_write = allocate(64);
    xfer_write->callback = usb_write_cb;
    xfer_write->context = this;
    xfer_write->bEndpointAddress = ep_out->bEndpointAddress;
    xfer_write->num_bytes = len;
    memcpy(xfer_write->data_buffer, data, len);

    esp_err_t err = usb_host_transfer_submit(xfer_write);
    if (err) {
        ESP_LOGW(TAG, "OUTDATA submit: 0x%02x", err);
    }
}

bool USBacmDevice::isConnected()
{
    return connected;
}

void USBacmDevice::_callback(int event, usb_transfer_t *transfer)
{
    switch (event) {
    case CDC_DATA_IN:
    case CDC_DATA_OUT:
        break;
    default:
        connected = true;
        break;
    }
    if (event_cb)
        event_cb(event, transfer->data_buffer, transfer->actual_num_bytes);
}
