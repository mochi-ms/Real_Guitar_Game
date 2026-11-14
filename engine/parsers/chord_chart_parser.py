"""Parser for .rgchart chord progression + strum/fingerstyle pattern files."""

from __future__ import annotations

import json
from pathlib import Path
import re
from typing import Any

from engine.fingering_solver import assign_left_fingers_for_shape
from engine.models import (
    STANDARD_TUNING_MIDI,
    ChordSnapshot,
    GuitarEvent,
    GuitarPerformance,
    RightAction,
    TimeSignature,
    fret_to_midi,
)


def _lh(shape_low_to_high: tuple[int, int, int, int, int, int]) -> list[int]:
    """Convert low-E..high-E chord shape (6..1) into internal string 1..6 order."""
    return list(reversed(shape_low_to_high))


# Standard voicings written naturally from Low E (string 6) to High E (string 1), -1 = muted
_CHORD_SHAPES_LOW_TO_HIGH: dict[str, tuple[int, int, int, int, int, int]] = {
    "C": (-1, 3, 2, 0, 1, 0),
    "Cmaj7": (-1, 3, 2, 0, 0, 0),
    "Cadd9": (-1, 3, 2, 0, 3, 0),
    "C7": (-1, 3, 2, 3, 1, 0),
    "Cm": (-1, 3, 5, 5, 4, 3),
    "D": (-1, -1, 0, 2, 3, 2),
    "Dm": (-1, -1, 0, 2, 3, 1),
    "Dm7": (-1, -1, 0, 2, 1, 1),
    "D7": (-1, -1, 0, 2, 1, 2),
    "Dsus2": (-1, -1, 0, 2, 3, 0),
    "Dsus4": (-1, -1, 0, 2, 3, 3),
    "D/F#": (2, -1, 0, 2, 3, 2),
    "E": (0, 2, 2, 1, 0, 0),
    "Em": (0, 2, 2, 0, 0, 0),
    "Em7": (0, 2, 2, 0, 3, 0),
    "E7": (0, 2, 0, 1, 0, 0),
    "F": (1, 3, 3, 2, 1, 1),
    "Fmaj7": (-1, -1, 3, 2, 1, 0),
    "Fm": (1, 3, 3, 1, 1, 1),
    "G": (3, 2, 0, 0, 0, 3),
    "G6": (3, 2, 0, 0, 0, 0),
    "G7": (3, 2, 0, 0, 0, 1),
    "G/B": (-1, 2, 0, 0, 3, 3),
    "A": (-1, 0, 2, 2, 2, 0),
    "Am": (-1, 0, 2, 2, 1, 0),
    "Am7": (-1, 0, 2, 0, 1, 0),
    "A7": (-1, 0, 2, 0, 2, 0),
    "Asus2": (-1, 0, 2, 2, 0, 0),
    "Asus4": (-1, 0, 2, 2, 3, 0),
    "B": (-1, 2, 4, 4, 4, 2),
    "Bm": (-1, 2, 4, 4, 3, 2),
    "Bm7": (-1, 2, 4, 2, 3, 2),
    "B7": (-1, 2, 1, 2, 0, 2),
    "Bb": (-1, 1, 3, 3, 3, 1),
    "Bbm": (-1, 1, 3, 3, 2, 1),
    "Eb": (-1, -1, 1, 3, 4, 3),
    "F#m": (2, 4, 4, 2, 2, 2),
    "C#m": (-1, 4, 6, 6, 5, 4),
    "G#m": (4, 6, 6, 4, 4, 4),
}

_SEMITONES = {
    "C": 0, "C#": 1, "Db": 1, "D": 2, "D#": 3, "Eb": 3,
    "E": 4, "F": 5, "F#": 6, "Gb": 6, "G": 7, "G#": 8,
    "Ab": 8, "A": 9, "A#": 10, "Bb": 10, "B": 11,
}

STRUM_PRESETS: dict[str, dict[str, Any]] = {
    "8beat_folk": {"subdivision": 2, "steps": ["D", "-", "D", "U", "-", "U", "D", "U"]},
    "16beat_ballad": {
        "subdivision": 4,
        "steps": ["D", "-", "-", "-", "D", "-", "D", "U", "-", "-", "U", "-", "D", "-", "D", "U"],
    },
    "pop_rock": {"subdivision": 2, "steps": ["D", "-", "D", "U", "X", "U", "D", "U"]},
    "island_strum": {"subdivision": 2, "steps": ["D", "-", "D", "U", "-", "U", "D", "U"]},
    "waltz_3_4": {"subdivision": 2, "steps": ["B", "-", "D", "U", "D", "U"]},
}

