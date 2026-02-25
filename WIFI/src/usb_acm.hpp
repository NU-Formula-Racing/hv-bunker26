#ifndef USB_ACM_HPP
#define USB_ACM_HPP

#include "usb/usb_host.h"
#include "usb_host.hpp"
#include <stddef.h>
#include <stdint.h>

/** CDC ACM event codes (match template callback). */
#define CDC_CTRL_SET_CONTROL_LINE_STATE  1
#define CDC_DATA_IN                      2
#define CDC_DATA_OUT                     3
#define CDC_CTRL_SET_LINE_CODING         4

typedef void (*usb_acm_event_cb_t)(int event, void *data, size_t len);

/**
 * Minimal CDC-ACM device wrapper.
 * Matches template interface; implement INDATA/OUTDATA via ESP-IDF CDC host driver if needed.
 */
class USBacmDevice {
public:
  USBacmDevice(const usb_config_desc_t *config_desc, USBhost *host);
  ~USBacmDevice();

  bool init();
  void deinit();
  void onEvent(usb_acm_event_cb_t cb) { event_cb_ = cb; }
  void setControlLine(uint8_t dtr, uint8_t rts);
  void setLineCoding(uint32_t baud, uint8_t stop, uint8_t parity, uint8_t data_bits);
  void INDATA();   /* start/continue reading from device */
  void OUTDATA(const uint8_t *data, size_t len);
  bool isConnected() const { return connected_; }

private:
  const usb_config_desc_t *config_desc_;
  USBhost *host_;
  usb_acm_event_cb_t event_cb_ = nullptr;
  bool connected_ = false;
  bool inited_ = false;
};

#endif
