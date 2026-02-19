#include <WiFi.h>
#include "wifi_connect.h"
#include "google_sheets.h"

int counter = 0;

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.print("STA MAC: ");
  Serial.println(WiFi.macAddress());

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
}
