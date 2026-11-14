"""Pure-Python Standard MIDI File (.mid) parser with guitar fingering optimization."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import struct

from engine.fingering_solver import RawMidiNote, solve_fingering_sequence
from engine.models import (
    STANDARD_TUNING_MIDI,
    GuitarEvent,
    GuitarPerformance,
    TimeSignature,
)


def _read_vlq(data: bytes, offset: int) -> tuple[int, int]:
    value = 0
    while offset < len(data):
        b = data[offset]
        offset += 1
        value = (value << 7) | (b & 0x7F)
        if not (b & 0x80):
            break
    return value, offset


def encode_vlq(value: int) -> bytes:
    """Encode a non-negative integer as a MIDI Variable-Length Quantity."""
    buf = [value & 0x7F]
    value >>= 7
    while value > 0:
        buf.append((value & 0x7F) | 0x80)
        value >>= 7
    return bytes(reversed(buf))


@dataclass(slots=True)
class _TempoPoint:
    tick: int
    us_per_quarter: int


def parse_midi_file(
    path: str | Path,
    tuning_midi: tuple[int, ...] = STANDARD_TUNING_MIDI,
    capo: int = 0,
    ignore_drums_channel: bool = True,
) -> GuitarPerformance:
    """Parse a Standard MIDI File (.mid/.midi) and solve optimal guitar fingering."""
    file_path = Path(path)
    data = file_path.read_bytes()
    if len(data) < 14 or data[:4] != b"MThd":
        raise ValueError(f"Invalid MIDI file header: {file_path}")

    header_len, _fmt, num_tracks, division = struct.unpack(">IHHH", data[4:14])
    if division & 0x8000:
        raise ValueError("SMPTE MIDI time division is not supported; use PPQN MIDI files.")
    ppqn = max(1, division)

    pos = 8 + header_len
    tempo_points: list[_TempoPoint] = [_TempoPoint(0, 500_000)]  # default 120 BPM
    time_sig = TimeSignature(4, 4)
    title = file_path.stem

    # Active notes: (channel, pitch) -> (start_tick, velocity_float)
    raw_tick_notes: list[tuple[int, int, int, float]] = []  # (start_tick, end_tick, pitch, vel)

    for track_idx in range(num_tracks):
        if pos + 8 > len(data) or data[pos : pos + 4] != b"MTrk":
            break
        track_len = struct.unpack(">I", data[pos + 4 : pos + 8])[0]
        track_start = pos + 8
        track_end = min(len(data), track_start + track_len)
        pos = track_end

        cursor = track_start
        abs_tick = 0
        running_status = 0
        active_notes: dict[tuple[int, int], tuple[int, float]] = {}

        while cursor < track_end:
            delta, cursor = _read_vlq(data, cursor)
            abs_tick += delta
            if cursor >= track_end:
                break

            status = data[cursor]
            if status < 0x80:
                if running_status == 0:
                    raise ValueError("MIDI running status encountered before status byte.")
                status = running_status
            else:
                cursor += 1
                if status < 0xF0:
                    running_status = status

            if status == 0xFF:
                if cursor >= track_end:
                    break
                meta_type = data[cursor]
                cursor += 1
                meta_len, cursor = _read_vlq(data, cursor)
                meta_payload = data[cursor : cursor + meta_len]
                cursor += meta_len

                if meta_type == 0x51 and len(meta_payload) == 3:
                    us_per_q = int.from_bytes(meta_payload, "big")
                    tempo_points.append(_TempoPoint(abs_tick, max(1000, us_per_q)))
                elif meta_type == 0x58 and len(meta_payload) >= 2:
                    time_sig = TimeSignature(
                        numerator=int(meta_payload[0]),
                        denominator=2 ** int(meta_payload[1]),
                    )
                elif meta_type == 0x03 and track_idx == 0 and meta_payload:
                    decoded = meta_payload.decode("utf-8", errors="ignore").strip()
                    if decoded:
                        title = decoded
            elif status in (0xF0, 0xF7):
                sysex_len, cursor = _read_vlq(data, cursor)
                cursor += sysex_len
            else:
                msg_type = status & 0xF0
                channel = status & 0x0F
                # Program Change (0xC0) & Channel Pressure (0xD0) take 1 data byte
                if msg_type in (0xC0, 0xD0):
                    cursor += 1
                    continue
                if cursor + 1 >= track_end:
                    break
                d1 = data[cursor]
                d2 = data[cursor + 1]
                cursor += 2

                if ignore_drums_channel and channel == 9:
                    continue

                if msg_type == 0x90 and d2 > 0:
                    key = (channel, d1)
                    if key in active_notes:
                        st, vel = active_notes.pop(key)
                        if abs_tick > st:
                            raw_tick_notes.append((st, abs_tick, d1, vel))
                    active_notes[key] = (abs_tick, d2 / 127.0)
                elif msg_type == 0x80 or (msg_type == 0x90 and d2 == 0):
                    key = (channel, d1)
                    if key in active_notes:
                        st, vel = active_notes.pop(key)
                        end_t = max(st + ppqn // 8, abs_tick)
                        raw_tick_notes.append((st, end_t, d1, vel))

        # Close dangling notes at track end
        for (_, pitch), (st, vel) in active_notes.items():
            end_t = max(st + ppqn // 4, abs_tick)
            raw_tick_notes.append((st, end_t, pitch, vel))

    # Build tempo segment lookup: (tick, sec_at_tick, us_per_quarter)
    tempo_points.sort(key=lambda t: t.tick)
    # Deduplicate same-tick tempo changes keeping last
    dedup_tempo: list[_TempoPoint] = []
    for tp in tempo_points:
        if dedup_tempo and dedup_tempo[-1].tick == tp.tick:
            dedup_tempo[-1] = tp
        else:
            dedup_tempo.append(tp)

    tempo_segments: list[tuple[int, float, int]] = []
    acc_sec = 0.0
    prev_tick = 0
    prev_us = dedup_tempo[0].us_per_quarter
    for tp in dedup_tempo:
        if tp.tick > prev_tick:
            acc_sec += ((tp.tick - prev_tick) / ppqn) * (prev_us / 1_000_000.0)
        tempo_segments.append((tp.tick, acc_sec, tp.us_per_quarter))
        prev_tick = tp.tick
        prev_us = tp.us_per_quarter

    def tick_to_sec(tick: int) -> float:
        base_tick, base_sec, us_per_q = tempo_segments[0]
        for seg_tick, seg_sec, seg_us in tempo_segments:
            if seg_tick <= tick:
                base_tick, base_sec, us_per_q = seg_tick, seg_sec, seg_us
            else:
                break
        return base_sec + ((tick - base_tick) / ppqn) * (us_per_q / 1_000_000.0)

    initial_bpm = round(60_000_000.0 / dedup_tempo[0].us_per_quarter, 2)
    raw_notes = [
        RawMidiNote(
            time_sec=tick_to_sec(st),
            duration_sec=max(0.05, tick_to_sec(et) - tick_to_sec(st)),
            midi_note=pitch,
            velocity=vel,
            beat=st / ppqn,
        )
        for st, et, pitch, vel in raw_tick_notes
    ]

    solved = solve_fingering_sequence(raw_notes, tuning_midi=tuning_midi, capo=capo)

    perf = GuitarPerformance(
        title=title,
        artist="MIDI Import",
        source_format="midi",
        bpm=initial_bpm,
        time_signature=time_sig,
        capo=capo,
        tuning_midi=list(tuning_midi),
    )
    for pos_item in solved:
        perf.events.append(
            GuitarEvent(
                id=0,
                time_sec=pos_item.raw.time_sec,
                duration_sec=pos_item.raw.duration_sec,
                beat=pos_item.raw.beat,
                string=pos_item.string,
                fret=pos_item.fret,
                midi_note=pos_item.raw.midi_note,
                velocity=pos_item.raw.velocity,
                left_finger=pos_item.left_finger,
                right_action=pos_item.right_action,
                articulation="normal",
            )
        )

    return perf.finalize()
