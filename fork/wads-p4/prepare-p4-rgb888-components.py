#!/usr/bin/env python3
"""Apply the P4 RGB888 prototype to copies of the pinned ESPHome components.

Pass a directory containing smart_display/ (prepared with the existing P4 video
patch) and lvgl/ (from ESPHome 2026.9.0). This modifies only that directory.
"""
import argparse
import hashlib
from pathlib import Path
import subprocess

EXPECTED_SHA256 = {
    'smart_display/glass.h': 'b01ad28dedde01185377c94660c87d01928cd49ede5a3978e7f583ed3558528f',
    'smart_display/glass_dsi.cpp': 'eab6d1087ca48305a903f5806aa7251494694422c078b45a5a00d5ad7f6cbf6e',
    'smart_display/live_view.cpp': '7c8480630d1c9f24b2a17752758555955b89548bc276d9ab54ac08a5bdc3c0dc',
    'smart_display/live_view.h': '0cafef07a4f7462500b4d67281d2530fe79d65b309dfd7a2270f182320b8bd95',
    'smart_display/picture_engine.h': '9dfef35b7ac99d6b347632503c5b35e550b3fda4da8ac2fae9ec66cc6a61c509',
    'smart_display/picture_engine_p4.cpp': 'c6a7a3fc827a62e1cc3e29d0d3bd34bb8c0a9b721d0341cdda47ce7e1f556f77',
    'smart_display/picture_fetch.cpp': '3e521122c8dc4b1559d604244de9ac735d579af87eae9d486559004f909b47ea',
    'smart_display/picture_jpeg.h': '82f69ae685257308cc97988379db38f9d15cf513575b7cfa85dfb781211d427b',
    'smart_display/picture_store.h': 'e2fd997b9f1ceb51127b72b50cbd227d6d04471d5307f42943d6691ae85c6f80',
    'lvgl/__init__.py': '3b27c6ca820e20847713957b429beed4e51ca08609fd6c76a47a0af3d8308fdb',
    'lvgl/lvgl_esphome.cpp': 'c08d3c98795c84909d542d55342293f8117f4fbcbb1ddd5711eea48ed477b6df',
}

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("component_root", type=Path)
args = parser.parse_args()
root = args.component_root.resolve()
for name in ("smart_display", "lvgl"):
    if not (root / name).is_dir():
        parser.error(f"missing component directory: {root / name}")
for relative, expected in EXPECTED_SHA256.items():
    source = root / relative
    if not source.is_file() or hashlib.sha256(source.read_bytes()).hexdigest() != expected:
        parser.error(f"component differs from pinned source: {source}")
patch = Path(__file__).resolve().with_name("wads-p4-rgb888-components.patch")
# A dry run prevents applying a partly compatible patch to the component copy.
command = ["patch", "--batch", "--forward", "-F", "0", "-p1", "-d", str(root), "-i", str(patch)]
subprocess.run(command + ["--dry-run"], check=True)
subprocess.run(command, check=True)
