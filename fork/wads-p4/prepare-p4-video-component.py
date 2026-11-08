#!/usr/bin/env python3
"""Copy the pinned Tessera smart_display component and patch P4 full-screen video.

Usage: prepare-p4-video-component.py UPSTREAM_COMPONENTS_DIR OUTPUT_DIR
UPSTREAM_COMPONENTS_DIR must be from Tessera screens-v0.4.97 (commit
9c7c6a674d2e214355b85d810fb7d08f8fcd3544). OUTPUT_DIR contains smart_display.
"""
from pathlib import Path
import shutil
import subprocess
import sys

source = Path(sys.argv[1]) / 'smart_display'
target = Path(sys.argv[2]) / 'smart_display'
base = '9c7c6a674d2e214355b85d810fb7d08f8fcd3544'
result = subprocess.run(['git', '-C', str(source.parent.parent), 'merge-base', '--is-ancestor', base, 'HEAD'])
if result.returncode:
    raise SystemExit(f'Tessera checkout must descend from {base}')
if target.exists():
    raise SystemExit(f'{target} exists; use a clean output directory')
shutil.copytree(source, target, ignore=shutil.ignore_patterns('__pycache__', '*.pyc'))
# The remote build bundle carries local component folders, not files beside
# them. Keep the translation resources inside this component so local and
# remote ESPHome builds produce identical text tables.
shutil.copytree(source.parent.parent / 'screen_manager' / 'translations', target / '_wads_translations')
text_gen = target / 'screen_text_gen.py'
text_source = text_gen.read_text()
original = "TRANSLATIONS = ROOT / 'screen_manager' / 'translations'"
assert text_source.count(original) == 1
text_gen.write_text(text_source.replace(original, "TRANSLATIONS = Path(__file__).resolve().parent / '_wads_translations'"))
p = target / 'live_view.cpp'
s = p.read_text()
needle = '  Look look;\n'
assert s.count(needle) == 1
s = s.replace(needle, needle + '''#if defined(PICTURE_ENGINE_P4)
  uint16_t *color_lut = nullptr;
  uint16_t lut_green = 0xffff, lut_blue = 0xffff;
#endif
''')
needle = 'static void free_stream(Stream *s) {\n'
assert s.count(needle) == 1
s = s.replace(needle, needle + '''#if defined(PICTURE_ENGINE_P4)
  if (s->color_lut) heap_caps_free(s->color_lut);
#endif
''')
needle = '#include <esp_err.h>\n'
assert s.count(needle) == 1
s = s.replace(needle, needle + '''#if defined(PICTURE_ENGINE_P4)
#include <esp_cache.h>
#include "wads-p4-color-transform.h"
#pragma message("WADS_P4_VIDEO_COLOR_PATCH_ACTIVE")
#endif
''')
needle = '''  if (!decoded) {
    s.skipped++;
    return;
  }
  if (s.shown == 0)
'''
assert s.count(needle) == 1
s = s.replace(needle, '''  if (!decoded) {
    s.skipped++;
    return;
  }
#if defined(PICTURE_ENGINE_P4)
  // Full-screen JPEG streams bypass LVGL's calibrated flush. Correct this
  // private RGB565 frame before the PPA scales it; stills and tile streams
  // are untouched. Invalidate DMA output before CPU access, then write back
  // the changes before the PPA reads the frame.
  if (!s.look.tile) {
    if (!s.color_lut)
      s.color_lut = static_cast<uint16_t *>(heap_caps_malloc(65536 * sizeof(uint16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (s.color_lut && (s.lut_green != wads_p4_green_gain || s.lut_blue != wads_p4_blue_gain)) {
      const uint16_t green = wads_p4_green_gain, blue = wads_p4_blue_gain;
      for (uint32_t i = 0; i < 65536 && !s.stopping; ++i) {
        s.color_lut[i] = wads_p4_correct_pixel(static_cast<uint16_t>(i));
        if ((i & 4095) == 4095) vTaskDelay(1);
      }
      if (s.stopping) return;
      s.lut_green = green;
      s.lut_blue = blue;
    }
    if (esp_cache_msync(s.picture, s.picture_size, ESP_CACHE_MSYNC_FLAG_DIR_M2C) == ESP_OK) {
      auto *pixels = reinterpret_cast<uint16_t *>(s.picture);
      for (int y = 0; y < h; ++y) {
        auto *row = pixels + static_cast<size_t>(y) * stride_w;
        for (int x = 0; x < w; ++x)
          row[x] = s.color_lut ? s.color_lut[row[x]] : wads_p4_correct_pixel(row[x]);
      }
      if (esp_cache_msync(s.picture, s.picture_size, ESP_CACHE_MSYNC_FLAG_DIR_C2M) != ESP_OK)
        ESP_LOGW(TAG, "could not flush corrected full-screen frame");
    } else {
      ESP_LOGW(TAG, "could not invalidate full-screen frame for color correction");
    }
  }
#endif
  if (s.shown == 0)
''')
p.write_text(s)
