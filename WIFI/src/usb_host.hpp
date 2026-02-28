#pragma once
#include "usb/usb_host.h"

class USBhost
{
    friend void _client_event_callback(const usb_host_client_event_msg_t *event_msg, void *arg);

protected:
    usb_device_info_t dev_info;
    usb_device_handle_t dev_hdl;
    usb_host_client_event_cb_t _client_event_cb = nullptr;

public:
    USBhost();
    ~USBhost();

    usb_host_client_handle_t client_hdl;

    bool init(bool create_tasks = true);
    bool open(const usb_host_client_event_msg_t *event_msg);
    void close();

    usb_device_info_t getDeviceInfo();
    const usb_device_desc_t *getDeviceDescriptor();
    const usb_config_desc_t *getConfigurationDescriptor();

    usb_host_client_handle_t clientHandle();
    usb_device_handle_t deviceHandle();

    void registerClientCb(usb_host_client_event_cb_t cb) { _client_event_cb = cb; }
};
