import struct
from pathlib import Path
import tempfile
import unittest
import zlib

from tools.generate_ui_textures import generate_ui_texture_pack


class TestUITextures(unittest.TestCase):
    def test_generate_valid_png_icons_and_panel(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            out_dir = Path(tmp)
            paths = generate_ui_texture_pack(out_dir)
            self.assertEqual(len(paths), 11)

            for path in paths:
                raw = path.read_bytes()
                self.assertEqual(raw[:8], b"\x89PNG\r\n\x1a\n", msg=f"{path.name} invalid PNG signature")
                self.assertEqual(raw[12:16], b"IHDR")
                width, height, bit_depth, color_type = struct.unpack(">IIBB", raw[16:26])
                self.assertEqual(bit_depth, 8)
                self.assertEqual(color_type, 6)  # RGBA
                expected_dim = 256 if "9Slice" in path.name else 128
                self.assertEqual((width, height), (expected_dim, expected_dim))

                # Extract IDAT chunk and confirm non-empty opaque/translucent pixels exist
                pos = 8
                idat_parts: list[bytes] = []
                while pos < len(raw):
                    length, tag = struct.unpack(">I4s", raw[pos : pos + 8])
                    data = raw[pos + 8 : pos + 8 + length]
                    if tag == b"IDAT":
                        idat_parts.append(data)
                    pos += 12 + length
                decompressed = zlib.decompress(b"".join(idat_parts))
                stride = 1 + width * 4
                self.assertEqual(len(decompressed), stride * height)
                alphas = [decompressed[r * stride + 1 + c * 4 + 3] for r in range(height) for c in range(width)]
                self.assertGreater(max(alphas), 200, msg=f"{path.name} has empty alpha channel")
                self.assertLess(min(alphas), 10, msg=f"{path.name} has no transparent background margin")


if __name__ == "__main__":
    unittest.main()
