#include "usb_host.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "USBhost";

USBhost::USBhost() {}

USBhost::~USBhost() {
  init_done_ = false;
}

void USBhost::registerClientCb(client_event_cb_t cb, void *arg) {
  user_cb_ = cb;
  user_arg_ = arg;
}

void USBhost::client_event_cb(const usb_host_client_event_msg_t *event_msg, void *arg) {
  USBhost *host = (USBhost *)arg;
  if (event_msg->event == USB_HOST_CLIENT_EVENT_NEW_DEV) {
    host->handleNewDev(event_msg);
  } else if (event_msg->event == USB_HOST_CLIENT_EVENT_DEV_GONE) {
    host->handleDevGone(event_msg);
  }
}

void USBhost::handleNewDev(const usb_host_client_event_msg_t *event_msg) {
  uint8_t addr = event_msg->new_dev.address;
  dev_addr_ = addr;

  esp_err_t err = usb_host_device_open(client_hdl_, addr, &dev_hdl_);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "usb_host_device_open failed: %s", esp_err_to_name(err));
    return;
  }

  err = usb_host_device_info(dev_hdl_, &dev_info_);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "usb_host_device_info failed: %s", esp_err_to_name(err));
    usb_host_device_close(client_hdl_, dev_hdl_);
    dev_hdl_ = nullptr;
    return;
  }

  err = usb_host_get_device_descriptor(dev_hdl_, &dev_desc_);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "usb_host_get_device_descriptor failed: %s", esp_err_to_name(err));
    usb_host_device_close(client_hdl_, dev_hdl_);
    dev_hdl_ = nullptr;
    return;
  }

  err = usb_host_get_active_config_descriptor(dev_hdl_, &config_desc_);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "usb_host_get_active_config_descriptor failed: %s", esp_err_to_name(err));
    usb_host_device_close(client_hdl_, dev_hdl_);
    dev_hdl_ = nullptr;
    return;
  }

  num_connected_++;
  if (user_cb_) {
    user_cb_(event_msg, user_arg_);
  }
}

void USBhost::handleDevGone(const usb_host_client_event_msg_t *event_msg) {
  if (user_cb_) {
    user_cb_(event_msg, user_arg_);
  }
  if (dev_hdl_) {
    usb_host_device_close(client_hdl_, dev_hdl_);
    dev_hdl_ = nullptr;
  }
  dev_desc_ = nullptr;
  config_desc_ = nullptr;
  dev_addr_ = 0;
  if (num_connected_ > 0) num_connected_--;
}

usb_device_info_t USBhost::getDeviceInfo() const {
  return dev_info_;
}

const usb_device_desc_t *USBhost::getDeviceDescriptor() const {
  return dev_desc_;
}

const usb_config_desc_t *USBhost::getConfigurationDescriptor() const {
  return config_desc_;
}

bool USBhost::hasDevice() const {
  return num_connected_ > 0;
}

int USBhost::getConnectedCount() const {
  return num_connected_;
}

void USBhost::syncCountFromLib() {
  usb_host_lib_info_t lib_info;
  if (usb_host_lib_info(&lib_info) == ESP_OK) {
    num_connected_ = lib_info.num_devices;
  }
}

static void usb_lib_task(void *arg) {
  (void)arg;
  const TickType_t timeout_ticks = pdMS_TO_TICKS(500);
  while (1) {
    uint32_t event_flags = 0;
    esp_err_t err = usb_host_lib_handle_events(timeout_ticks, &event_flags);
    if (err != ESP_OK && err != (esp_err_t)ESP_ERR_TIMEOUT) {
      ESP_LOGE(TAG, "usb_host_lib_handle_events: %s", esp_err_to_name(err));
    }
  }
}

void USBhost::client_task_entry(void *arg) {
  USBhost *host = (USBhost *)arg;
  host->client_task_impl();
}

void USBhost::client_task_impl() {
  usb_host_client_config_t client_config = {};
  client_config.is_synchronous = false;
  client_config.max_num_event_msg = 5;
  client_config.async.client_event_callback = client_event_cb;
  client_config.async.callback_arg = this;

  esp_err_t err = usb_host_client_register(&client_config, &client_hdl_);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "usb_host_client_register failed: %s", esp_err_to_name(err));
    return;
  }

  vTaskDelay(pdMS_TO_TICKS(200));
  syncCountFromLib();
  if (getConnectedCount() > 0) {
    ESP_LOGI(TAG, "already connected at init: %d device(s)", getConnectedCount());
  }

  ESP_LOGI(TAG, "waiting for device connect...");
  while (1) {
    err = usb_host_client_handle_events(client_hdl_, portMAX_DELAY);
    if (err != ESP_OK && err != (esp_err_t)ESP_ERR_TIMEOUT) {
      ESP_LOGE(TAG, "usb_host_client_handle_events: %s", esp_err_to_name(err));
    }
    syncCountFromLib();
  }
}

bool USBhost::init() {
  if (init_done_) return true;

  ESP_LOGI(TAG, "USB host init");
  usb_host_config_t host_config = {};
  host_config.skip_phy_setup = false;
  host_config.intr_flags = ESP_INTR_FLAG_LEVEL1;

  esp_err_t err = usb_host_install(&host_config);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "usb_host_install failed: %s", esp_err_to_name(err));
    return false;
  }

  xTaskCreatePinnedToCore(usb_lib_task, "usb_lib", 4096, nullptr, 20, nullptr, 0);
  xTaskCreatePinnedToCore(client_task_entry, "usb_client", 4096, this, 21, nullptr, 0);

  init_done_ = true;
  return true;
}
