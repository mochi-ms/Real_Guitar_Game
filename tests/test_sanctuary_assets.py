import tempfile
import unittest
from pathlib import Path
import struct

from tools.download_sanctuary_pack import extract_texture_download_plan
from tools.generate_material_textures import generate_sanctuary_material_textures
from tools.generate_sanctuary_props_glb import (
    build_all_custom_sanctuary_glbs,
    terrain_height_m,
)


class TestSanctuaryAssets(unittest.TestCase):
    def test_extract_texture_download_plan(self) -> None:
        payload = {
            "Diffuse": {"2k": {"jpg": {"url": "https://ex/diff.jpg", "md5": "d1"}}},
            "nor_dx": {"2k": {"png": {"url": "https://ex/nor.png", "md5": "n1"}}},
            "Rough": {"2k": {"png": {"url": "https://ex/rgh.png", "md5": "r1"}}},
            "AO": {"2k": {"jpg": {"url": "https://ex/ao.jpg", "md5": "a1"}}},
        }
        plan = extract_texture_download_plan("forest_ground_04", payload)
        self.assertEqual(len(plan), 4)
        self.assertEqual(plan[0][0], "forest_ground_04_diff_2k.jpg")
        self.assertEqual(plan[1][0], "forest_ground_04_nor_dx_2k.png")

    def test_terrain_sculpt_has_clearing_lake_and_peaks(self) -> None:
        clearing_y = terrain_height_m(-2.0, 0.0)
        lake_y = terrain_height_m(22.0, 0.0)
        peak_y = terrain_height_m(-110.0, 60.0)
        self.assertGreater(clearing_y, 0.05)
        self.assertLess(lake_y, -1.0)
        self.assertGreater(peak_y, 12.0)

    def test_material_textures_and_custom_glbs(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            tex_paths = generate_sanctuary_material_textures(root / "tex")
            self.assertEqual(len(tex_paths), 3)
            for p in tex_paths:
                data = p.read_bytes()
                self.assertTrue(data.startswith(b"\x89PNG\r\n\x1a\n"))

            glb_paths = build_all_custom_sanctuary_glbs(root / "glb")
            self.assertEqual(len(glb_paths), 4)
            for gp in glb_paths:
                raw = gp.read_bytes()
                magic, version, length = struct.unpack("<III", raw[:12])
                self.assertEqual(magic, 0x46546C67)
                self.assertEqual(version, 2)
                self.assertEqual(length, len(raw))


if __name__ == "__main__":
    unittest.main()
