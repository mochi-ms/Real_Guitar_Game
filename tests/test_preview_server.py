import json
import threading
import unittest
from urllib.request import Request, urlopen
from http.server import ThreadingHTTPServer

from engine.preview_server import PreviewHandler


class TestPreviewServer(unittest.TestCase):
    def test_preview_endpoints(self) -> None:
        srv = ThreadingHTTPServer(("127.0.0.1", 0), PreviewHandler)
        port = srv.server_address[1]
        thread = threading.Thread(target=srv.serve_forever, daemon=True)
        thread.start()
        try:
            base = f"http://127.0.0.1:{port}"
            with urlopen(f"{base}/") as resp:
                self.assertEqual(resp.status, 200)
                self.assertIn(b"RealGuitarGame", resp.read())

            with urlopen(f"{base}/api/samples") as resp:
                samples = json.loads(resp.read().decode("utf-8"))["samples"]
                self.assertIn("fingerstyle_canon_intro.musicxml", samples)

            with urlopen(f"{base}/api/compile?file=acoustic_ballad_strum.rgchart") as resp:
                perf = json.loads(resp.read().decode("utf-8"))
                self.assertEqual(perf["source_format"], "rgchart")
                self.assertGreater(len(perf["events"]), 20)

            req = Request(
                f"{base}/api/compile-custom",
                data=json.dumps({
                    "title": "API Test",
                    "bpm": 90,
                    "style": "fingerstyle",
                    "pattern": "travis_pick",
                    "measures": ["C", "G"],
                }).encode("utf-8"),
                headers={"Content-Type": "application/json"},
                method="POST",
            )
            with urlopen(req) as resp:
                custom = json.loads(resp.read().decode("utf-8"))
                self.assertEqual(custom["play_style"], "fingerstyle")
                self.assertGreater(len(custom["events"]), 10)
        finally:
            srv.shutdown()
            srv.server_close()


if __name__ == "__main__":
    unittest.main()
