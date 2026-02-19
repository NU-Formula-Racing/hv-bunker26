#include "google_sheets.h"
#include "wifi_connect.h"
#include <WiFi.h>
#include <HTTPClient.h>

const char* GOOGLE_SCRIPT_URL =
  "https://script.google.com/macros/s/AKfycbxtlJvyy2Q0xReYulAGB1CNoFAAwGCL0wNKL1c_5MKj8xmABIi9E8qyi3wf130gixbL/exec";

bool postToGoogleSheets(const String& jsonPayload) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected. Reconnecting...");
    connectWiFi();
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Still offline, aborting POST.");
    return false;
  }

  HTTPClient http;

  http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);

  if (!http.begin(GOOGLE_SCRIPT_URL)) {
    Serial.println("http.begin() failed");
    return false;
  }

  http.addHeader("Content-Type", "application/json");
  http.addHeader("User-Agent", "ESP32-S3-HV-Bunker/1.0");
  http.setTimeout(15000);

  int code = http.POST((uint8_t*)jsonPayload.c_str(), jsonPayload.length());

  String resp = http.getString();

  http.end();

  Serial.print("POST code: ");
  Serial.println(code);

  // Treating redirect as success
  if (code == 302) {
    Serial.println("Redirect from Apps Script (normal). Treating as success.");
    return true;
  }

  return (code >= 200 && code < 400);
}

String makePayload() {
  // Minimal manual JSON (numbers must not be quoted; strings must be quoted)
  String s = "{";
  s += "\"device_id\":\"esp32-s3\",";
  s += "\"soc_pct\":12.3,";
  s += "\"v_max\":4.2,";
  s += "\"v_min\":3.9,";
  s += "\"v_avg\":4.05,";
  s += "\"t_max\":40.1,";
  s += "\"t_min\":25.2,";
  s += "\"t_avg\":32.6,";
  s += "\"cell_voltages_V\":[4.01,4.02],";
  s += "\"cell_temps_C\":[30.1,31.2]";
  s += "}";
  return s;
}
