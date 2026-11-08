"""Run against an isolated Tessera 0.4.97 checkout with picture-paint.patch applied."""
import io
import sys
from pathlib import Path
import unittest

from PIL import Image, JpegImagePlugin

sys.path.insert(0, str(Path(sys.argv.pop(1)) / "screen_manager" / "app"))
from picture_paint import paint  # noqa: E402
from picture_question import Owner, Place, Question  # noqa: E402


class PhotoQuality(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        image = Image.new("RGB", (720, 720))
        image.putdata([((x * 255) // 719, (y * 255) // 719, ((x + y) * 255) // 1438)
                       for y in range(720) for x in range(720)])
        raw = io.BytesIO()
        image.save(raw, "PNG")
        cls.raw = raw.getvalue()

    def encoded(self, owner, size):
        question = Question(Owner(owner), (None,), size, (Place(0, 0, *size, fit="exact"),), formats=("jpeg",))
        return paint(question, [self.raw])

    def test_p4_saver_uses_high_quality_baseline_jpeg(self):
        body = self.encoded("saver", (720, 720))
        with Image.open(io.BytesIO(body)) as image:
            self.assertEqual(image.size, (720, 720))
            self.assertEqual(JpegImagePlugin.get_sampling(image), 0)  # 4:4:4
            self.assertEqual(image.quantization[0][0], 2)  # quality 95
            self.assertFalse(image.info.get("progressive", False))

    def test_other_sizes_and_camera_views_keep_original_encoding(self):
        for owner, size in (("saver", (480, 480)), ("view", (720, 720)), ("tile", (720, 720))):
            with self.subTest(owner=owner, size=size):
                with Image.open(io.BytesIO(self.encoded(owner, size))) as image:
                    self.assertEqual(JpegImagePlugin.get_sampling(image), 2)  # 4:2:0
                    self.assertNotEqual(image.quantization[0][0], 2)


if __name__ == "__main__":
    unittest.main()
