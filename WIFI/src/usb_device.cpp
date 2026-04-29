#include "esp_log.h"
#include "usb_device.h"

USBhostDevice::USBhostDevice() {}

USBhostDevice::~USBhostDevice() {}

esp_err_t USBhostDevice::init(size_t len)
{
    esp_err_t err = usb_host_transfer_alloc(len, 0, &xfer_ctrl);
    xfer_ctrl->device_handle = _host->deviceHandle();
    xfer_ctrl->context = this;
    xfer_ctrl->bEndpointAddress = 0;
    return err;
}

IRAM_ATTR usb_transfer_t *USBhostDevice::allocate(size_t _size)
{
    usb_transfer_t *transfer = NULL;
    esp_err_t err = usb_host_transfer_alloc(_size, 0, &transfer);
    if (!err) {
        transfer->device_handle = _host->deviceHandle();
        transfer->context = this;
    }
    return transfer;
}

IRAM_ATTR esp_err_t USBhostDevice::deallocate(usb_transfer_t *transfer)
{
    esp_err_t err = usb_host_transfer_free(transfer);
    if (ESP_OK != err) {
        ESP_LOGE("", "deallocate free transfer : %d", err);
    }
    return err;
}

void USBhostDevice::onEvent(usb_host_event_cb_t _cb)
{
    event_cb = _cb;
}

bool USBhostDevice::deinit()
{
    if (!config_desc || !_host)
        return true;

    usb_host_client_handle_t client = _host->clientHandle();
    usb_device_handle_t dev = _host->deviceHandle();
    // Must release claimed interfaces before the host closes the device (after this callback).
    // Skipping release when dev was non-null caused the next attach to fail ("device won't turn on").
    if (!client || !dev)
        return true;

    for (size_t n = 0; n < config_desc->bNumInterfaces; n++) {
        esp_err_t err = usb_host_interface_release(client, dev, (uint8_t)n);
        if (err != ESP_OK && err != ESP_ERR_NOT_FOUND)
            ESP_LOGW("USBhostDevice", "interface_release %u: %s", (unsigned)n, esp_err_to_name(err));
    }
    return true;
}
