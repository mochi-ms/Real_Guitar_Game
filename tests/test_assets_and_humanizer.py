import json
from pathlib import Path
import struct
import tempfile
import unittest

from engine.humanizer import humanize_performance
from engine.parsers.chord_chart_parser import parse_chord_chart_data
from engine.soundbank import export_ue5_multisample_bank
from tools.generate_guitar_glb import export_acoustic_guitar_combined_glb, export_acoustic_guitar_glb


class TestAssetsAndHumanizer(unittest.TestCase):
    def test_humanizer_preserves_order_and_valid_ranges(self) -> None:
        perf = parse_chord_chart_data(
            {
                "title": "Humanizer Test",
                "bpm": 80,
                "style": "fingerstyle",
                "pattern": "pima_ballad",
                "measures": ["C", "Am"],
            }
        )
        humanized = humanize_performance(perf, seed=7)
        self.assertGreater(len(humanized.events), 10)
        for ev in humanized.events:
            self.assertGreaterEqual(ev.velocity, 0.12)
            self.assertLessEqual(ev.velocity, 1.0)
            self.assertGreaterEqual(ev.time_sec, 0.0)

    def test_export_glb_has_valid_gltf2_header_and_strings(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            glb_path = Path(tmp) / "guitar.glb"
            export_acoustic_guitar_glb(glb_path)
            raw = glb_path.read_bytes()
            magic, version, total_len = struct.unpack("<III", raw[:12])
            self.assertEqual(magic, 0x46546C67)  # 'glTF'
            self.assertEqual(version, 2)
            self.assertEqual(total_len, len(raw))

            json_len, json_type = struct.unpack("<II", raw[12:20])
            self.assertEqual(json_type, 0x4E4F534A)  # 'JSON'
            gltf = json.loads(raw[20 : 20 + json_len].decode("utf-8"))
            mesh_names = {m["name"] for m in gltf["meshes"]}
            for s in range(1, 7):
                self.assertIn(f"String_{s}", mesh_names)
            self.assertIn("FretWires", mesh_names)
            self.assertIn("GuitarBody_Top", mesh_names)

            full_path = Path(tmp) / "guitar_full.glb"
            export_acoustic_guitar_combined_glb(full_path)
            raw_full = full_path.read_bytes()
            full_json_len, _ = struct.unpack("<II", raw_full[12:20])
            full_gltf = json.loads(raw_full[20 : 20 + full_json_len].decode("utf-8"))
            self.assertEqual(len(full_gltf["meshes"]), 1)
            self.assertEqual(full_gltf["meshes"][0]["name"], "AcousticGuitar_Full")
            self.assertGreaterEqual(len(full_gltf["meshes"][0]["primitives"]), 15)

    def test_multisample_bank_generation_small_range(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            map_path = export_ue5_multisample_bank(tmp, min_midi=40, max_midi=44, step=2, sample_rate=22050)
            data = json.loads(map_path.read_text(encoding="utf-8"))
            self.assertEqual(len(data["zones"]), 6)  # 3 notes x 2 layers
            for z in data["zones"]:
                self.assertTrue((Path(tmp) / z["wav_file"]).is_file())


if __name__ == "__main__":
    unittest.main()
