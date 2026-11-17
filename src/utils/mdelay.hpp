#pragma once

#include <Arduino.h>
#include <cstdint>

namespace utils {

struct MDelay {
  uint32_t delay_ms;

  MDelay(uint32_t delay_ms) : delay_ms(delay_ms) { t0 = micros(); }

  void reset() { t0 = micros(); }

  uint32_t operator()() {
    uint32_t elapsed = (micros() - t0) / 1000;
    uint32_t remaining = (delay_ms <= elapsed) ? 0 : delay_ms - elapsed;
    delay(remaining);
    t0 = micros();
    return remaining;
  }

private:
  uint32_t t0;
};

} // namespace utils
