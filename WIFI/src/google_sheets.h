#pragma once
#include <Arduino.h>

bool postToGoogleSheets(const String& jsonPayload);
String makePayload();
