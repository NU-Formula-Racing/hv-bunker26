#include <WiFi.h>
#include "wifi_connect.h"
#include "google_sheets.h"
#include "usb_host.hpp"
#include "usb_acm.hpp"
#include "usb/usb_helpers.h"
#include "esp_log.h"

USBhost host;
USBacmDevice *device = nullptr;
int counter = 0;

static void cdc_callback(int event, void *data, size_t len) {
  switch (event) {
    case CDC_CTRL_SET_CONTROL_LINE_STATE:
      if (device) device->setLineCoding(115200, 0, 0, 8);
      break;
    case CDC_DATA_IN:
      if (device) {
        device->INDATA();
        if (data && len) Serial.write((const uint8_t *)data, len);
      }
      break;
    case CDC_DATA_OUT:
      break;
    case CDC_CTRL_SET_LINE_CODING:
      break;
  }
}

static void client_event_callback(const usb_host_client_event_msg_t *event_msg, void *arg) {
  (void)arg;
  if (event_msg->event == USB_HOST_CLIENT_EVENT_NEW_DEV) {
    usb_device_info_t info = host.getDeviceInfo();
    ESP_LOGI("main", "device speed: %s, addr: %d, bMaxPacketSize0: %d, config: %d",
             info.speed ? "FULL" : "LOW", info.dev_addr, info.bMaxPacketSize0, info.bConfigurationValue);

    const usb_device_desc_t *dev_desc = host.getDeviceDescriptor();
    const usb_config_desc_t *config_desc = host.getConfigurationDescriptor();
    if (!dev_desc || !config_desc) return;

    int offset = 0;
    for (size_t i = 0; i < dev_desc->bNumConfigurations; i++) {
      for (size_t n = 0; n < config_desc->bNumInterfaces; n++) {
        const usb_intf_desc_t *intf = usb_parse_interface_descriptor(config_desc, (uint8_t)n, 0, &offset);
        if (!intf) continue;
        if (intf->bInterfaceClass == 0x0a) {  // CDC ACM
          device = new USBacmDevice(config_desc, &host);
          if (device) {
            device->init();
            device->onEvent(cdc_callback);
            device->setControlLine(1, 1);
            device->INDATA();
          }
          return;
        }
      }
    }
  } else {
    ESP_LOGW("main", "DEVICE gone");
    if (device) {
      device->deinit();
      delete device;
    }
    device = nullptr;
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.print("STA MAC: ");
  Serial.println(WiFi.macAddress());

  host.registerClientCb(client_event_callback, nullptr);
  bool usb_ok = host.init();
  Serial.println(usb_ok ? "USB host is set up successfully." : "USB host setup failed.");

  connectWiFi();
}

void loop() {
  while (counter < 10) {
    String payload = makePayload();
    bool ok = postToGoogleSheets(payload);
    Serial.print("POST success? ");
    Serial.println(ok ? "YES" : "NO");
    counter++;
  }

  static unsigned long last = 0;
  if (millis() - last >= 5000) {
    last = millis();
    Serial.print("[idle] USB host: ");
    Serial.println(host.hasDevice() ? "has device" : "no device");
    Serial.print("[idle] USB connected count: ");
    Serial.println(host.getConnectedCount());
  }

  if (device && device->isConnected()) {
    device->OUTDATA((const uint8_t *)"test\n", 5);
    delay(1000);
  }
}
