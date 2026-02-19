#ifndef GOOGLE_SHEETS_H
#define GOOGLE_SHEETS_H

#include <Arduino.h>

// POST JSON payload to the Google Apps Script web app. Returns true on success (2xx or 302).
bool postToGoogleSheets(const String& jsonPayload);

// Build the JSON payload for the sheet (device_id, voltages, temps, etc.).
String makePayload();

#endif
