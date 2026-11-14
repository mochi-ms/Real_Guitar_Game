"""Unified score compiler: converts MusicXML, Guitar Pro (.gp), MIDI, or .rgchart into GuitarPerformance JSON (+ optional WAV)."""

from __future__ import annotations

import argparse
from pathlib import Path
import sys

from engine.audio_synth import synthesize_performance_to_wav
from engine.models import GuitarPerformance
from engine.parsers.chord_chart_parser import parse_chord_chart_file
from engine.parsers.gp_parser import parse_guitar_pro_file
from engine.parsers.midi_parser import parse_midi_file
from engine.parsers.musicxml_parser import parse_musicxml_file


def compile_score(input_path: str | Path) -> GuitarPerformance:
    """Auto-detect file format and compile into a normalized GuitarPerformance."""
    path = Path(input_path)
    if not path.exists():
        raise FileNotFoundError(f"Score file not found: {path}")

    ext = path.suffix.lower()
    if ext in (".musicxml", ".xml", ".mxl"):
        return parse_musicxml_file(path)
    if ext in (".gp", ".gpif", ".gp3", ".gp4", ".gp5", ".gpx"):
        return parse_guitar_pro_file(path)
    if ext in (".mid", ".midi"):
        return parse_midi_file(path)
    if ext in (".rgchart", ".json"):
        return parse_chord_chart_file(path)

    raise ValueError(
        f"Unsupported score extension {ext!r}. Supported: .musicxml, .xml, .mxl, .gp, .gpif, .mid, .midi, .rgchart, .json"
    )


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Compile guitar TAB, MIDI, or chord charts into canonical GuitarPerformance JSON."
    )
    parser.add_argument("input", type=Path, help="Input score file (.musicxml, .gp, .mid, .rgchart)")
    parser.add_argument(
        "-o", "--output", type=Path, default=None, help="Output JSON path (default: output/<stem>.performance.json)"
    )
    parser.add_argument("--wav", action="store_true", help="Also synthesize an acoustic guitar WAV preview")
    parser.add_argument("--sample-rate", type=int, default=44100, help="WAV sample rate (default: 44100)")
    args = parser.parse_args(argv)

    perf = compile_score(args.input)
    out_json = args.output or (Path("output") / f"{args.input.stem}.performance.json")
    out_json.parent.mkdir(parents=True, exist_ok=True)
    out_json.write_text(perf.to_json(), encoding="utf-8")

    print(
        f"[OK] Compiled '{perf.title}' ({perf.source_format}, style={perf.play_style}, "
        f"bpm={perf.bpm}, events={len(perf.events)}, duration={perf.duration_sec:.2f}s) -> {out_json}"
    )

    if args.wav:
        out_wav = out_json.with_suffix(".wav")
        synthesize_performance_to_wav(perf, out_wav, sample_rate=args.sample_rate)
        print(f"[OK] Synthesized acoustic WAV -> {out_wav}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
