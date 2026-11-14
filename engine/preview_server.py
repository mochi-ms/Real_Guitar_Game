"""Local HTTP preview server for testing guitar scores, fingering animation, and acoustic WAV synthesis."""

from __future__ import annotations

import argparse
import base64
from http import HTTPStatus
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json
from pathlib import Path
import tempfile
from urllib.parse import parse_qs, urlparse

from engine.audio_synth import synthesize_performance_to_wav
from engine.compiler import compile_score
from engine.humanizer import humanize_performance
from engine.models import GuitarPerformance
from engine.parsers.chord_chart_parser import (
    FINGERSTYLE_PRESETS,
    STRUM_PRESETS,
    parse_chord_chart_data,
)

REPO_ROOT = Path(__file__).resolve().parent.parent
SAMPLES_DIR = REPO_ROOT / "samples"
PREVIEW_HTML = REPO_ROOT / "preview" / "index.html"
GUITAR_GLB = REPO_ROOT / "ue5" / "RealGuitarGame" / "Content" / "Models" / "AcousticGuitar_Reference.glb"


def _resolve_sample_path(name: str) -> Path:
    candidate = (SAMPLES_DIR / name).resolve()
    if candidate.parent != SAMPLES_DIR.resolve() or not candidate.is_file():
        raise FileNotFoundError(f"Sample not found: {name}")
    return candidate


class PreviewHandler(BaseHTTPRequestHandler):
    server_version = "RealGuitarGamePreview/1.1"

    def log_message(self, format: str, *args: object) -> None:  # noqa: A002
        return

    def _send_json(self, payload: object, status: int = HTTPStatus.OK) -> None:
        raw = json.dumps(payload, ensure_ascii=False).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(raw)))
        self.end_headers()
        self.wfile.write(raw)

    def _send_bytes(self, raw: bytes, content_type: str, status: int = HTTPStatus.OK) -> None:
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(raw)))
        self.end_headers()
        self.wfile.write(raw)

    def do_GET(self) -> None:  # noqa: N802
        parsed = urlparse(self.path)
        if parsed.path == "/":
            html = PREVIEW_HTML.read_bytes()
            self._send_bytes(html, "text/html; charset=utf-8")
            return

        if parsed.path == "/api/presets":
            self._send_json(
                {
                    "strum": list(STRUM_PRESETS.keys()),
                    "fingerstyle": list(FINGERSTYLE_PRESETS.keys()),
                }
            )
            return

        if parsed.path == "/api/samples":
            exts = {".musicxml", ".xml", ".mxl", ".gp", ".gpif", ".gp3", ".gp4", ".gp5", ".mid", ".midi", ".rgchart"}
            files = sorted(
                p.name for p in SAMPLES_DIR.iterdir() if p.is_file() and p.suffix.lower() in exts
            )
            self._send_json({"samples": files})
            return

        if parsed.path == "/api/compile":
            qs = parse_qs(parsed.query)
            name = qs.get("file", [""])[0]
            try:
                sample_path = _resolve_sample_path(name)
                perf = humanize_performance(compile_score(sample_path))
                self._send_json(perf.to_dict())
            except Exception as exc:  # noqa: BLE001
                self._send_json({"error": str(exc)}, HTTPStatus.BAD_REQUEST)
            return

        if parsed.path == "/api/wav":
            qs = parse_qs(parsed.query)
            name = qs.get("file", [""])[0]
            try:
                sample_path = _resolve_sample_path(name)
                perf = humanize_performance(compile_score(sample_path))
                with tempfile.NamedTemporaryFile(suffix=".wav", delete=False) as tf:
                    tmp_wav = Path(tf.name)
                try:
                    synthesize_performance_to_wav(perf, tmp_wav, sample_rate=44100)
                    wav_bytes = tmp_wav.read_bytes()
                finally:
                    tmp_wav.unlink(missing_ok=True)
                self._send_bytes(wav_bytes, "audio/wav")
            except Exception as exc:  # noqa: BLE001
                self._send_json({"error": str(exc)}, HTTPStatus.BAD_REQUEST)
            return

        if parsed.path == "/api/model.glb" and GUITAR_GLB.is_file():
            self._send_bytes(GUITAR_GLB.read_bytes(), "model/gltf-binary")
            return

        self.send_error(HTTPStatus.NOT_FOUND)

    def do_POST(self) -> None:  # noqa: N802
        parsed = urlparse(self.path)
        length = int(self.headers.get("Content-Length", "0"))
        body = self.rfile.read(length) if length > 0 else b"{}"

        if parsed.path == "/api/compile-custom":
            try:
                payload = json.loads(body.decode("utf-8"))
                perf = humanize_performance(parse_chord_chart_data(payload))
                self._send_json(perf.to_dict())
            except Exception as exc:  # noqa: BLE001
                self._send_json({"error": str(exc)}, HTTPStatus.BAD_REQUEST)
            return

        if parsed.path == "/api/upload-score":
            try:
                payload = json.loads(body.decode("utf-8"))
                filename = Path(str(payload.get("filename", "score.musicxml"))).name
                raw_bytes = base64.b64decode(str(payload["content_base64"]))
                suffix = Path(filename).suffix.lower()
                with tempfile.NamedTemporaryFile(suffix=suffix, delete=False) as tf:
                    tmp_score = Path(tf.name)
                try:
                    tmp_score.write_bytes(raw_bytes)
                    if suffix == ".json":
                        data = json.loads(raw_bytes.decode("utf-8"))
                        if "events" in data and "schema_version" in data:
                            perf = GuitarPerformance.from_dict(data)
                        else:
                            perf = humanize_performance(parse_chord_chart_data(data))
                    else:
                        perf = humanize_performance(compile_score(tmp_score))
                finally:
                    tmp_score.unlink(missing_ok=True)
                self._send_json(perf.to_dict())
            except Exception as exc:  # noqa: BLE001
                self._send_json({"error": str(exc)}, HTTPStatus.BAD_REQUEST)
            return

        if parsed.path == "/api/render-wav":
            try:
                payload = json.loads(body.decode("utf-8"))
                perf = GuitarPerformance.from_dict(payload)
                with tempfile.NamedTemporaryFile(suffix=".wav", delete=False) as tf:
                    tmp_wav = Path(tf.name)
                try:
                    synthesize_performance_to_wav(perf, tmp_wav, sample_rate=44100)
                    wav_bytes = tmp_wav.read_bytes()
                finally:
                    tmp_wav.unlink(missing_ok=True)
                self._send_bytes(wav_bytes, "audio/wav")
            except Exception as exc:  # noqa: BLE001
                self._send_json({"error": str(exc)}, HTTPStatus.BAD_REQUEST)
            return

        self.send_error(HTTPStatus.NOT_FOUND)


def main() -> None:
    parser = argparse.ArgumentParser(description="Start RealGuitarGame score preview server.")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8765)
    args = parser.parse_args()

    srv = ThreadingHTTPServer((args.host, args.port), PreviewHandler)
    print(f"RealGuitarGame Preview Server running at http://{args.host}:{args.port}")
    try:
        srv.serve_forever()
    except KeyboardInterrupt:
        print("\nShutting down preview server.")
    finally:
        srv.server_close()


if __name__ == "__main__":
    main()
