"""Musical expression & humanization pass for GuitarPerformance timelines."""

from __future__ import annotations

import random

from engine.models import GuitarPerformance


def humanize_performance(
    perf: GuitarPerformance,
    timing_jitter_sec: float = 0.004,
    velocity_jitter: float = 0.05,
    melody_boost: float = 0.08,
    seed: int = 1337,
) -> GuitarPerformance:
    """Apply subtle human timing/velocity feel while preserving chord sync and ties."""
    rng = random.Random(seed)
    perf.finalize()

    for ev in perf.events:
        # Keep beat-0 downbeat crisp, add gentle human micro-timing elsewhere
        if ev.time_sec > 0.02 and ev.strum_group_id is None:
            ev.time_sec = max(0.0, ev.time_sec + rng.uniform(-timing_jitter_sec, timing_jitter_sec))

        vel = ev.velocity + rng.uniform(-velocity_jitter, velocity_jitter)
        # Gentle melody accent on top strings in fingerstyle
        if ev.right_action in ("pluck_ring", "pluck_middle") and ev.string <= 2:
            vel += melody_boost
        if ev.articulation in ("hammer_on", "pull_off"):
            vel *= 0.78

        ev.velocity = round(max(0.12, min(1.0, vel)), 4)

    return perf.finalize()
