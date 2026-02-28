#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "usb/usb_host.h"

#include "usb_host.hpp"

static const char *TAG = "USBhost";

void _client_event_callback(const usb_host_client_event_msg_t *event_msg, void *arg)
{
    USBhost *host = (USBhost *)arg;
    if (event_msg->event == USB_HOST_CLIENT_EVENT_NEW_DEV) {
        host->open(event_msg);
        ESP_LOGI(TAG, "NEW_DEV: address %d", event_msg->new_dev.address);
        if (host->_client_event_cb) {
            host->_client_event_cb(event_msg, arg);
        }
    } else {
        ESP_LOGI(TAG, "DEV_GONE");
        if (host->_client_event_cb) {
            host->_client_event_cb(event_msg, arg);
        }
        host->close();
    }
}

static void client_async_seq_task(void *param)
{
    USBhost *host = (USBhost *)param;
    ESP_LOGI(TAG, "async task started");
    while (1) {
        usb_host_client_handle_t client_hdl = host->client_hdl;
        uint32_t event_flags;
        if (client_hdl)
            usb_host_client_handle_events(client_hdl, 1);
        if (ESP_OK == usb_host_lib_handle_events(1, &event_flags)) {
            if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
                ESP_LOGD(TAG, "No more clients");
                do {
                    if (usb_host_device_free_all() != ESP_ERR_NOT_FINISHED)
                        break;
                } while (1);
                usb_host_uninstall();
                host->init(false);
            }
            if (event_flags & USB_HOST_LIB_EVENT_FLAGS_ALL_FREE) {
                ESP_LOGD(TAG, "All free");
                usb_host_client_deregister(client_hdl);
                host->client_hdl = NULL;
            }
        }
    }
    vTaskDelete(NULL);
}

USBhost::USBhost() {}

USBhost::~USBhost() {}

bool USBhost::init(bool create_tasks)
{
    usb_host_config_t config = {};
    config.intr_flags = ESP_INTR_FLAG_LEVEL1;

    esp_err_t err = usb_host_install(&config);
    ESP_LOGI(TAG, "usb_host_install: %s", esp_err_to_name(err));

    usb_host_client_config_t client_config = {};
    client_config.is_synchronous = false;
    client_config.max_num_event_msg = 15;
    client_config.async.client_event_callback = _client_event_callback;
    client_config.async.callback_arg = this;

    err = usb_host_client_register(&client_config, &client_hdl);
    ESP_LOGI(TAG, "usb_host_client_register: %s", esp_err_to_name(err));

    if (create_tasks) {
        xTaskCreate(client_async_seq_task, "usb_async", 6 * 512, this, 20, NULL);
    }

    return true;
}

bool USBhost::open(const usb_host_client_event_msg_t *event_msg)
{
    esp_err_t err = usb_host_device_open(client_hdl, event_msg->new_dev.address, &dev_hdl);
    ESP_LOGI(TAG, "device_open: %s", esp_err_to_name(err));

    const usb_device_desc_t *device_desc;
    usb_host_get_device_descriptor(dev_hdl, &device_desc);
    const usb_config_desc_t *config_desc;
    usb_host_get_active_config_descriptor(dev_hdl, &config_desc);

    return true;
}

void USBhost::close()
{
    usb_host_device_close(client_hdl, dev_hdl);
}

usb_device_info_t USBhost::getDeviceInfo()
{
    usb_host_device_info(dev_hdl, &dev_info);
    return dev_info;
}

const usb_device_desc_t *USBhost::getDeviceDescriptor()
{
    const usb_device_desc_t *device_desc;
    usb_host_get_device_descriptor(dev_hdl, &device_desc);
    return device_desc;
}

const usb_config_desc_t *USBhost::getConfigurationDescriptor()
{
    const usb_config_desc_t *config_desc;
    usb_host_get_active_config_descriptor(dev_hdl, &config_desc);
    return config_desc;
}

usb_host_client_handle_t USBhost::clientHandle()
{
    return client_hdl;
}

usb_device_handle_t USBhost::deviceHandle()
{
    return dev_hdl;
}
