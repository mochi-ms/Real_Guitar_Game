from pathlib import Path
import struct
import tempfile
import unittest

from tools.generate_app_icon import ICON_SIZES, generate_windows_app_icon


class TestAppIcon(unittest.TestCase):
    def test_generate_windows_ico_multi_resolution(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            ico_path = Path(tmp) / "Application.ico"
            out = generate_windows_app_icon(ico_path)
            raw = out.read_bytes()
            reserved, ico_type, count = struct.unpack_from("<HHH", raw, 0)
            self.assertEqual(reserved, 0)
            self.assertEqual(ico_type, 1)
            self.assertEqual(count, len(ICON_SIZES))

            dims: list[int] = []
            for idx in range(count):
                w = raw[6 + idx * 16]
                dims.append(256 if w == 0 else w)
                offset = struct.unpack_from("<I", raw, 18 + idx * 16)[0]
                self.assertEqual(raw[offset : offset + 4], b"\x89PNG")
            self.assertEqual(tuple(dims), ICON_SIZES)


if __name__ == "__main__":
    unittest.main()
