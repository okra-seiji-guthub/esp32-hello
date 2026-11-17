#pragma once

#include <Arduino.h>
#include <Wire.h>

inline void i2cScanAndPrint(TwoWire &bus = Wire) {
  Serial.println("I2C scan (7-bit addresses):");
  uint8_t found = 0;
  for (uint8_t addr = 1; addr < 127; addr++) {
    bus.beginTransmission(addr);
    if (bus.endTransmission() == 0) {
      Serial.printf("  0x%02X\n", addr);
      found++;
    }
  }
  if (found == 0) {
    Serial.println("  (none — check SDA/SCL/GND/3V3, wiring when adding antenna)");
  }
}

// BME280 chip ID register 0xD0 must read 0x60
inline bool i2cProbeBme280(uint8_t addr, TwoWire &bus = Wire) {
  bus.beginTransmission(addr);
  bus.write(0xD0);
  if (bus.endTransmission(false) != 0)
    return false;
  if (bus.requestFrom(addr, (uint8_t)1) != 1)
    return false;
  return bus.available() && bus.read() == 0x60;
}

inline uint8_t detectBme280Address(TwoWire &bus = Wire) {
  for (uint8_t addr : {uint8_t(0x76), uint8_t(0x77)}) {
    if (i2cProbeBme280(addr, bus)) {
      Serial.printf("BME280 detected at 0x%02X\n", addr);
      return addr;
    }
  }
  Serial.println("BME280 not found at 0x76 or 0x77");
  return 0;
}
