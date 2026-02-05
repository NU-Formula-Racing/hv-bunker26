#include <WiFi.h>

const char* SSID = "Device-Northwestern";   // <- set this
const char* PASS = "";                      // <- set if required; otherwise ""

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

  Serial.printf("Connecting to SSID: %s\n", SSID);

  // If the network is open, PASS can be ""
  WiFi.begin(SSID, PASS);

  // Wait up to ~15 seconds
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi: CONNECTED");
    Serial.print("IP: ");   Serial.println(WiFi.localIP());
    Serial.print("RSSI: "); Serial.println(WiFi.RSSI());
  } else {
    Serial.print("WiFi: NOT connected, status=");
    Serial.println(WiFi.status()); // see mapping below
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.print("STA MAC: ");
  Serial.println(WiFi.macAddress());

  connectWiFi();
}

void loop() {
  setup();
  // // Periodically print status
  // static unsigned long last = 0;
  // if (millis() - last > 5000) {
  //   last = millis();

  //   wl_status_t s = WiFi.status();
  //   if (s == WL_CONNECTED) {
  //     Serial.print("[OK] IP=");
  //     Serial.print(WiFi.localIP());
  //     Serial.print(" RSSI=");
  //     Serial.println(WiFi.RSSI());
  //   } else {
  //     Serial.print("[DROP] status=");
  //     Serial.println((int)s);
  //     // try reconnect
  //     connectWiFi();
  //   }
  // }
}