FINGERSTYLE_PRESETS: dict[str, dict[str, Any]] = {
    "pima_ballad": {"subdivision": 2, "steps": ["P", "i", "m", "a", "m", "i", "m", "i"]},
    "folk_arp_8": {"subdivision": 2, "steps": ["P", "i", "m", "i", "a", "m", "i", "m"]},
    "travis_pick": {"subdivision": 2, "steps": ["P+a", "i", "P_alt", "m", "P", "a", "P_alt", "i"]},
    "waltz_arp_3_4": {"subdivision": 2, "steps": ["P", "i", "m", "a", "m", "i"]},
    "romanza_triplet": {"subdivision": 3, "steps": ["P+a", "m", "i", "a", "m", "i", "a", "m", "i", "a", "m", "i"]},
}


def resolve_chord_shape(symbol: str) -> list[int]:
    """Resolve a chord symbol into a 6-element fret array ordered string 1..6."""
    clean = symbol.strip()
    if clean in _CHORD_SHAPES_LOW_TO_HIGH:
        return _lh(_CHORD_SHAPES_LOW_TO_HIGH[clean])

    # Fallback: synthesize E-shape or A-shape movable barre chord
    match = re.match(r"^([A-G][#b]?)(m7|maj7|m|7)?(?:/.*)?$", clean)
    if not match:
        raise ValueError(f"Unknown chord symbol: {symbol!r}")
    root, quality = match.group(1), match.group(2) or ""
    root_pc = _SEMITONES[root]

    # Prefer E-shape (low E root = pc 4) if fret <= 7, else A-shape (A string root = pc 9)
    e_fret = (root_pc - 4) % 12
    a_fret = (root_pc - 9) % 12
    if e_fret <= 7:
        base_map = {
            "": (0, 2, 2, 1, 0, 0),
            "m": (0, 2, 2, 0, 0, 0),
            "7": (0, 2, 0, 1, 0, 0),
            "m7": (0, 2, 0, 0, 0, 0),
            "maj7": (0, 2, 1, 1, 0, 0),
        }
        base = base_map[quality]
        return _lh(tuple(f + e_fret for f in base))  # type: ignore[arg-type]

    base_map_a = {
        "": (-1, 0, 2, 2, 2, 0),
        "m": (-1, 0, 2, 2, 1, 0),
        "7": (-1, 0, 2, 0, 2, 0),
        "m7": (-1, 0, 2, 0, 1, 0),
        "maj7": (-1, 0, 2, 1, 2, 0),
    }
    base = base_map_a[quality]
    return _lh(tuple(-1 if f < 0 else f + a_fret for f in base))  # type: ignore[arg-type]


def _parse_pattern_steps(raw_pattern: str | list[str]) -> list[str]:
    if isinstance(raw_pattern, list):
        return [str(tok).strip() for tok in raw_pattern if str(tok).strip()]
    return [tok.strip() for tok in raw_pattern.strip().split() if tok.strip()]


def _bass_strings_for_shape(frets_1_to_6: list[int]) -> tuple[int, int]:
    """Return (primary_bass_string, alternating_bass_string) from active strings 6..1."""
    active_bass = [s for s in (6, 5, 4, 3) if frets_1_to_6[s - 1] >= 0]
    if not active_bass:
        active_bass = [s for s in (6, 5, 4, 3, 2, 1) if frets_1_to_6[s - 1] >= 0]
    primary = active_bass[0]
    alt = active_bass[1] if len(active_bass) > 1 else primary
    return primary, alt


def _pick_treble_string(role: str, frets_1_to_6: list[int]) -> tuple[int, RightAction]:
    if role == "i":
        candidates, action = (3, 4, 2, 1), "pluck_index"
    elif role == "m":
        candidates, action = (2, 3, 1, 4), "pluck_middle"
    else:  # 'a'
        candidates, action = (1, 2, 3, 4), "pluck_ring"
    for s in candidates:
        if frets_1_to_6[s - 1] >= 0:
            return s, action  # type: ignore[return-value]
    return 1, action  # type: ignore[return-value]


