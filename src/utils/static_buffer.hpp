// static_buffer.hpp
#pragma once

#include <algorithm>
#include <cstdint>
#include <string_view>

namespace utils {

template <size_t N> struct StaticBuffer {
  char data[N]{};
  uint8_t len = 0; // uint8_t length is enough for buffers <= 255 bytes

  // Append a C string without overflowing the fixed buffer
  bool append(std::string_view sv) {
    if (len + sv.size() + 1 > N)
      return false; // Reject append if it would overflow
    std::copy_n(sv.data(), sv.size(), data + len);
    len += sv.size();
    data[len] = '\0';
    return true;
  }
};

} // namespace utils
