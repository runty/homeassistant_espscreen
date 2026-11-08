#pragma once
#include <cstdint>
#include <array>
#include "wads-p4-color-transform.h"
// Continuous 8-bit rendering of the measured P4 color model.
// A separate SpyderX verification of this RGB888 path is still needed.
static constexpr uint16_t wads_p4_linear8[256] = {
  0, 1, 2, 4, 5, 6, 7, 9, 10, 11, 12, 14, 15, 16, 18, 20,
  21, 23, 25, 27, 29, 31, 33, 35, 37, 40, 42, 45, 48, 50, 53, 56,
  59, 62, 66, 69, 72, 76, 79, 83, 87, 91, 95, 99, 103, 107, 112, 116,
  121, 126, 131, 136, 141, 146, 151, 156, 162, 168, 173, 179, 185, 191, 197, 204,
  210, 217, 223, 230, 237, 244, 251, 258, 265, 273, 280, 288, 296, 304, 312, 320,
  329, 337, 346, 354, 363, 372, 381, 390, 400, 409, 419, 429, 438, 448, 458, 469,
  479, 490, 500, 511, 522, 533, 544, 556, 567, 579, 590, 602, 614, 626, 639, 651,
  664, 676, 689, 702, 715, 729, 742, 756, 769, 783, 797, 811, 826, 840, 855, 869,
  884, 899, 914, 930, 945, 961, 976, 992, 1008, 1025, 1041, 1058, 1074, 1091, 1108, 1125,
  1142, 1160, 1177, 1195, 1213, 1231, 1249, 1268, 1286, 1305, 1324, 1343, 1362, 1381, 1400, 1420,
  1440, 1460, 1480, 1500, 1521, 1541, 1562, 1583, 1604, 1625, 1647, 1668, 1690, 1712, 1734, 1756,
  1778, 1801, 1824, 1846, 1869, 1893, 1916, 1940, 1963, 1987, 2011, 2035, 2060, 2084, 2109, 2134,
  2159, 2184, 2210, 2235, 2261, 2287, 2313, 2339, 2366, 2392, 2419, 2446, 2473, 2501, 2528, 2556,
  2584, 2612, 2640, 2668, 2697, 2725, 2754, 2783, 2813, 2842, 2872, 2902, 2931, 2962, 2992, 3022,
  3053, 3084, 3115, 3146, 3178, 3209, 3241, 3273, 3305, 3338, 3370, 3403, 3436, 3469, 3502, 3535,
  3569, 3603, 3637, 3671, 3705, 3740, 3775, 3810, 3845, 3880, 3916, 3951, 3987, 4023, 4060, 4096,
};
// Matrix terms are immutable. Precompute them in flash to avoid nine
// multiplies per output pixel on the P4's UI and camera paths.
using WadsP4Terms = std::array<std::array<std::array<int32_t, 256>, 3>, 3>;
static constexpr WadsP4Terms wads_p4_make_terms() {
 WadsP4Terms terms{};
 for (int c=0;c<3;++c) for (int source=0;source<3;++source)
  for (int i=0;i<256;++i) terms[c][source][i]=wads_p4_color_matrix[c][source]*wads_p4_linear8[i];
 return terms;
}
static constexpr WadsP4Terms wads_p4_terms = wads_p4_make_terms();
struct WadsP4Rgb888Map { uint8_t map[3][257]; uint16_t green = 0xffff, blue = 0xffff; };
static void wads_p4_init_rgb888_map(WadsP4Rgb888Map &cal) {
 for (int i=0; i<257; ++i) {
  cal.map[0][i] = wads_p4_inverse_red[i];
  cal.map[1][i] = (wads_p4_tone(wads_p4_inverse_green[i],255) * wads_p4_green_gain + 31)/63;
  cal.map[2][i] = (wads_p4_tone(wads_p4_inverse_blue[i],255) * wads_p4_blue_gain + 15)/31;
 }
}
static inline void wads_p4_correct_rgb888(uint8_t *pixel, const WadsP4Rgb888Map &cal) {
 // LVGL RGB888 uses packed B,G,R bytes.
 const uint8_t blue=pixel[0], green=pixel[1], red=pixel[2];
 if (!blue && !green && red <= 66) return;
 uint8_t mapped[3];
 for (int c=0;c<3;++c) {
  int32_t v=2048 + wads_p4_terms[c][0][red] + wads_p4_terms[c][1][green] + wads_p4_terms[c][2][blue];
  v >>= 12;
  if (v<0) v=0; else if (v>4096) v=4096;
  mapped[c]=cal.map[c][(v+8)>>4];
 }
 pixel[0]=mapped[2];pixel[1]=mapped[1];pixel[2]=mapped[0];
}
