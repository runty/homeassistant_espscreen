#pragma once
#include <cstdint>
#include "wads-p4-color-lut.h"

inline uint16_t wads_p4_green_gain = 52;
inline uint16_t wads_p4_blue_gain = 22;

static inline uint16_t wads_p4_tone(uint16_t value, uint16_t maximum) {
  const int32_t v = value, m = maximum;
  const int32_t numerator = v * (m - v) * (3 * m - 5 * v);
  const int32_t denominator = m * m * 10;
  const int32_t rounded = numerator >= 0 ? (numerator + denominator / 2) / denominator
                                         : -((-numerator + denominator / 2) / denominator);
  return static_cast<uint16_t>(v + rounded);
}
static inline uint16_t wads_p4_correct_pixel(uint16_t c) {
  // Preserve the low red night-clock pixels, including antialiasing.
  const uint16_t source_red = (c >> 11) & 31;
  if ((c & 0x7ff) == 0 && source_red <= 8) return c;
  const int32_t linear[3] = {wads_p4_linear5[source_red],
                             wads_p4_linear6[(c >> 5) & 63],
                             wads_p4_linear5[c & 31]};
  uint16_t mapped[3];
  const uint16_t *inverse[3] = {wads_p4_inverse_red, wads_p4_inverse_green, wads_p4_inverse_blue};
  for (int channel = 0; channel < 3; ++channel) {
    int32_t value = 2048;
    for (int source = 0; source < 3; ++source)
      value += wads_p4_color_matrix[channel][source] * linear[source];
    value >>= 12;
    if (value < 0) value = 0;
    if (value > 4096) value = 4096;
    mapped[channel] = inverse[channel][(value + 8) >> 4];
  }
  uint16_t red5 = (mapped[0] * 31 + 127) / 255;
  const uint16_t green6 = (mapped[1] * 63 + 127) / 255;
  const uint16_t blue5 = (mapped[2] * 31 + 127) / 255;
  // Red-primary measurements bypassed the original tone curve; mixed
  // colors use it. Undo that curve here to match the measured model.
  if (green6 != 0 || blue5 != 0) red5 = wads_p4_inverse_red_tone[red5];
  c = (red5 << 11) | (green6 << 5) | blue5;
  const uint16_t red = (c >> 11) & 31;
  const uint16_t green = (c >> 5) & 63;
  const uint16_t blue = c & 31;
  // Leave pure red untouched so the Good Night clock keeps its dimness.
  const uint16_t corrected_red = (green == 0 && blue == 0) ? red : wads_p4_tone(red, 31);
  const uint16_t corrected_green = wads_p4_tone(green, 63);
  const uint16_t corrected_blue = wads_p4_tone(blue, 31);
  return (corrected_red << 11) | (((corrected_green * wads_p4_green_gain + 31) / 63) << 5) |
         ((corrected_blue * wads_p4_blue_gain + 15) / 31);
}
