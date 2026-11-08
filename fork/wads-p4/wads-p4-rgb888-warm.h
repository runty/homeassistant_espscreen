#pragma once
#include "wads-p4-rgb888-color.h"
#include "lvgl.h"
#include <esp_timer.h>
#include "esphome/core/log.h"
#include "esphome/components/lvgl/lvgl_esphome.h"
// Apply the calibrated transform to packed RGB888 LVGL strips.
static void wads_p4_warm_flush(lv_display_t *display, const lv_area_t *area, uint8_t *pixels) {
  static WadsP4Rgb888Map cal;
  if (cal.green != wads_p4_green_gain || cal.blue != wads_p4_blue_gain) {
    wads_p4_init_rgb888_map(cal);
    cal.green = wads_p4_green_gain;
    cal.blue = wads_p4_blue_gain;
  }
  const int64_t started = esp_timer_get_time();
  const size_t count = static_cast<size_t>(lv_area_get_width(area)) * lv_area_get_height(area);
  for (size_t i=0; i<count; ++i) wads_p4_correct_rgb888(pixels + 3*i, cal);
  const int64_t corrected = esp_timer_get_time();
  esphome::lvgl::LvglComponent::static_flush_cb(display, area, pixels);
  const int64_t ended = esp_timer_get_time();
  static int64_t last_report = 0;
  if (ended - started > 100000 && ended - last_report > 2000000) {
    ESP_LOGW("wads_rgb888", "flush %ux%u: correction %u ms, display %u ms",
             unsigned(lv_area_get_width(area)), unsigned(lv_area_get_height(area)),
             unsigned((corrected-started)/1000), unsigned((ended-corrected)/1000));
    last_report = ended;
  }
}
