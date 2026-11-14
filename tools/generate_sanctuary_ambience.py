"""Synthesize a seamless 16-second stereo lakeside sanctuary ambient loop (.wav)."""

from __future__ import annotations

import math
from pathlib import Path
import random
import struct
import wave

REPO_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_AMBIENCE_WAV = (
    REPO_ROOT / "ue5" / "RealGuitarGame" / "Content" / "Audio" / "Sanctuary_Ambience.wav"
)


def generate_sanctuary_ambience_wav(
    output_path: Path | None = None,
    duration_sec: float = 16.0,
    sample_rate: int = 44100,
    seed: int = 710,
) -> Path:
    """Create a warm stereo loop with forest breeze, soft lake shore laps, and hearth crackles."""
    out = output_path or DEFAULT_AMBIENCE_WAV
    out.parent.mkdir(parents=True, exist_ok=True)

    n = int(duration_sec * sample_rate)
    rng = random.Random(seed)

    # Low-pass pink-tinted breeze & gentle water bed using cyclic oscillators + filtered noise
    left = [0.0] * n
    right = [0.0] * n

    # Pre-schedule sparse soft water laps & cozy ember crackles wrapped modularly for seamless looping
    lap_events: list[tuple[int, float, float]] = []
    for i in range(10):
        center = int((i + rng.uniform(0.15, 0.85)) * (n / 10.0)) % n
        pan = rng.uniform(-0.45, 0.45)
        amp = rng.uniform(0.06, 0.12)
        lap_events.append((center, pan, amp))

    crackle_events: list[tuple[int, float, float]] = []
    for _ in range(52):
        idx = rng.randrange(0, n)
        pan = rng.uniform(-0.7, -0.15)
        amp = rng.uniform(0.04, 0.14)
        crackle_events.append((idx, pan, amp))

    lp_l = 0.0
    lp_r = 0.0
    wl = 0.0
    wr = 0.0

    # Periodic phase multiples so the end loops smoothly into index 0
    two_pi = 2.0 * math.pi
    crossfade_samples = int(0.8 * sample_rate)

    raw_l = [0.0] * (n + crossfade_samples)
    raw_r = [0.0] * (n + crossfade_samples)

    for i in range(n + crossfade_samples):
        t = i / sample_rate
        # Soft wind swell modulation (integer cycles over duration_sec)
        swell_l = 0.55 + 0.35 * math.sin(two_pi * 2.0 * (i / n)) + 0.15 * math.sin(two_pi * 5.0 * (i / n) + 0.7)
        swell_r = 0.55 + 0.35 * math.sin(two_pi * 2.0 * (i / n) + 1.1) + 0.15 * math.sin(two_pi * 3.0 * (i / n) + 2.1)

        wn_l = rng.uniform(-1.0, 1.0)
        wn_r = rng.uniform(-1.0, 1.0)

        # Warm smooth low-pass filter (~380Hz air/foliage breeze)
        lp_l = 0.945 * lp_l + 0.055 * wn_l
        lp_r = 0.945 * lp_r + 0.055 * wn_r

        # Deeper lake murmur bed (~110Hz warm brownian texture)
        wl = 0.985 * wl + 0.015 * wn_l
        wr = 0.985 * wr + 0.015 * wn_r

        raw_l[i] = 0.16 * lp_l * swell_l + 0.24 * wl
        raw_r[i] = 0.16 * lp_r * swell_r + 0.24 * wr

    # Overlay soft shoreline water laps
    lap_len = int(1.3 * sample_rate)
    for center, pan, amp in lap_events:
        lgain = math.sqrt(0.5 * (1.0 - pan))
        rgain = math.sqrt(0.5 * (1.0 + pan))
        local_lp = 0.0
        for j in range(lap_len):
            idx = (center + j) % n
            env = math.sin(math.pi * (j / lap_len)) ** 1.8
            noise = rng.uniform(-1.0, 1.0)
            local_lp = 0.88 * local_lp + 0.12 * noise
            val = amp * env * local_lp
            raw_l[idx] += val * lgain
            raw_r[idx] += val * rgain

    # Overlay delicate campfire ember pops/crackles
    pop_len = int(0.032 * sample_rate)
    for start, pan, amp in crackle_events:
        lgain = math.sqrt(0.5 * (1.0 - pan))
        rgain = math.sqrt(0.5 * (1.0 + pan))
        freq = rng.uniform(1100.0, 2600.0)
        for j in range(pop_len):
            idx = (start + j) % n
            t_pop = j / sample_rate
            env = math.exp(-j / (0.0055 * sample_rate))
            sig = amp * env * (0.65 * math.sin(two_pi * freq * t_pop) + 0.35 * rng.uniform(-1.0, 1.0))
            raw_l[idx] += sig * lgain
            raw_r[idx] += sig * rgain

    # Equal-power crossfade overlap at loop seam for click-free infinite looping
    for i in range(n):
        if i < crossfade_samples:
            alpha = i / crossfade_samples
            left[i] = alpha * raw_l[i] + (1.0 - alpha) * raw_l[n + i]
            right[i] = alpha * raw_r[i] + (1.0 - alpha) * raw_r[n + i]
        else:
            left[i] = raw_l[i]
            right[i] = raw_r[i]

    buf = bytearray()
    for l_val, r_val in zip(left, right):
        il = max(-32767, min(32767, int(l_val * 24000.0)))
        ir = max(-32767, min(32767, int(r_val * 24000.0)))
        buf.extend(struct.pack("<hh", il, ir))

    with wave.open(str(out), "wb") as wf:
        wf.setnchannels(2)
        wf.setsampwidth(2)
        wf.setframerate(sample_rate)
        wf.writeframes(bytes(buf))

    return out


if __name__ == "__main__":
    p = generate_sanctuary_ambience_wav()
    print(f"Generated {p} ({p.stat().st_size} bytes)")
