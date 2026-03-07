#include "google_sheets.h"
#include "bms_data.h"
#include "wifi_connect.h"
#include "uart_serial.h"
#include <WiFi.h>
#include <HTTPClient.h>

const char* GOOGLE_SCRIPT_URL =
  "https://script.google.com/macros/s/AKfycbxtlJvyy2Q0xReYulAGB1CNoFAAwGCL0wNKL1c_5MKj8xmABIi9E8qyi3wf130gixbL/exec";

bool postToGoogleSheets(const String& jsonPayload) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial0.println("WiFi not connected. Reconnecting...");
    connectWiFi();
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial0.println("Still offline, aborting POST.");
    return false;
  }

  HTTPClient http;

  http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);

  if (!http.begin(GOOGLE_SCRIPT_URL)) {
    Serial0.println("http.begin() failed");
    return false;
  }

  http.addHeader("Content-Type", "application/json");
  http.addHeader("User-Agent", "ESP32-S3-HV-Bunker/1.0");
  http.setTimeout(15000);

  int code = http.POST((uint8_t*)jsonPayload.c_str(), jsonPayload.length());
  http.end();

  if (code == 302)
    return true;

  return (code >= 200 && code < 400);
}

String makePayload() {
  bms_data_t avg;
  bms_accum_snapshot(&avg);

  static char buf[3000];
  int pos = 0;

  pos += snprintf(buf + pos, sizeof(buf) - pos,
    "{\"device_id\":\"esp32-s3\","
    "\"soc\":%.2f,"
    "\"total_v\":%.2f,"
    "\"v_max\":%.2f,"
    "\"v_min\":%.2f,"
    "\"v_avg\":%.2f,"
    "\"t_max\":%.2f,"
    "\"t_min\":%.2f,"
    "\"t_avg\":%.2f,"
    "\"current\":%.2f,"
    "\"time\":%u,"
    "\"undervoltage\":%d,"
    "\"overvoltage\":%d,"
    "\"pec\":%d,"
    "\"overtemperature\":%d,",
    avg.soc, avg.total_v,
    avg.max_v, avg.min_v, avg.avg_v,
    avg.max_temp, avg.min_temp, avg.avg_temp,
    avg.current, avg.time_ms,
    avg.undervoltage, avg.overvoltage,
    avg.pec, avg.overtemperature);

  int total = avg.num_cells > 0 ? avg.num_cells : BMS_NUM_CELLS;
  for (int g = 0; g < total; g += 10) {
    int end = g + 10;
    if (end > total) end = total;
    pos += snprintf(buf + pos, sizeof(buf) - pos,
                    "\"cells_%d_%d\":[", g + 1, end);
    for (int i = g; i < end && pos < (int)sizeof(buf) - 20; i++) {
      if (i > g) buf[pos++] = ',';
      pos += snprintf(buf + pos, sizeof(buf) - pos, "%.2f", avg.cell_v[i]);
    }
    pos += snprintf(buf + pos, sizeof(buf) - pos, "],");
  }

  if (pos > 0 && buf[pos - 1] == ',') pos--;
  pos += snprintf(buf + pos, sizeof(buf) - pos, "}");

  return String(buf);
}
