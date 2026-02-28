#include <WiFi.h>
#include "wifi_connect.h"
#include "google_sheets.h"
#include "usb_host.hpp"
#include "usb_acm.hpp"
#include "usb/usb_helpers.h"
#include "esp_log.h"
#include "uart_serial.h"

HardwareSerial Serial0(0);

USBhost host;
USBacmDevice *device = nullptr;
int counter = 0;

static void cdc_callback(int event, void *data, size_t len)
{
    switch (event) {
    case CDC_CTRL_SET_CONTROL_LINE_STATE:
        Serial0.println("CDC: control line state set");
        if (device) device->setLineCoding(115200, 0, 0, 8);
        break;
    case CDC_DATA_IN:
        if (device) {
            device->INDATA();
            if (data && len) Serial0.write((const uint8_t *)data, len);
        }
        break;
    case CDC_DATA_OUT:
        break;
    case CDC_CTRL_SET_LINE_CODING:
        Serial0.println("CDC: line coding set — starting bulk reads");
        if (device) device->INDATA();
        break;
    }
}

static void client_event_callback(const usb_host_client_event_msg_t *event_msg, void *arg)
{
    if (event_msg->event == USB_HOST_CLIENT_EVENT_NEW_DEV) {
        usb_device_info_t info = host.getDeviceInfo();
        Serial0.printf("USB device connected: speed=%s addr=%d maxpkt=%d config=%d\n",
                        info.speed ? "FULL" : "LOW", info.dev_addr,
                        info.bMaxPacketSize0, info.bConfigurationValue);

        const usb_device_desc_t *dev_desc = host.getDeviceDescriptor();
        const usb_config_desc_t *config_desc = host.getConfigurationDescriptor();
        if (!dev_desc || !config_desc) return;

        int offset = 0;
        for (size_t i = 0; i < dev_desc->bNumConfigurations; i++) {
            for (size_t n = 0; n < config_desc->bNumInterfaces; n++) {
                const usb_intf_desc_t *intf = usb_parse_interface_descriptor(config_desc, n, 0, &offset);
                if (!intf) continue;
                Serial0.printf("  interface %d: class=0x%02x\n", n, intf->bInterfaceClass);
                if (intf->bInterfaceClass == 0x0a) {
                    device = new USBacmDevice(config_desc, &host);
                    n = config_desc->bNumInterfaces;
                    if (device) {
                        device->init();
                        device->onEvent(cdc_callback);
                        device->setControlLine(1, 1);
                    }
                }
            }
        }
    } else {
        Serial0.println("USB device disconnected");
        if (device) {
            device->deinit();
            delete device;
        }
        device = nullptr;
    }
}

void setup()
{
    Serial0.begin(115200);
    delay(500);

    Serial0.print("STA MAC: ");
    Serial0.println(WiFi.macAddress());

    host.registerClientCb(client_event_callback);
    host.init();
    Serial0.println("USB host initialized.");

    connectWiFi();
}

void loop()
{
    while (counter < 10) {
        String payload = makePayload();
        bool ok = postToGoogleSheets(payload);
        Serial0.print("POST success? ");
        Serial0.println(ok ? "YES" : "NO");
        counter++;
    }

    static unsigned long last = 0;
    if (millis() - last >= 5000) {
        last = millis();
        if (device && device->isConnected()) {
            const usb_device_desc_t *desc = host.getDeviceDescriptor();
            usb_device_info_t info = host.getDeviceInfo();
            Serial0.printf("[idle] USB device: connected  VID=0x%04X PID=0x%04X  speed=%s addr=%d\n",
                           desc ? desc->idVendor : 0, desc ? desc->idProduct : 0,
                           info.speed ? "FULL" : "LOW", info.dev_addr);
        } else {
            Serial0.println("[idle] USB device: waiting");
        }
    }

}
