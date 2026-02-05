#include <Arduino.h>
#include "esp_wpa2.h"
#include <WiFi.h>
#include <HTTPClient.h>

const char* SSID = "Device-Northwestern";   // <- set this
const char* PASS = "";                      // <- set if required; otherwise ""

// App Script Web App URL:
#define GOOGLE_SCRIPT_URL "https://script.google.com/macros/s/AKfycbxtlJvyy2Q0xReYulAGB1CNoFAAwGCL0wNKL1c_5MKj8xmABIi9E8qyi3wf130gixbL/exec"

// Identifiers
#define DEVICE_ID "esp32-bunker-01"

// Time of last, non-blocking post
static unsigned long lastPostMs = 0;

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


bool postToGoogleSheets(const String& jsonPayload) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected. Reconnecting...");
    connectWiFi();
  }

  HTTPClient http; // creates an HTTP client object
  http.begin(GOOGLE_SCRIPT_URL); // send request to the URL
  http.addHeader("Content-Type", "application/json"); // specify content-type header 

  //sends the HTTP POST request and returns an integer status code
  int code = http.POST((uint8_t*)jsonPayload.c_str(), jsonPayload.length());
  String resp = http.getString(); // Reads text the server sent back (like {"ok":true}).
  http.end();

  Serial.print("POST code: ");
  Serial.println(code);
  if (resp.length()) {
    Serial.print("Response: ");
    Serial.println(resp);
  }

  return (code >= 200 && code < 300); // Returns true if status code is 2xx (success)
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.print("STA MAC: ");
  Serial.println(WiFi.macAddress());

  connectWiFi();
}

void loop() {
  // Periodically print status
  static unsigned long last = 0;
  if (millis() - last > 5000) {
    last = millis();

    wl_status_t s = WiFi.status();
    if (s == WL_CONNECTED) {
      Serial.print("[OK] IP=");
      Serial.print(WiFi.localIP());
      Serial.print(" RSSI=");
      Serial.println(WiFi.RSSI());
    } else {
      Serial.print("[DROP] status=");
      Serial.println((int)s);
      // try reconnect
      connectWiFi();
    }
  }

  // Send a test POST every 10 seconds
  if (millis() - lastPostMs >= 10000) {
    lastPostMs = millis();

    // Example payload (replace these later with real values)
    float soc = 86.4;
    float vMax = 4.12, vMin = 4.08, vAvg = 4.10;
    float tMax = 37.2, tMin = 28.9, tAvg = 32.4;

    // Arrays stored as JSON arrays (Apps Script will stringify them into one cell)
    // Start with a String to avoid ambiguous const char* + String concatenation
    String payload = String("{\"device_id\":\"") + DEVICE_ID + String("\",");
    payload += "\"soc_pct\":" + String(soc, 1) + ",";
    payload += "\"v_max\":" + String(vMax, 3) + ",";
    payload += "\"v_min\":" + String(vMin, 3) + ",";
    payload += "\"v_avg\":" + String(vAvg, 3) + ",";
    payload += "\"t_max\":" + String(tMax, 1) + ",";
    payload += "\"t_min\":" + String(tMin, 1) + ",";
    payload += "\"t_avg\":" + String(tAvg, 1) + ",";
    payload += "\"cell_voltages_V\":[4.10,4.11,4.09,4.12],";
    payload += "\"cell_temps_C\":[31.0,32.5,30.1,33.2]";
    payload += "}";

    Serial.println("Posting:");
    Serial.println(payload);

    bool ok = postToGoogleSheets(payload);
    Serial.println(ok ? "Upload OK" : "Upload FAILED");
  }

  delay(50);
}