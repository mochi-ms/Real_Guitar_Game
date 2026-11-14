"""Canonical data models for guitar scores and performance timelines."""

from __future__ import annotations

from dataclasses import asdict, dataclass, field
import json
from typing import Any, Literal

# Standard tuning from string 1 (High E4) to string 6 (Low E2)
STANDARD_TUNING_MIDI: tuple[int, ...] = (64, 59, 55, 50, 45, 40)

SourceFormat = Literal["musicxml", "guitar_pro", "midi", "rgchart"]
PlayStyle = Literal["fingerstyle", "strum", "hybrid"]
RightAction = Literal[
    "pluck_thumb",
    "pluck_index",
    "pluck_middle",
    "pluck_ring",
    "pick_down",
    "pick_up",
    "strum_down",
    "strum_up",
    "percussive_slap",
]
Articulation = Literal[
    "normal",
    "let_ring",
    "palm_mute",
    "dead_note",
    "hammer_on",
    "pull_off",
    "slide",
    "harmonic",
    "vibrato",
]

_NOTE_OFFSETS = {
    "C": 0,
    "D": 2,
    "E": 4,
    "F": 5,
    "G": 7,
    "A": 9,
    "B": 11,
}
_NOTE_NAMES = ("C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B")


def note_to_midi(step: str, alter: int = 0, octave: int = 4) -> int:
    """Convert pitch components (step, alter, octave) to a MIDI note number."""
    step_upper = step.strip().upper()
    if step_upper not in _NOTE_OFFSETS:
        raise ValueError(f"Unsupported pitch step: {step!r}")
    return (octave + 1) * 12 + _NOTE_OFFSETS[step_upper] + alter


def midi_to_note_name(midi_note: int) -> str:
    """Convert a MIDI note number to scientific pitch notation (e.g. 64 -> 'E4')."""
    octave = (midi_note // 12) - 1
    name = _NOTE_NAMES[midi_note % 12]
    return f"{name}{octave}"


def fret_to_midi(
    string_num: int,
    fret: int,
    tuning_midi: tuple[int, ...] | list[int] = STANDARD_TUNING_MIDI,
    capo: int = 0,
) -> int:
    """Calculate sounding MIDI note for a 1-indexed string (1=high E .. 6=low E) and fret."""
    if not 1 <= string_num <= 6:
        raise ValueError(f"string_num must be in 1..6, got {string_num}")
    if fret < 0 or fret > 24:
        raise ValueError(f"fret must be in 0..24, got {fret}")
    return int(tuning_midi[string_num - 1]) + capo + fret


@dataclass(slots=True)
class TimeSignature:
    numerator: int = 4
    denominator: int = 4


@dataclass(slots=True)
class ChordSnapshot:
    time_sec: float
    duration_sec: float
    symbol: str
    frets: list[int]  # length 6, string 1..6 (-1 = mute, 0 = open)
    fingers: list[int] = field(default_factory=lambda: [0, 0, 0, 0, 0, 0])


@dataclass(slots=True)
class GuitarEvent:
    id: int
    time_sec: float
    duration_sec: float
    string: int  # 1 (high E) .. 6 (low E)
    fret: int  # 0..24
    midi_note: int
    velocity: float  # 0.0 .. 1.0
    left_finger: int  # 0=open, 1=index, 2=middle, 3=ring, 4=pinky, 5=thumb
    right_action: RightAction
    articulation: Articulation = "normal"
    beat: float = 0.0
    strum_group_id: int | None = None
    chord_symbol: str | None = None


@dataclass(slots=True)
class GuitarPerformance:
    title: str
    source_format: SourceFormat
    bpm: float = 90.0
    artist: str = "Unknown"
    play_style: PlayStyle = "hybrid"
    time_signature: TimeSignature = field(default_factory=TimeSignature)
    capo: int = 0
    tuning_midi: list[int] = field(default_factory=lambda: list(STANDARD_TUNING_MIDI))
    duration_sec: float = 0.0
    chords: list[ChordSnapshot] = field(default_factory=list)
    events: list[GuitarEvent] = field(default_factory=list)
    schema_version: str = "1.0"

    def finalize(self) -> "GuitarPerformance":
        """Sort events chronologically, renumber IDs, and refresh play_style and duration."""
        self.events.sort(key=lambda e: (round(e.time_sec, 6), -e.string, e.fret))
        for idx, ev in enumerate(self.events):
            ev.id = idx
            ev.time_sec = round(ev.time_sec, 5)
            ev.duration_sec = round(max(0.02, ev.duration_sec), 5)
            ev.velocity = round(max(0.05, min(1.0, ev.velocity)), 4)
            ev.beat = round(ev.beat, 5)

        if self.events:
            max_end = max(e.time_sec + e.duration_sec for e in self.events)
            self.duration_sec = round(max(self.duration_sec, max_end), 4)

        has_strum = any(e.right_action in ("strum_down", "strum_up") for e in self.events)
        has_pluck = any(
            e.right_action in ("pluck_thumb", "pluck_index", "pluck_middle", "pluck_ring")
            for e in self.events
        )
        if has_strum and has_pluck:
            self.play_style = "hybrid"
        elif has_strum:
            self.play_style = "strum"
        else:
            self.play_style = "fingerstyle"
        return self

    def to_dict(self) -> dict[str, Any]:
        self.finalize()
        return {
            "schema_version": self.schema_version,
            "title": self.title,
            "artist": self.artist,
            "source_format": self.source_format,
            "play_style": self.play_style,
            "bpm": round(self.bpm, 3),
            "time_signature": asdict(self.time_signature),
            "capo": self.capo,
            "tuning_midi": list(self.tuning_midi),
            "duration_sec": self.duration_sec,
            "chords": [asdict(c) for c in self.chords],
            "events": [asdict(e) for e in self.events],
        }

    def to_json(self, indent: int = 2) -> str:
        return json.dumps(self.to_dict(), indent=indent, ensure_ascii=False)

    @classmethod
    def from_dict(cls, data: dict[str, Any]) -> "GuitarPerformance":
        ts = data.get("time_signature") or {}
        chords = [ChordSnapshot(**c) for c in data.get("chords", [])]
        events = [GuitarEvent(**e) for e in data.get("events", [])]
        return cls(
            schema_version=data.get("schema_version", "1.0"),
            title=data.get("title", "Untitled"),
            artist=data.get("artist", "Unknown"),
            source_format=data.get("source_format", "rgchart"),
            play_style=data.get("play_style", "hybrid"),
            bpm=float(data.get("bpm", 90.0)),
            time_signature=TimeSignature(
                numerator=int(ts.get("numerator", 4)),
                denominator=int(ts.get("denominator", 4)),
            ),
            capo=int(data.get("capo", 0)),
            tuning_midi=list(data.get("tuning_midi", STANDARD_TUNING_MIDI)),
            duration_sec=float(data.get("duration_sec", 0.0)),
            chords=chords,
            events=events,
        )
