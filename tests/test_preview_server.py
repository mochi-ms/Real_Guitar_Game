import base64
import io
import json
from pathlib import Path
import unittest

from engine.preview_server import PreviewHandler


class _FakeSocket:
    def __init__(self, raw_request: bytes) -> None:
        self._rfile = io.BytesIO(raw_request)
        self._wfile = io.BytesIO()

    def makefile(self, mode: str, *args: object, **kwargs: object) -> io.BytesIO:
        return self._rfile if "r" in mode else self._wfile

    def sendall(self, data: bytes) -> None:
        self._wfile.write(data)


def _dispatch(method: str, path: str, body: bytes | None = None) -> tuple[int, dict[str, str], bytes]:
    headers = f"Host: 127.0.0.1\r\n"
    if body is not None:
        headers += f"Content-Type: application/json\r\nContent-Length: {len(body)}\r\n"
    raw = f"{method} {path} HTTP/1.1\r\n{headers}\r\n".encode("ascii") + (body or b"")
    sock = _FakeSocket(raw)
    PreviewHandler(sock, ("127.0.0.1", 0), None)  # type: ignore[arg-type]
    raw_resp = sock._wfile.getvalue()
    head_bytes, resp_body = raw_resp.split(b"\r\n\r\n", 1)
    lines = head_bytes.decode("iso-8859-1").split("\r\n")
    status_code = int(lines[0].split()[1])
    resp_headers = {}
    for line in lines[1:]:
        if ":" in line:
            k, v = line.split(":", 1)
            resp_headers[k.strip().lower()] = v.strip()
    return status_code, resp_headers, resp_body


class TestPreviewServer(unittest.TestCase):
    def test_preview_endpoints(self) -> None:
        status, _, body = _dispatch("GET", "/")
        self.assertEqual(status, 200)
        self.assertIn(b"RealGuitarGame", body)

        status, _, body = _dispatch("GET", "/api/samples")
        self.assertEqual(status, 200)
        samples = json.loads(body.decode("utf-8"))["samples"]
        self.assertIn("fingerstyle_canon_intro.musicxml", samples)

        status, _, body = _dispatch("GET", "/api/compile?file=acoustic_ballad_strum.rgchart")
        self.assertEqual(status, 200)
        perf = json.loads(body.decode("utf-8"))
        self.assertEqual(perf["source_format"], "rgchart")
        self.assertGreater(len(perf["events"]), 20)

        custom_payload = json.dumps(
            {
                "title": "API Test",
                "bpm": 90,
                "style": "fingerstyle",
                "pattern": "travis_pick",
                "measures": ["C", "G"],
            }
        ).encode("utf-8")
        status, _, body = _dispatch("POST", "/api/compile-custom", custom_payload)
        self.assertEqual(status, 200)
        custom = json.loads(body.decode("utf-8"))
        self.assertEqual(custom["play_style"], "fingerstyle")
        self.assertGreater(len(custom["events"]), 10)

        sample_xml = (
            Path(__file__).resolve().parent.parent / "samples" / "fingerstyle_canon_intro.musicxml"
        ).read_bytes()
        upload_payload = json.dumps(
            {
                "filename": "fingerstyle_canon_intro.musicxml",
                "content_base64": base64.b64encode(sample_xml).decode("ascii"),
            }
        ).encode("utf-8")
        status, _, body = _dispatch("POST", "/api/upload-score", upload_payload)
        self.assertEqual(status, 200)
        uploaded = json.loads(body.decode("utf-8"))
        self.assertEqual(uploaded["source_format"], "musicxml")
        self.assertGreater(len(uploaded["events"]), 25)


if __name__ == "__main__":
    unittest.main()
