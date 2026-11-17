#include "utils/bme_sample.hpp"

// 7Semi keeps readAll()/data private; friend access is limited to this TU
#define private public
#include <7Semi_BME280.h>
#undef private

bool readBmeSample(BMI280_7Semi &bme, BmeSample &out) {
  if (!bme.readAll())
    return false;
#ifdef BME280_DOUBLE_ENABLE
  // Bosch API: temperature and humidity are already in °C / %RH
  out.temperature_c = static_cast<float>(bme.data.temperature);
  out.humidity_rh = static_cast<float>(bme.data.humidity);
#else
  out.temperature_c = bme.data.temperature / 100.0f;
  out.humidity_rh = bme.data.humidity / 1024.0f;
#endif
  out.pressure_hpa = static_cast<float>(bme.data.pressure) / 100.0f; // Pa → hPa
  return true;
}
