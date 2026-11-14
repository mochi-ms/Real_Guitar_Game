"""Parser for Guitar Pro 7/8 (.gp) ZIP archives and GPIF (.gpif) XML files."""

from __future__ import annotations

from pathlib import Path
import xml.etree.ElementTree as ET
import zipfile

from engine.fingering_solver import assign_left_fingers_for_map, assign_right_hand_pluck
from engine.models import (
    STANDARD_TUNING_MIDI,
    Articulation,
    GuitarEvent,
    GuitarPerformance,
    RightAction,
    TimeSignature,
    fret_to_midi,
)

_NOTE_VALUE_BEATS = {
    "Whole": 4.0,
    "Half": 2.0,
    "Quarter": 1.0,
    "Eighth": 0.5,
    "16th": 0.25,
    "32nd": 0.125,
    "64th": 0.0625,
}


def _load_gpif_root(path: Path) -> ET.Element:
    suffix = path.suffix.lower()
    if suffix in (".gp3", ".gp4", ".gp5", ".gpx"):
        raise ValueError(
            f"Legacy binary Guitar Pro format ({suffix}) is not directly supported without conversion. "
            "Export as Guitar Pro 7/8 (.gp) or MusicXML (.musicxml/.mxl) first."
        )

    if suffix == ".gp":
        with zipfile.ZipFile(path, "r") as zf:
            candidates = [n for n in zf.namelist() if n.endswith("score.gpif")]
            if not candidates:
                raise ValueError(f"No Content/score.gpif found in Guitar Pro archive: {path}")
            return ET.fromstring(zf.read(candidates[0]))

    return ET.fromstring(path.read_bytes())


def _prop_dict(elem: ET.Element) -> dict[str, ET.Element]:
    props: dict[str, ET.Element] = {}
    props_node = elem.find("Properties")
    if props_node is None:
        return props
    for prop in props_node.findall("Property"):
        name = prop.attrib.get("name")
        if name:
            props[name] = prop
    return props


def _text(parent: ET.Element | None, path: str, default: str = "") -> str:
    if parent is None:
        return default
    node = parent.find(path)
    return node.text.strip() if node is not None and node.text else default


def _rhythm_beats(rhythm_elem: ET.Element) -> float:
    base = _NOTE_VALUE_BEATS.get(_text(rhythm_elem, "NoteValue", "Quarter"), 1.0)
    dots_node = rhythm_elem.find("AugmentationDot")
    if dots_node is not None:
        count = int(dots_node.attrib.get("count", "1"))
        if count == 1:
            base *= 1.5
        elif count == 2:
            base *= 1.75
    tuplet = rhythm_elem.find("PrimaryTuplet")
    if tuplet is not None:
        num = int(tuplet.attrib.get("num", "1"))
        den = int(tuplet.attrib.get("den", "1"))
        if num > 0:
            base *= den / num
    return base


