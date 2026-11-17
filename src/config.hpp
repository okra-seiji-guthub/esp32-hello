#pragma once

#include "secrets.h"

// WiFi settings
struct WiFiSettings {
  const char *ssid = WIFI_SSID;
  const char *password = WIFI_PASSWORD;
};
