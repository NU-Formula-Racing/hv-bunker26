#include <WiFi.h>
#include <HTTPClient.h>
int counter = 0;
const char* SSID = "Device-Northwestern";   // <- set this
const char* PASS = "";                      // <- set if required; otherwise ""
const char* GOOGLE_SCRIPT_URL =
  "https://script.google.com/macros/s/AKfycbxtlJvyy2Q0xReYulAGB1CNoFAAwGCL0wNKL1c_5MKj8xmABIi9E8qyi3wf130gixbL/exec";

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
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Still offline, aborting POST.");
    return false;
  }

  HTTPClient http;

  // Equivalent to curl -L (follow redirects)
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  // Start request
  if (!http.begin(GOOGLE_SCRIPT_URL)) {
    Serial.println("http.begin() failed");
    return false;
  }

  http.addHeader("Content-Type", "application/json");
  http.addHeader("User-Agent", "ESP32-S3-HV-Bunker/1.0");
  http.setTimeout(15000);

  // Send POST body
  int code = http.POST((uint8_t*)jsonPayload.c_str(), jsonPayload.length());

  // Read response text (even on error codes, server may reply with useful info)
  String resp = http.getString();

  http.end();

  Serial.print("POST code: ");
  Serial.println(code);
  if (resp.length()) {
    Serial.print("Response: ");
    Serial.println(resp);
  }

  // 2xx = success
  return (code >= 200 && code < 300);
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
