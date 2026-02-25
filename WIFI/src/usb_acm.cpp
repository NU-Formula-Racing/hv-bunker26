#include "usb_acm.hpp"
#include "esp_log.h"

static const char *TAG = "USBacm";

USBacmDevice::USBacmDevice(const usb_config_desc_t *config_desc, USBhost *host)
  : config_desc_(config_desc), host_(host) {}

USBacmDevice::~USBacmDevice() {
  deinit();
}

bool USBacmDevice::init() {
  if (!host_ || !config_desc_) return false;
  inited_ = true;
  connected_ = true;
  ESP_LOGI(TAG, "CDC ACM device init (stub)");
  return true;
}

void USBacmDevice::deinit() {
  inited_ = false;
  connected_ = false;
  ESP_LOGI(TAG, "CDC ACM device deinit");
}

void USBacmDevice::setControlLine(uint8_t dtr, uint8_t rts) {
  (void)dtr;
  (void)rts;
  if (event_cb_) {
    event_cb_(CDC_CTRL_SET_CONTROL_LINE_STATE, nullptr, 0);
  }
}

void USBacmDevice::setLineCoding(uint32_t baud, uint8_t stop, uint8_t parity, uint8_t data_bits) {
  (void)baud;
  (void)stop;
  (void)parity;
  (void)data_bits;
  if (event_cb_) {
    event_cb_(CDC_CTRL_SET_LINE_CODING, nullptr, 0);
  }
}

void USBacmDevice::INDATA() {
  if (event_cb_) {
    event_cb_(CDC_DATA_IN, nullptr, 0);
  }
}

void USBacmDevice::OUTDATA(const uint8_t *data, size_t len) {
  (void)data;
  (void)len;
  /* TODO: use cdc_acm_host or raw bulk OUT transfer */
  if (event_cb_) {
    event_cb_(CDC_DATA_OUT, nullptr, 0);
  }
}