def parse_chord_chart_data(data: dict[str, Any]) -> GuitarPerformance:
    """Compile an in-memory .rgchart dictionary into a GuitarPerformance timeline."""
    title = str(data.get("title", "Acoustic Pattern"))
    artist = str(data.get("artist", "RealGuitarGame"))
    bpm = float(data.get("bpm", 88.0))
    capo = int(data.get("capo", 0))
    ts_data = data.get("time_signature", {"numerator": 4, "denominator": 4})
    ts = TimeSignature(
        numerator=int(ts_data.get("numerator", 4)),
        denominator=int(ts_data.get("denominator", 4)),
    )
    tuning = list(data.get("tuning_midi", STANDARD_TUNING_MIDI))
    sec_per_beat = 60.0 / bpm

    perf = GuitarPerformance(
        title=title,
        artist=artist,
        source_format="rgchart",
        bpm=bpm,
        time_signature=ts,
        capo=capo,
        tuning_midi=tuning,
    )

    sections = data.get("sections")
    if sections is None:
        # Allow flat top-level chart syntax
        sections = [
            {
                "name": "Main",
                "style": data.get("style", "strum"),
                "pattern": data.get("pattern", "8beat_folk"),
                "subdivision": data.get("subdivision"),
                "measures": data.get("measures", []),
            }
        ]

    current_beat = 0.0
    strum_group_counter = 0

    for sec in sections:
        style = str(sec.get("style", "strum")).lower()
        pattern_spec = sec.get("pattern", "8beat_folk" if style == "strum" else "pima_ballad")

        if isinstance(pattern_spec, str) and pattern_spec in STRUM_PRESETS:
            preset = STRUM_PRESETS[pattern_spec]
            steps = list(preset["steps"])
            subdiv = int(sec.get("subdivision") or preset["subdivision"])
            style = "strum"
        elif isinstance(pattern_spec, str) and pattern_spec in FINGERSTYLE_PRESETS:
            preset = FINGERSTYLE_PRESETS[pattern_spec]
            steps = list(preset["steps"])
            subdiv = int(sec.get("subdivision") or preset["subdivision"])
            style = "fingerstyle"
        else:
            steps = _parse_pattern_steps(pattern_spec)
            subdiv = int(sec.get("subdivision") or max(1, round(len(steps) / ts.numerator)))

        step_beats = 1.0 / subdiv
        step_sec = step_beats * sec_per_beat

        for measure in sec.get("measures", []):
            # Measure can be a string ("C") or list of chords (["C", "G"]) or object
            if isinstance(measure, str):
                measure_chords = [{"chord": measure, "beats": ts.numerator}]
            elif isinstance(measure, list):
                per_chord_beats = ts.numerator / max(1, len(measure))
                measure_chords = [{"chord": c, "beats": per_chord_beats} for c in measure]
            elif isinstance(measure, dict) and "chords" in measure:
                measure_chords = measure["chords"]
            else:
                measure_chords = [measure]

            measure_start_beat = current_beat
            total_steps = ts.numerator * subdiv

            # Build chord lookup across steps in this measure
            chord_segments: list[tuple[float, float, str, list[int], list[int]]] = []
            seg_cursor = measure_start_beat
            for entry in measure_chords:
                if isinstance(entry, str):
                    symbol, beats = entry, float(ts.numerator) / len(measure_chords)
                    custom_frets = None
                else:
                    symbol = str(entry["chord"])
                    beats = float(entry.get("beats", ts.numerator))
                    custom_frets = entry.get("frets")
                frets = list(custom_frets) if custom_frets else resolve_chord_shape(symbol)
                fingers = assign_left_fingers_for_shape(frets)
                start_s = seg_cursor * sec_per_beat
                dur_s = beats * sec_per_beat
                perf.chords.append(
                    ChordSnapshot(
                        time_sec=round(start_s, 5),
                        duration_sec=round(dur_s, 5),
                        symbol=symbol,
                        frets=frets,
                        fingers=fingers,
                    )
                )
                chord_segments.append((seg_cursor, seg_cursor + beats, symbol, frets, fingers))
                seg_cursor += beats

            for step_idx in range(total_steps):
                token = steps[step_idx % len(steps)]
                if token in ("-", ".", "_"):
                    continue

                b_pos = measure_start_beat + step_idx * step_beats
                t_pos = b_pos * sec_per_beat

                # Find active chord at this beat
                active = chord_segments[-1]
                for seg in chord_segments:
                    if seg[0] - 1e-6 <= b_pos < seg[1] - 1e-6:
                        active = seg
                        break
                _, seg_end_beat, symbol, frets, fingers = active
                remaining_sec = max(step_sec, (seg_end_beat - b_pos) * sec_per_beat)

                if style == "strum" or token in ("D", "U", "X", "B"):
                    if token == "X":
                        # Muted percussive chuck across middle strings
                        strum_group_counter += 1
                        for idx_s, s_num in enumerate((4, 3, 2)):
                            fret_val = max(0, frets[s_num - 1])
                            perf.events.append(
                                GuitarEvent(
                                    id=0,
                                    time_sec=t_pos + idx_s * 0.004,
                                    duration_sec=min(0.08, step_sec * 0.8),
                                    beat=b_pos,
                                    string=s_num,
                                    fret=fret_val,
                                    midi_note=fret_to_midi(s_num, fret_val, tuning, capo),
                                    velocity=0.55,
                                    left_finger=fingers[s_num - 1],
                                    right_action="percussive_slap",
                                    articulation="dead_note",
                                    strum_group_id=strum_group_counter,
                                    chord_symbol=symbol,
                                )
                            )
                    elif token == "B":
                        bass_s, _ = _bass_strings_for_shape(frets)
                        fret_val = max(0, frets[bass_s - 1])
                        perf.events.append(
                            GuitarEvent(
                                id=0,
                                time_sec=t_pos,
                                duration_sec=remaining_sec,
                                beat=b_pos,
                                string=bass_s,
                                fret=fret_val,
                                midi_note=fret_to_midi(bass_s, fret_val, tuning, capo),
                                velocity=0.82,
                                left_finger=fingers[bass_s - 1],
                                right_action="pluck_thumb",
                                articulation="let_ring",
                                chord_symbol=symbol,
                            )
                        )
                    else:
                        is_down = token.upper() == "D"
                        active_strings = [s for s in (6, 5, 4, 3, 2, 1) if frets[s - 1] >= 0]
                        if is_down:
                            ordered = active_strings
                            right_act: RightAction = "strum_down"
                            spread = 0.0038
                            base_vel = 0.84 if (step_idx % subdiv == 0) else 0.70
                        else:
                            # Up-strums naturally brush top 3-4 strings softly
                            ordered = list(reversed(active_strings))[:4]
                            right_act = "strum_up"
                            spread = 0.0032
                            base_vel = 0.62

                        strum_group_counter += 1
                        for order_i, s_num in enumerate(ordered):
                            fret_val = frets[s_num - 1]
                            vel = max(0.35, min(1.0, base_vel - order_i * 0.02))
                            perf.events.append(
                                GuitarEvent(
                                    id=0,
                                    time_sec=t_pos + order_i * spread,
                                    duration_sec=remaining_sec,
                                    beat=b_pos,
                                    string=s_num,
                                    fret=fret_val,
                                    midi_note=fret_to_midi(s_num, fret_val, tuning, capo),
                                    velocity=vel,
                                    left_finger=fingers[s_num - 1],
                                    right_action=right_act,
                                    articulation="let_ring",
                                    strum_group_id=strum_group_counter,
                                    chord_symbol=symbol,
                                )
                            )
                else:
                    # Fingerstyle / arpeggio token (can combine with '+', e.g. 'P+a')
                    sub_tokens = [p.strip() for p in token.split("+") if p.strip()]
                    primary_bass, alt_bass = _bass_strings_for_shape(frets)
                    for sub_tok in sub_tokens:
                        if sub_tok in ("P", "p"):
                            s_num, r_act, vel = primary_bass, "pluck_thumb", 0.85
                        elif sub_tok in ("P_alt", "p2"):
                            s_num, r_act, vel = alt_bass, "pluck_thumb", 0.76
                        elif sub_tok in ("i", "m", "a"):
                            s_num, r_act = _pick_treble_string(sub_tok, frets)
                            vel = 0.74 if sub_tok == "a" else 0.66
                        elif sub_tok.isdigit() and 1 <= int(sub_tok) <= 6:
                            s_num = int(sub_tok)
                            r_act = "pluck_thumb" if s_num >= 4 else (
                                "pluck_index" if s_num == 3 else (
                                    "pluck_middle" if s_num == 2 else "pluck_ring"
                                )
                            )
                            vel = 0.72
                        else:
                            continue

                        fret_val = max(0, frets[s_num - 1])
                        perf.events.append(
                            GuitarEvent(
                                id=0,
                                time_sec=t_pos,
                                duration_sec=max(step_sec * 1.8, remaining_sec * 0.85),
                                beat=b_pos,
                                string=s_num,
                                fret=fret_val,
                                midi_note=fret_to_midi(s_num, fret_val, tuning, capo),
                                velocity=vel,
                                left_finger=fingers[s_num - 1],
                                right_action=r_act,  # type: ignore[arg-type]
                                articulation="let_ring",
                                chord_symbol=symbol,
                            )
                        )

            current_beat = measure_start_beat + ts.numerator

    return perf.finalize()


def parse_chord_chart_file(path: str | Path) -> GuitarPerformance:
    """Load and compile a .rgchart JSON file."""
    file_path = Path(path)
    data = json.loads(file_path.read_text(encoding="utf-8"))
    return parse_chord_chart_data(data)