def parse_guitar_pro_file(path: str | Path) -> GuitarPerformance:
    """Parse a Guitar Pro 7/8 (.gp) or .gpif file into GuitarPerformance."""
    file_path = Path(path)
    root = _load_gpif_root(file_path)

    title = _text(root, "Score/Title") or file_path.stem
    artist = _text(root, "Score/Artist") or "Unknown"

    # Initial tempo from MasterTrack automations
    bpm = 90.0
    for auto in root.findall("MasterTrack/Automations/Automation"):
        if _text(auto, "Type") == "Tempo":
            val_parts = _text(auto, "Value").split()
            if val_parts:
                bpm = float(val_parts[0])
                break

    # Choose first track with Tuning property (usually guitar)
    tracks = root.findall("Tracks/Track")
    track_idx = 0
    tuning = list(STANDARD_TUNING_MIDI)
    capo = 0

    for idx, trk in enumerate(tracks):
        t_props = _prop_dict(trk)
        if "Tuning" in t_props:
            pitches_txt = _text(t_props["Tuning"], "Pitches")
            if pitches_txt:
                raw_pitches = [int(p) for p in pitches_txt.split()]
                if len(raw_pitches) == 6:
                    # GPIF Pitches list low->high (40..64); internal is string 1..6 (high->low)
                    tuning = list(reversed(raw_pitches))
                    track_idx = idx
                    if "CapoFret" in t_props:
                        capo = int(_text(t_props["CapoFret"], "Fret", "0"))
                    break

    bars_by_id = {b.attrib["id"]: b for b in root.findall("Bars/Bar") if "id" in b.attrib}
    voices_by_id = {v.attrib["id"]: v for v in root.findall("Voices/Voice") if "id" in v.attrib}
    beats_by_id = {b.attrib["id"]: b for b in root.findall("Beats/Beat") if "id" in b.attrib}
    notes_by_id = {n.attrib["id"]: n for n in root.findall("Notes/Note") if "id" in n.attrib}
    rhythms_by_id = {r.attrib["id"]: r for r in root.findall("Rhythms/Rhythm") if "id" in r.attrib}

    sec_per_beat = 60.0 / bpm
    ts = TimeSignature(4, 4)

    perf = GuitarPerformance(
        title=title,
        artist=artist,
        source_format="guitar_pro",
        bpm=bpm,
        time_signature=ts,
        capo=capo,
        tuning_midi=tuning,
    )

    current_time_sec = 0.0
    current_beat = 0.0
    strum_group_counter = 0
    active_ties: dict[int, GuitarEvent] = {}

    master_bars = root.findall("MasterBars/MasterBar")
    for mb in master_bars:
        time_txt = _text(mb, "Time", "4/4")
        if "/" in time_txt:
            num_s, den_s = time_txt.split("/", 1)
            ts = TimeSignature(int(num_s), int(den_s))
            perf.time_signature = ts

        bar_ids = _text(mb, "Bars").split()
        bar_length_beats = ts.numerator * (4.0 / ts.denominator)
        if track_idx >= len(bar_ids):
            current_beat += bar_length_beats
            current_time_sec += bar_length_beats * sec_per_beat
            continue

        bar_elem = bars_by_id.get(bar_ids[track_idx])
        if bar_elem is None:
            current_beat += bar_length_beats
            current_time_sec += bar_length_beats * sec_per_beat
            continue

        voice_ids = [vid for vid in _text(bar_elem, "Voices").split() if vid != "-1"]
        max_voice_beats = 0.0

        for vid in voice_ids:
            voice_elem = voices_by_id.get(vid)
            if voice_elem is None:
                continue
            voice_beat_offset = 0.0

            for beat_id in _text(voice_elem, "Beats").split():
                beat_elem = beats_by_id.get(beat_id)
                if beat_elem is None:
                    continue
                rhythm_node = beat_elem.find("Rhythm")
                rhythm_ref = rhythm_node.attrib.get("ref") if rhythm_node is not None else None
                rhythm_elem = rhythms_by_id.get(rhythm_ref or "")
                beat_dur_beats = _rhythm_beats(rhythm_elem) if rhythm_elem is not None else 1.0
                beat_dur_sec = beat_dur_beats * sec_per_beat

                note_ids_txt = _text(beat_elem, "Notes")
                if not note_ids_txt:
                    voice_beat_offset += beat_dur_beats
                    continue

                b_props = _prop_dict(beat_elem)
                brush_dir: str | None = None
                if "Brush" in b_props:
                    brush_dir = _text(b_props["Brush"], "Direction", "Down").lower()

                parsed_notes: list[tuple[int, int, Articulation, bool, bool, float]] = []
                for nid in note_ids_txt.split():
                    note_elem = notes_by_id.get(nid)
                    if note_elem is None:
                        continue
                    n_props = _prop_dict(note_elem)
                    if "String" not in n_props or "Fret" not in n_props:
                        continue
                    gp_str_idx = int(_text(n_props["String"], "String", "0"))
                    string_num = max(1, min(6, 6 - gp_str_idx))
                    fret_num = max(0, min(24, int(_text(n_props["Fret"], "Fret", "0"))))

                    articulation: Articulation = "normal"
                    if "PalmMuted" in n_props:
                        articulation = "palm_mute"
                    elif "Muted" in n_props:
                        articulation = "dead_note"
                    elif "HopoOrigin" in n_props or "HammerOn" in n_props:
                        articulation = "hammer_on"
                    elif "PullOff" in n_props:
                        articulation = "pull_off"
                    elif "Slide" in n_props:
                        articulation = "slide"
                    elif "Harmonic" in n_props:
                        articulation = "harmonic"
                    elif note_elem.find("LetRing") is not None:
                        articulation = "let_ring"
                    elif note_elem.find("Vibrato") is not None:
                        articulation = "vibrato"

                    tie_elem = note_elem.find("Tie")
                    tie_origin = tie_elem is not None and tie_elem.attrib.get("origin") == "true"
                    tie_dest = tie_elem is not None and tie_elem.attrib.get("destination") == "true"

                    parsed_notes.append((string_num, fret_num, articulation, tie_origin, tie_dest, 0.80))

                if parsed_notes:
                    left_fingers = assign_left_fingers_for_map(
                        {s: f for s, f, *_ in parsed_notes}
                    )
                    note_time = current_time_sec + voice_beat_offset * sec_per_beat
                    note_beat = current_beat + voice_beat_offset

                    if brush_dir in ("down", "up") and len(parsed_notes) >= 2:
                        strum_group_counter += 1
                        ordered = sorted(
                            parsed_notes,
                            key=lambda x: -x[0] if brush_dir == "down" else x[0],
                        )
                        r_act: RightAction = "strum_down" if brush_dir == "down" else "strum_up"
                        for idx_s, (s, f, art, t_orig, t_dest, vel) in enumerate(ordered):
                            if t_dest and s in active_ties:
                                active_ties[s].duration_sec += beat_dur_sec
                                if not t_orig:
                                    del active_ties[s]
                                continue
                            ev = GuitarEvent(
                                id=0,
                                time_sec=note_time + idx_s * 0.004,
                                duration_sec=beat_dur_sec,
                                beat=note_beat,
                                string=s,
                                fret=f,
                                midi_note=fret_to_midi(s, f, tuning, capo),
                                velocity=vel,
                                left_finger=left_fingers.get(s, 0),
                                right_action=r_act,
                                articulation=art,
                                strum_group_id=strum_group_counter,
                            )
                            perf.events.append(ev)
                            if t_orig:
                                active_ties[s] = ev
                    else:
                        used_right: set[RightAction] = set()
                        for s, f, art, t_orig, t_dest, vel in sorted(parsed_notes, key=lambda x: -x[0]):
                            if t_dest and s in active_ties:
                                active_ties[s].duration_sec += beat_dur_sec
                                if not t_orig:
                                    del active_ties[s]
                                continue
                            r_act = assign_right_hand_pluck(s, used_right)
                            used_right.add(r_act)
                            ev = GuitarEvent(
                                id=0,
                                time_sec=note_time,
                                duration_sec=beat_dur_sec,
                                beat=note_beat,
                                string=s,
                                fret=f,
                                midi_note=fret_to_midi(s, f, tuning, capo),
                                velocity=vel,
                                left_finger=left_fingers.get(s, 0),
                                right_action=r_act,
                                articulation=art,
                            )
                            perf.events.append(ev)
                            if t_orig:
                                active_ties[s] = ev

                voice_beat_offset += beat_dur_beats
            max_voice_beats = max(max_voice_beats, voice_beat_offset)

        advance_beats = max_voice_beats if max_voice_beats > 0 else bar_length_beats
        current_beat += advance_beats
        current_time_sec += advance_beats * sec_per_beat

    return perf.finalize()
