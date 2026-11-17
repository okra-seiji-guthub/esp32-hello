#pragma once

class BMI280_7Semi;

struct BmeSample {
  float temperature_c = 0.f;
  float pressure_hpa = 0.f;
  float humidity_rh = 0.f;
};

bool readBmeSample(BMI280_7Semi &bme, BmeSample &out);
