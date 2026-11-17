#pragma once

#include "config.hpp"
#include <WiFi.h>
#include <WiFiType.h>
#include <esp_coexist.h>
#include <esp_wifi.h>

inline const char *wifiStatusToString(wl_status_t status) {
  switch (status) {
  case WL_IDLE_STATUS:
    return "IDLE";
  case WL_NO_SSID_AVAIL:
    return "NO_SSID_AVAIL";
  case WL_SCAN_COMPLETED:
    return "SCAN_COMPLETED";
  case WL_CONNECTED:
    return "CONNECTED";
  case WL_CONNECT_FAILED:
    return "CONNECT_FAILED";
  case WL_CONNECTION_LOST:
    return "CONNECTION_LOST";
  case WL_DISCONNECTED:
    return "DISCONNECTED";
  default:
    return "UNKNOWN";
  }
}

inline const char *wifiScanCodeToString(int n) {
  if (n == WIFI_SCAN_RUNNING)
    return "SCAN_RUNNING";
  if (n == WIFI_SCAN_FAILED)
    return "SCAN_FAILED";
  if (n >= 0)
    return "OK";
  return "UNKNOWN";
}

inline void wifiLogMac() {
  uint8_t mac[6] = {};
  const esp_err_t err = esp_wifi_get_mac(WIFI_IF_STA, mac);
  if (err == ESP_OK) {
    Serial.printf("WiFi STA MAC %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0],
                  mac[1], mac[2], mac[3], mac[4], mac[5]);
  } else {
    Serial.printf("WiFi STA MAC unavailable (esp_err=%d)\n", (int)err);
  }
}

// Bring up STA and RF before scanning
inline bool wifiPrepareSta(uint32_t warmup_ms = 1200) {
  WiFi.persistent(false);
  WiFi.mode(WIFI_OFF);
  delay(100);
  if (!WiFi.mode(WIFI_STA)) {
    Serial.println("WiFi: mode(STA) failed");
    return false;
  }
  WiFi.setSleep(false);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);
  WiFi.disconnect(true, false);
  esp_coex_preference_set(ESP_COEX_PREFER_WIFI);

  WiFi.enableSTA(true);
  delay(warmup_ms);

  wifi_mode_t mode = WIFI_MODE_NULL;
  if (esp_wifi_get_mode(&mode) != ESP_OK || (mode & WIFI_MODE_STA) == 0) {
    Serial.println("WiFi: STA interface not up after warmup");
    return false;
  }
  wifiLogMac();
  return true;
}

// Synchronous scan with retries; returns AP count or WIFI_SCAN_*
inline int wifiScanOnce(bool show_hidden = true) {
  WiFi.scanDelete();
  delay(30);
  // Active scan, 350 ms per channel
  return WiFi.scanNetworks(false, show_hidden, false, 350);
}

inline int wifiScanWithRetry(int attempts = 3) {
  int last = WIFI_SCAN_FAILED;
  for (int i = 0; i < attempts; ++i) {
    last = wifiScanOnce();
    Serial.printf("WiFi: scan attempt %d/%d -> %d (%s)\n", i + 1, attempts,
                  last, wifiScanCodeToString(last));
    if (last > 0)
      return last;
    if (last == 0)
      return 0;
    delay(400);
  }
  return last;
}

inline void wifiScanAndReport(const char *target_ssid) {
  Serial.println("WiFi: scanning ...");
  const int n = wifiScanWithRetry(3);

  if (n == WIFI_SCAN_RUNNING) {
    Serial.println("WiFi: scan still running (unexpected for sync scan)");
    return;
  }
  if (n == WIFI_SCAN_FAILED) {
    Serial.println(
        "WiFi: scan failed — RF/driver (reboot, antenna, USB power)");
    return;
  }
  if (n == 0) {
    Serial.println(
        "WiFi: 0 APs seen — move near router, enable 2.4GHz, check antenna");
    return;
  }

  bool found = false;
  for (int i = 0; i < n; ++i) {
    const String ssid = WiFi.SSID(i);
    const int32_t rssi = WiFi.RSSI(i);
    const wifi_auth_mode_t auth = WiFi.encryptionType(i);
    const int32_t ch = WiFi.channel(i);
    Serial.printf("  [%d] \"%s\" ch=%ld RSSI=%ld auth=%d\n", i, ssid.c_str(),
                  (long)ch, (long)rssi, (int)auth);
    if (ssid == target_ssid) {
      found = true;
    }
  }
  Serial.printf("WiFi: target SSID \"%s\" %s\n", target_ssid,
                found ? "is visible" : "NOT in scan list");
}

// Returns false on timeout or auth/SSID failure
inline bool connectWifi(const WiFiSettings &settings,
                        uint32_t timeout_ms = 60 * 1000) {
  if (!wifiPrepareSta()) {
    return false;
  }

  wifiScanAndReport(settings.ssid);

  Serial.printf("WiFi: connecting to SSID \"%s\"\n", settings.ssid);
  WiFi.begin(settings.ssid, settings.password);

  const uint32_t t0 = millis();
  wl_status_t last = WL_DISCONNECTED;
  while (WiFi.status() != WL_CONNECTED) {
    const wl_status_t st = WiFi.status();
    if (st != last) {
      last = st;
      Serial.printf("WiFi status: %s (%d)\n", wifiStatusToString(st), (int)st);
    }
    if (st == WL_CONNECT_FAILED || st == WL_NO_SSID_AVAIL) {
      Serial.println("WiFi: give up (SSID not found or auth failed)");
      wifiScanAndReport(settings.ssid);
      return false;
    }
    if (millis() - t0 >= timeout_ms) {
      Serial.printf("WiFi: timeout (last status %s)\n",
                    wifiStatusToString(WiFi.status()));
      wifiScanAndReport(settings.ssid);
      return false;
    }
    delay(250);
  }

  Serial.printf("WiFi OK, IP=%s RSSI=%d dBm\n",
                WiFi.localIP().toString().c_str(), WiFi.RSSI());
  return true;
}
