#include "wifi_connect.h"
#include "uart_serial.h"
#include <WiFi.h>

const char* SSID = "Device-Northwestern";   // <- set this
const char* PASS = "";                      // <- set if required; otherwise ""

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

  Serial0.printf("Connecting to SSID: %s\n", SSID);

  // If the network is open, PASS can be ""
  WiFi.begin(SSID, PASS);

  // Wait up to ~15 seconds
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) {
    Serial0.print(".");
    delay(500);
  }
  Serial0.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial0.println("WiFi: CONNECTED");
    Serial0.print("IP: ");   Serial0.println(WiFi.localIP());
    Serial0.print("RSSI: "); Serial0.println(WiFi.RSSI());
  } else {
    Serial0.print("WiFi: NOT connected, status=");
    Serial0.println(WiFi.status());
  }
}
