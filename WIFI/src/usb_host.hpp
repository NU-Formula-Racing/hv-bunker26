#ifndef USB_HOST_HPP
#define USB_HOST_HPP

#include "usb/usb_host.h"
#include <stddef.h>
#include <stdint.h>

/**
 * Wrapper for ESP-IDF USB Host Library.
 * Register a client callback, call init(), then the callback runs on device connect/disconnect.
 */
class USBhost {
public:
  typedef void (*client_event_cb_t)(const usb_host_client_event_msg_t *event_msg, void *arg);

  USBhost();
  ~USBhost();

  /** Register callback for NEW_DEV / DEV_GONE. Call before init(). */
  void registerClientCb(client_event_cb_t cb, void *arg = nullptr);

  /** Install USB host and start daemon + client tasks. */
  bool init();

  /** Call from NEW_DEV callback: device must be open (we open it before invoking your callback). */
  usb_device_info_t getDeviceInfo() const;
  const usb_device_desc_t *getDeviceDescriptor() const;
  const usb_config_desc_t *getConfigurationDescriptor() const;

  /** Polling: any device connected (including already at boot). */
  bool hasDevice() const;
  int getConnectedCount() const;

  /** Called from client task to sync count from library. */
  void syncCountFromLib();

  /** Handles for internal use by USBacmDevice. */
  usb_host_client_handle_t getClientHandle() const { return client_hdl_; }
  usb_device_handle_t getDeviceHandle() const { return dev_hdl_; }
  uint8_t getDeviceAddress() const { return dev_addr_; }

  /** Entry for client task (static). */
  static void client_task_entry(void *arg);

private:
  client_event_cb_t user_cb_ = nullptr;
  void *user_arg_ = nullptr;
  usb_host_client_handle_t client_hdl_ = nullptr;
  usb_device_handle_t dev_hdl_ = nullptr;
  uint8_t dev_addr_ = 0;
  usb_device_info_t dev_info_ = {};
  const usb_device_desc_t *dev_desc_ = nullptr;
  const usb_config_desc_t *config_desc_ = nullptr;
  volatile int num_connected_ = 0;
  bool init_done_ = false;

  static void client_event_cb(const usb_host_client_event_msg_t *event_msg, void *arg);
  void handleNewDev(const usb_host_client_event_msg_t *event_msg);
  void handleDevGone(const usb_host_client_event_msg_t *event_msg);
  void client_task_impl();
};

#endif
