// Run: c++ -std=c++17 -O2 tests/test_p4_rgb888.cpp -o /tmp/test_p4_rgb888 && /tmp/test_p4_rgb888
#include <cassert>
#include <cstdint>
#include <set>
#include "wads-p4-rgb888-color.h"

int main() {
  WadsP4Rgb888Map calibration;
  wads_p4_init_rgb888_map(calibration);
  uint8_t night[] = {0, 0, 40}; // packed BGR
  wads_p4_correct_rgb888(night, calibration);
  assert(night[0] == 0 && night[1] == 0 && night[2] == 40);
  std::set<uint8_t> blue, green, red;
  for (unsigned n = 0; n < 256; ++n) {
    uint8_t pixel[] = {uint8_t(n), uint8_t(n), uint8_t(n)};
    wads_p4_correct_rgb888(pixel, calibration);
    blue.insert(pixel[0]); green.insert(pixel[1]); red.insert(pixel[2]);
  }
  // This catches accidental reintroduction of 5/6/5-bit quantization.
  assert(red.size() > 32 && green.size() > 64 && blue.size() > 32);
}
