# Optional RGB888 overlay for the Waveshare P4

This overlay carries the display pipeline used on one ESP32-P4-WIFI6-Touch-LCD-4B. It renders the MIPI display,
LVGL, JPEG stills, live tiles and full-screen camera video in packed RGB888 rather than RGB565, without dithering.
It is opt-in: upstream board profiles and other devices continue to use their existing pixel formats.

The color correction and default green/blue gains in these files were measured on one physical panel. They are not
a universal profile or a verified ICC calibration; remeasure another panel instead of assuming they match. The
8-bit port has not had a separate SpyderX verification. Low-red night-clock pixels are preserved. On the tested panel
the owner saw less banding in the same sky photo; some banding remained and opening a view sometimes paused for
roughly half a second. Long-term stability and other P4 boards have not been validated.

The changes depend on Tessera 0.4.97 at `9c7c6a674d2e214355b85d810fb7d08f8fcd3544` and ESPHome 2026.9.0.
Review and port the component patch before changing either dependency; never assume it applies to a later release.
The scripts check the base and patch inputs. Prepare in a new directory:

```sh
python3 fork/wads-p4/prepare-p4-video-component.py components /path/to/new-components
cp -R /path/to/esphome-2026.9.0/esphome/components/lvgl /path/to/new-components/lvgl
python3 fork/wads-p4/prepare-p4-rgb888-components.py /path/to/new-components
```

Copy the output `smart_display` and `lvgl` folders into a local ESPHome external-component directory and copy the
four `wads-p4-*.h` headers beside the ESPHome profile. The [example override](example.local.yaml) shows the needed
display format, LVGL flush and component selection. It deliberately contains no Wi-Fi credentials, Home Assistant
entity configuration, branding, or Good Night automation. Merge it with your existing device override, preserving
its settings. The validated device was compiled with these components on Bai and installed over Wi-Fi; keep a
recovery route and test still images, camera video, touch, standby and brightness on the actual panel.

Run `c++ -std=c++17 -O2 fork/wads-p4/test_p4_rgb888.cpp -o /tmp/test_p4_rgb888 && /tmp/test_p4_rgb888` for the
host-side grayscale and night-red checks. This does not replace a device test.
