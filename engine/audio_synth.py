"""Pure-Python Karplus-Strong physical-modeling acoustic guitar WAV synthesizer."""

from __future__ import annotations

import math
from pathlib import Path
import random
import struct
import wave

from engine.models import GuitarEvent, GuitarPerformance


def midi_to_hz(midi_note: int) -> float:
    return 440.0 * (2.0 ** ((midi_note - 69) / 12.0))


def _synthesize_string_note(
    event: GuitarEvent,
    sample_rate: int,
    rng: random.Random,
) -> list[float]:
    midi = event.midi_note + (12 if event.articulation == "harmonic" else 0)
    freq = max(65.0, min(2200.0, midi_to_hz(midi)))

    # Let notes ring naturally for acoustic realism unless palm-muted or dead-note
    if event.articulation == "dead_note":
        render_sec = min(0.09, event.duration_sec + 0.04)
        decay_base = 0.84
        brightness = 0.35
    elif event.articulation == "palm_mute":
        render_sec = min(0.45, event.duration_sec + 0.12)
        decay_base = 0.968
        brightness = 0.32
    else:
        tail = 1.6 if event.articulation == "let_ring" else 0.9
        render_sec = min(3.4, event.duration_sec + tail)
        # Higher frequencies decay faster on real strings
        decay_base = 0.9986 - min(0.0045, (freq / 1200.0) * 0.003)
        brightness = 0.56 if event.right_action in ("strum_down", "strum_up") else 0.48

    if event.articulation in ("hammer_on", "pull_off"):
        brightness *= 0.65

    num_samples = max(64, int(render_sec * sample_rate))
    exact_period = sample_rate / freq
    period_int = max(2, int(exact_period))
    frac = exact_period - period_int

    # Excitation burst shaped by pick-position comb + soft lowpass for warm acoustic timbre
    pick_ratio = 0.22 if event.right_action in ("strum_down", "strum_up") else 0.28
    pick_delay = max(1, int(period_int * pick_ratio))

    exc = [0.0] * period_int
    prev_noise = 0.0
    for i in range(period_int):
        raw = rng.uniform(-1.0, 1.0)
        smoothed = 0.55 * raw + 0.45 * prev_noise
        prev_noise = smoothed
        exc[i] = smoothed

    # Apply pluck-position comb notch
    delay_line = [0.0] * (period_int + 2)
    for i in range(period_int):
        delayed = exc[i - pick_delay] if i >= pick_delay else 0.0
        delay_line[i] = exc[i] - 0.72 * delayed

    if event.articulation == "harmonic":
        # Pure harmonic emphasis
        for i in range(period_int):
            delay_line[i] = 0.5 * math.sin(2.0 * math.pi * i / period_int) + 0.25 * delay_line[i]

    out = [0.0] * num_samples
    line_len = period_int + 1
    write_idx = 0
    prev_sample = delay_line[0]
    body_lp = 0.0
    gain = 0.36 * event.velocity

    # Release fade after note duration ends (gentler when let_ring)
    sustain_end_sample = int(event.duration_sec * sample_rate)
    release_coeff = 0.99965 if event.articulation == "let_ring" else 0.9988

    one_minus_b = 1.0 - brightness
    one_minus_f = 1.0 - frac
    env = 1.0

    for n in range(num_samples):
        read_a = (write_idx - period_int) % line_len
        read_b = (read_a - 1) % line_len
        delayed_val = one_minus_f * delay_line[read_a] + frac * delay_line[read_b]

        # Weighted low-pass string loop filter
        filtered = decay_base * (brightness * delayed_val + one_minus_b * prev_sample)
        prev_sample = filtered
        delay_line[write_idx] = filtered
        write_idx = (write_idx + 1) % line_len

        if n > sustain_end_sample:
            env *= release_coeff

        # Warm wooden guitar body smoothing
        body_lp = 0.72 * filtered + 0.28 * body_lp
        out[n] = body_lp * gain * env

    return out


def synthesize_performance_to_wav(
    perf: GuitarPerformance,
    output_wav_path: str | Path,
    sample_rate: int = 44100,
    seed: int = 42,
) -> Path:
    """Render a GuitarPerformance timeline into a stereo 16-bit PCM WAV file."""
    perf.finalize()
    out_path = Path(output_wav_path)
    out_path.parent.mkdir(parents=True, exist_ok=True)

    total_sec = max(1.0, perf.duration_sec + 1.8)
    total_samples = int(total_sec * sample_rate)
    left_buf = [0.0] * total_samples
    right_buf = [0.0] * total_samples

    rng = random.Random(seed)

    # Slight natural stereo spread across strings 6(left) .. 1(right)
    for ev in perf.events:
        note_buf = _synthesize_string_note(ev, sample_rate, rng)
        start_idx = max(0, int(ev.time_sec * sample_rate))
        pan = (3.5 - ev.string) * 0.08  # -0.20 .. +0.20
        l_gain = math.cos((pan + 1.0) * math.pi * 0.25)
        r_gain = math.sin((pan + 1.0) * math.pi * 0.25)

        limit = min(len(note_buf), total_samples - start_idx)
        for i in range(limit):
            s = note_buf[i]
            pos = start_idx + i
            left_buf[pos] += s * l_gain
            right_buf[pos] += s * r_gain

    # Subtle stereo acoustic room reflection (~22ms / ~37ms)
    d1 = int(0.022 * sample_rate)
    d2 = int(0.037 * sample_rate)
    for i in range(d2, total_samples):
        left_buf[i] += 0.11 * right_buf[i - d1] + 0.06 * left_buf[i - d2]
        right_buf[i] += 0.11 * left_buf[i - d1] + 0.06 * right_buf[i - d2]

    # Normalize safely with soft clipping protection
    peak = max(
        max((abs(x) for x in left_buf), default=0.0),
        max((abs(x) for x in right_buf), default=0.0),
        1e-6,
    )
    norm_scale = min(1.8, 0.88 / peak)

    pcm_bytes = bytearray(total_samples * 4)
    pack_into = struct.pack_into
    for i in range(total_samples):
        l_val = math.tanh(left_buf[i] * norm_scale)
        r_val = math.tanh(right_buf[i] * norm_scale)
        li = max(-32767, min(32767, int(l_val * 32767.0)))
        ri = max(-32767, min(32767, int(r_val * 32767.0)))
        pack_into("<hh", pcm_bytes, i * 4, li, ri)

    with wave.open(str(out_path), "wb") as wf:
        wf.setnchannels(2)
        wf.setsampwidth(2)
        wf.setframerate(sample_rate)
        wf.writeframes(pcm_bytes)

    return out_path
