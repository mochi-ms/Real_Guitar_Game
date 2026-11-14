import unittest

from tools.download_cc0_assets import extract_gltf_download_plan


class TestCC0Downloader(unittest.TestCase):
    def test_extract_gltf_download_plan_selects_2k_and_includes(self) -> None:
        sample_payload = {
            "gltf": {
                "4k": {"gltf": {"url": "https://example.com/bench_4k.gltf", "md5": "4khash", "include": {}}},
                "2k": {
                    "gltf": {
                        "url": "https://example.com/painted_wooden_bench_2k.gltf",
                        "md5": "2khash",
                        "include": {
                            "textures/diff_2k.jpg": {"url": "https://example.com/diff_2k.jpg", "md5": "d1"},
                            "painted_wooden_bench.bin": {"url": "https://example.com/bench.bin", "md5": "b1"},
                        },
                    }
                },
            }
        }
        plan = extract_gltf_download_plan("painted_wooden_bench", sample_payload)
        self.assertEqual(plan[0], ("painted_wooden_bench_2k.gltf", "https://example.com/painted_wooden_bench_2k.gltf", "2khash"))
        self.assertEqual(len(plan), 3)
        self.assertIn(("textures/diff_2k.jpg", "https://example.com/diff_2k.jpg", "d1"), plan)


if __name__ == "__main__":
    unittest.main()
