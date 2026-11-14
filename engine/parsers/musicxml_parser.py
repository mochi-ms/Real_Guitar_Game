"""Parser for MusicXML (.musicxml, .xml, .mxl) guitar TAB and standard notation files."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import xml.etree.ElementTree as ET
import zipfile

from engine.fingering_solver import (
    RawMidiNote,
    assign_left_fingers_for_map,
    assign_right_hand_pluck,
    solve_fingering_sequence,
)
from engine.models import (
    STANDARD_TUNING_MIDI,
    Articulation,
    GuitarEvent,
    GuitarPerformance,
    RightAction,
    TimeSignature,
    fret_to_midi,
    note_to_midi,
)


def _strip_ns(tag: str) -> str:
    return tag.rsplit("}", 1)[-1] if "}" in tag else tag


def _load_musicxml_root(path: Path) -> ET.Element:
    if path.suffix.lower() == ".mxl":
        with zipfile.ZipFile(path, "r") as zf:
            root_file: str | None = None
            if "META-INF/container.xml" in zf.namelist():
                container_xml = ET.fromstring(zf.read("META-INF/container.xml"))
                for elem in container_xml.iter():
                    if _strip_ns(elem.tag) == "rootfile" and elem.attrib.get("full-path"):
                        root_file = elem.attrib["full-path"]
                        break
            if not root_file:
                xml_names = [n for n in zf.namelist() if n.lower().endswith((".xml", ".musicxml")) and "META-INF" not in n]
                if not xml_names:
                    raise ValueError(f"No MusicXML score found inside .mxl archive: {path}")
                root_file = xml_names[0]
            xml_bytes = zf.read(root_file)
    else:
        xml_bytes = path.read_bytes()

    root = ET.fromstring(xml_bytes)
    for elem in root.iter():
        elem.tag = _strip_ns(elem.tag)
    return root


@dataclass(slots=True)
class _ParsedTabNote:
    time_sec: float
    duration_sec: float
    beat: float
    string: int | None
    fret: int | None
    midi_note: int
    velocity: float
    articulation: Articulation
    arpeggiate_dir: str | None  # "up" | "down" | None
    tie_stop: bool
    tie_start: bool


def _child_text(parent: ET.Element, tag: str) -> str | None:
    node = parent.find(tag)
    return node.text.strip() if node is not None and node.text else None


def parse_musicxml_file(path: str | Path) -> GuitarPerformance:
    """Parse a MusicXML (.musicxml / .xml / .mxl) guitar file into GuitarPerformance."""
    file_path = Path(path)
    root = _load_musicxml_root(file_path)
    if root.tag != "score-partwise":
        raise ValueError(f"Only <score-partwise> MusicXML is supported (got <{root.tag}>).")

    title = (
        _child_text(root, "work/work-title")
        or _child_text(root, "movement-title")
        or file_path.stem
    )
    artist = "Unknown"
    for creator in root.findall("identification/creator"):
        if creator.text and creator.attrib.get("type", "composer") in ("composer", "artist", "arranger"):
            artist = creator.text.strip()
            break

    parts = root.findall("part")
    if not parts:
        raise ValueError(f"No <part> elements found in {file_path}")

    # Pick the first part that contains <technical><string>, or fallback to first part
    chosen_part = parts[0]
    for p in parts:
        if p.find(".//technical/string") is not None:
            chosen_part = p
            break

    tuning = list(STANDARD_TUNING_MIDI)
    capo = 0
    bpm = 90.0
    ts = TimeSignature(4, 4)
    divisions = 480

    # Extract initial tuning / capo / tempo if present in measure 1
    first_attr = chosen_part.find("measure/attributes")
    if first_attr is not None:
        staff_tunings = first_attr.findall("staff-details/staff-tuning")
        if len(staff_tunings) == 6:
            # MusicXML line="1" is lowest string (6th string) .. line="6" is highest string (1st string)
            parsed_tuning: dict[int, int] = {}
            for st in staff_tunings:
                line = int(st.attrib.get("line", "1"))
                step = _child_text(st, "tuning-step") or "E"
                alter = int(_child_text(st, "tuning-alter") or "0")
                octave = int(_child_text(st, "tuning-octave") or "4")
                string_num = 7 - line
                if 1 <= string_num <= 6:
                    parsed_tuning[string_num] = note_to_midi(step, alter, octave)
            if len(parsed_tuning) == 6:
                tuning = [parsed_tuning[s] for s in range(1, 7)]
        capo_txt = _child_text(first_attr, "staff-details/capo")
        if capo_txt and capo_txt.isdigit():
            capo = int(capo_txt)

    first_sound = chosen_part.find(".//sound[@tempo]")
    if first_sound is not None:
        bpm = float(first_sound.attrib["tempo"])

    current_time_sec = 0.0
    current_beat = 0.0
    collected: list[_ParsedTabNote] = []
    # Track tied notes keyed by (string or midi) so tie-stop extends previous note
    active_ties: dict[tuple[int | None, int], _ParsedTabNote] = {}

    for measure in chosen_part.findall("measure"):
        attr = measure.find("attributes")
        if attr is not None:
            div_txt = _child_text(attr, "divisions")
            if div_txt:
                divisions = max(1, int(float(div_txt)))
            beats_txt = _child_text(attr, "time/beats")
            btype_txt = _child_text(attr, "time/beat-type")
            if beats_txt and btype_txt:
                ts = TimeSignature(numerator=int(beats_txt), denominator=int(btype_txt))

        measure_cursor_ticks = 0
        max_measure_ticks = 0
        last_note_start_ticks = 0
        last_note_dur_ticks = 0
        sec_per_tick = (60.0 / bpm) / divisions

        for child in list(measure):
            if child.tag == "direction":
                snd = child.find("sound")
                if snd is not None and "tempo" in snd.attrib:
                    bpm = float(snd.attrib["tempo"])
                    sec_per_tick = (60.0 / bpm) / divisions
            elif child.tag == "sound" and "tempo" in child.attrib:
                bpm = float(child.attrib["tempo"])
                sec_per_tick = (60.0 / bpm) / divisions
            elif child.tag == "backup":
                dur = int(float(_child_text(child, "duration") or "0"))
                measure_cursor_ticks = max(0, measure_cursor_ticks - dur)
            elif child.tag == "forward":
                dur = int(float(_child_text(child, "duration") or "0"))
                measure_cursor_ticks += dur
                max_measure_ticks = max(max_measure_ticks, measure_cursor_ticks)
            elif child.tag == "note":
                is_chord = child.find("chord") is not None
                is_rest = child.find("rest") is not None
                dur_ticks = int(float(_child_text(child, "duration") or "0"))

                if is_chord:
                    start_ticks = last_note_start_ticks
                else:
                    start_ticks = measure_cursor_ticks
                    last_note_start_ticks = start_ticks
                    last_note_dur_ticks = dur_ticks
                    measure_cursor_ticks += dur_ticks
                    max_measure_ticks = max(max_measure_ticks, measure_cursor_ticks)

                if is_rest or dur_ticks <= 0:
                    continue

                note_time_sec = current_time_sec + start_ticks * sec_per_tick
                note_dur_sec = dur_ticks * sec_per_tick
                note_beat = current_beat + (start_ticks / divisions)

                # Pitch
                pitch_node = child.find("pitch")
                midi_note = 64
                if pitch_node is not None:
                    step = _child_text(pitch_node, "step") or "E"
                    alter = int(float(_child_text(pitch_node, "alter") or "0"))
                    octave = int(_child_text(pitch_node, "octave") or "4")
                    midi_note = note_to_midi(step, alter, octave)

                # Technical TAB info
                string_num: int | None = None
                fret_num: int | None = None
                articulation: Articulation = "normal"
                arpeggiate_dir: str | None = None

                notations = child.find("notations")
                if notations is not None:
                    tech = notations.find("technical")
                    if tech is not None:
                        s_txt = _child_text(tech, "string")
                        f_txt = _child_text(tech, "fret")
                        if s_txt and s_txt.isdigit():
                            string_num = int(s_txt)
                        if f_txt and f_txt.isdigit():
                            fret_num = int(f_txt)
                        if tech.find("hammer-on") is not None:
                            articulation = "hammer_on"
                        elif tech.find("pull-off") is not None:
                            articulation = "pull_off"
                        elif tech.find("harmonic") is not None:
                            articulation = "harmonic"
                        elif tech.find("palm-mute") is not None:
                            articulation = "palm_mute"
                    if notations.find("slide") is not None or notations.find("glissando") is not None:
                        articulation = "slide"
                    arp = notations.find("arpeggiate")
                    if arp is not None:
                        arpeggiate_dir = arp.attrib.get("direction", "down")

                if string_num is not None and fret_num is not None and 1 <= string_num <= 6:
                    midi_note = fret_to_midi(string_num, fret_num, tuning, capo)

                tie_types = {t.attrib.get("type") for t in child.findall("tie")}
                tie_start = "start" in tie_types
                tie_stop = "stop" in tie_types
                tie_key = (string_num, midi_note)

                if tie_stop and tie_key in active_ties:
                    prev = active_ties[tie_key]
                    prev.duration_sec += note_dur_sec
                    if not tie_start:
                        del active_ties[tie_key]
                    continue

                parsed = _ParsedTabNote(
                    time_sec=note_time_sec,
                    duration_sec=note_dur_sec,
                    beat=note_beat,
                    string=string_num,
                    fret=fret_num,
                    midi_note=midi_note,
                    velocity=0.80,
                    articulation=articulation,
                    arpeggiate_dir=arpeggiate_dir,
                    tie_stop=tie_stop,
                    tie_start=tie_start,
                )
                collected.append(parsed)
                if tie_start:
                    active_ties[tie_key] = parsed

        # Advance time by actual measure length (or time signature measure length)
        if max_measure_ticks <= 0:
            max_measure_ticks = int(divisions * ts.numerator * (4 / ts.denominator))
        current_time_sec += max_measure_ticks * sec_per_tick
        current_beat += max_measure_ticks / divisions

    perf = GuitarPerformance(
        title=title,
        artist=artist,
        source_format="musicxml",
        bpm=bpm,
        time_signature=ts,
        capo=capo,
        tuning_midi=tuning,
    )

    if not collected:
        return perf.finalize()

    # If file lacked TAB tags, fall back to fingering solver
    if all(n.string is None or n.fret is None for n in collected):
        raw = [
            RawMidiNote(
                time_sec=n.time_sec,
                duration_sec=n.duration_sec,
                midi_note=n.midi_note,
                velocity=n.velocity,
                beat=n.beat,
            )
            for n in collected
        ]
        for pos in solve_fingering_sequence(raw, tuning, capo):
            perf.events.append(
                GuitarEvent(
                    id=0,
                    time_sec=pos.raw.time_sec,
                    duration_sec=pos.raw.duration_sec,
                    beat=pos.raw.beat,
                    string=pos.string,
                    fret=pos.fret,
                    midi_note=pos.raw.midi_note,
                    velocity=pos.raw.velocity,
                    left_finger=pos.left_finger,
                    right_action=pos.right_action,
                    articulation="normal",
                )
            )
        return perf.finalize()

    # Group simultaneous TAB notes to assign coherent left-hand fingers and strum spreads
    collected.sort(key=lambda n: (round(n.time_sec, 5), -(n.string or 1)))
    clusters: list[list[_ParsedTabNote]] = [[collected[0]]]
    for item in collected[1:]:
        if abs(item.time_sec - clusters[-1][0].time_sec) <= 0.015:
            clusters[-1].append(item)
        else:
            clusters.append([item])

    strum_group_counter = 0
    for cluster in clusters:
        resolved: list[tuple[_ParsedTabNote, int, int]] = []
        for item in cluster:
            s = item.string if item.string and 1 <= item.string <= 6 else 1
            f = item.fret if item.fret is not None and 0 <= item.fret <= 24 else 0
            resolved.append((item, s, f))

        left_fingers = assign_left_fingers_for_map({s: f for _, s, f in resolved})
        arp_dir = next((it.arpeggiate_dir for it, _, _ in resolved if it.arpeggiate_dir), None)

        if arp_dir in ("down", "up") and len(resolved) >= 2:
            strum_group_counter += 1
            # MusicXML arpeggiate direction="down" goes high-to-low pitch visually or low-to-high string
            ordered = sorted(
                resolved,
                key=lambda r: -r[1] if arp_dir == "down" else r[1],
            )
            r_act: RightAction = "strum_down" if arp_dir == "down" else "strum_up"
            for idx_s, (item, s, f) in enumerate(ordered):
                perf.events.append(
                    GuitarEvent(
                        id=0,
                        time_sec=item.time_sec + idx_s * 0.004,
                        duration_sec=item.duration_sec,
                        beat=item.beat,
                        string=s,
                        fret=f,
                        midi_note=item.midi_note,
                        velocity=item.velocity,
                        left_finger=left_fingers.get(s, 0),
                        right_action=r_act,
                        articulation=item.articulation,
                        strum_group_id=strum_group_counter,
                    )
                )
        else:
            used_right: set[RightAction] = set()
            for item, s, f in sorted(resolved, key=lambda r: -r[1]):
                r_act = assign_right_hand_pluck(s, used_right)
                used_right.add(r_act)
                perf.events.append(
                    GuitarEvent(
                        id=0,
                        time_sec=item.time_sec,
                        duration_sec=item.duration_sec,
                        beat=item.beat,
                        string=s,
                        fret=f,
                        midi_note=item.midi_note,
                        velocity=item.velocity,
                        left_finger=left_fingers.get(s, 0),
                        right_action=r_act,
                        articulation=item.articulation,
                    )
                )

    return perf.finalize()
