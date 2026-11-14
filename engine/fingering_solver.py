"""Ergonomic guitar fretboard fingering solver and right-hand finger assigner."""

from __future__ import annotations

from dataclasses import dataclass
from itertools import product
from typing import Iterable, Sequence

from engine.models import STANDARD_TUNING_MIDI, RightAction


@dataclass(slots=True)
class RawMidiNote:
    time_sec: float
    duration_sec: float
    midi_note: int
    velocity: float = 0.8
    beat: float = 0.0


@dataclass(slots=True)
class SolvedPosition:
    raw: RawMidiNote
    string: int
    fret: int
    left_finger: int
    right_action: RightAction


def clamp_to_guitar_range(
    midi_note: int,
    tuning_midi: Sequence[int] = STANDARD_TUNING_MIDI,
    capo: int = 0,
    max_fret: int = 20,
) -> int:
    """Fold out-of-range MIDI pitches by octaves so they fit on the fretboard."""
    low = min(tuning_midi) + capo
    high = max(tuning_midi) + capo + max_fret
    note = midi_note
    while note < low:
        note += 12
    while note > high:
        note -= 12
    return note


def candidate_positions(
    midi_note: int,
    tuning_midi: Sequence[int] = STANDARD_TUNING_MIDI,
    capo: int = 0,
    max_fret: int = 20,
) -> list[tuple[int, int]]:
    """Return all valid (string_1_to_6, fret_0_to_max) pairs for a MIDI note."""
    target = clamp_to_guitar_range(midi_note, tuning_midi, capo, max_fret)
    positions: list[tuple[int, int]] = []
    for idx, open_midi in enumerate(tuning_midi):
        fret = target - (int(open_midi) + capo)
        if 0 <= fret <= max_fret:
            positions.append((idx + 1, fret))
    return positions


def assign_left_fingers_for_map(string_to_fret: dict[int, int]) -> dict[int, int]:
    """Assign left-hand fingers (0=open, 1..4=index..pinky) for a set of active strings."""
    result: dict[int, int] = {}
    fretted = [(s, f) for s, f in string_to_fret.items() if f > 0]
    for s, f in string_to_fret.items():
        if f == 0:
            result[s] = 0

    if not fretted:
        return result

    min_fret = min(f for _, f in fretted)
    # Sort by fret ascending, then bass-to-treble (higher string number first)
    fretted.sort(key=lambda item: (item[1], -item[0]))

    if len(fretted) == 1:
        s, _ = fretted[0]
        result[s] = 1
        return result

    # Detect potential barre when multiple strings sit on min_fret with higher notes above
    at_min = [s for s, f in fretted if f == min_fret]
    above_min = [(s, f) for s, f in fretted if f > min_fret]
    if len(at_min) >= 2 and above_min:
        for s in at_min:
            result[s] = 1
        finger = 2
        for s, f in above_min:
            offset = f - min_fret
            candidate_finger = min(4, max(finger, min(4, 1 + offset)))
            result[s] = candidate_finger
            finger = min(4, candidate_finger + 1)
        return result

    used = 0
    for idx, (s, f) in enumerate(fretted):
        offset = f - min_fret
        finger = min(4, max(idx + 1, 1 + offset))
        if finger <= used and used < 4:
            finger = used + 1
        finger = min(4, finger)
        used = finger
        result[s] = finger
    return result


def assign_left_fingers_for_shape(frets: Sequence[int]) -> list[int]:
    """Given 6 frets (string 1..6, -1=mute), return 6 finger numbers (0..4)."""
    active = {i + 1: f for i, f in enumerate(frets) if f >= 0}
    assigned = assign_left_fingers_for_map(active)
    return [assigned.get(i + 1, 0) if frets[i] >= 0 else 0 for i in range(6)]


def assign_right_hand_pluck(string_num: int, used_actions: set[RightAction] | None = None) -> RightAction:
    """Choose a natural fingerstyle pluck action (P/i/m/a) for a string, avoiding collisions."""
    used = used_actions or set()
    if string_num >= 4:
        preferred: tuple[RightAction, ...] = ("pluck_thumb", "pluck_index", "pluck_middle", "pluck_ring")
    elif string_num == 3:
        preferred = ("pluck_index", "pluck_thumb", "pluck_middle", "pluck_ring")
    elif string_num == 2:
        preferred = ("pluck_middle", "pluck_index", "pluck_ring", "pluck_thumb")
    else:
        preferred = ("pluck_ring", "pluck_middle", "pluck_index", "pluck_thumb")

    for act in preferred:
        if act not in used:
            return act
    return preferred[0]


def _cluster_notes(notes: Sequence[RawMidiNote], window_sec: float = 0.025) -> list[list[RawMidiNote]]:
    if not notes:
        return []
    ordered = sorted(notes, key=lambda n: (n.time_sec, n.midi_note))
    clusters: list[list[RawMidiNote]] = [[ordered[0]]]
    for note in ordered[1:]:
        if note.time_sec - clusters[-1][0].time_sec <= window_sec and len(clusters[-1]) < 6:
            clusters[-1].append(note)
        else:
            clusters.append([note])
    return clusters


def _cluster_assignments(
    cluster: Sequence[RawMidiNote],
    tuning_midi: Sequence[int],
    capo: int,
    max_fret: int,
) -> list[tuple[tuple[int, int], ...]]:
    """Generate valid non-colliding string assignments for simultaneous notes in a cluster."""
    per_note_cands = [
        candidate_positions(n.midi_note, tuning_midi, capo, max_fret) for n in cluster
    ]
    valid: list[tuple[tuple[int, int], ...]] = []

    for combo in product(*per_note_cands):
        strings = [s for s, _ in combo]
        if len(set(strings)) != len(strings):
            continue
        fretted = [f for _, f in combo if f > 0]
        if fretted and (max(fretted) - min(fretted) > 4):
            continue
        valid.append(combo)

    if valid:
        return valid

    # Fallback if a cluster spans too wide (>6 notes or impossible chord): greedy per-note assignment
    used_strings: set[int] = set()
    greedy: list[tuple[int, int]] = []
    for cands in per_note_cands:
        pick = next((pos for pos in cands if pos[0] not in used_strings), cands[0])
        used_strings.add(pick[0])
        greedy.append(pick)
    return [tuple(greedy)]


def _anchor_fret(combo: Sequence[tuple[int, int]], fallback: float) -> float:
    fretted = [f for _, f in combo if f > 0]
    return sum(fretted) / len(fretted) if fretted else fallback


def _local_cost(combo: Sequence[tuple[int, int]]) -> float:
    fretted = [f for _, f in combo if f > 0]
    if not fretted:
        return 0.0
    span = max(fretted) - min(fretted)
    avg_fret = sum(fretted) / len(fretted)
    # Slightly favor lower/open-position frets and tight hand shapes
    return span * 2.2 + avg_fret * 0.35


def solve_fingering_sequence(
    raw_notes: Iterable[RawMidiNote],
    tuning_midi: Sequence[int] = STANDARD_TUNING_MIDI,
    capo: int = 0,
    max_fret: int = 20,
) -> list[SolvedPosition]:
    """Optimize 6-string fretboard positions and finger assignments across a MIDI note stream."""
    notes = list(raw_notes)
    if not notes:
        return []

    clusters = _cluster_notes(notes)
    states_per_cluster = [
        _cluster_assignments(cl, tuning_midi, capo, max_fret) for cl in clusters
    ]

    # Dynamic programming (Viterbi) across clusters
    dp_costs: list[list[float]] = []
    back_ptrs: list[list[int]] = []

    first_costs = [_local_cost(combo) for combo in states_per_cluster[0]]
    dp_costs.append(first_costs)
    back_ptrs.append([-1] * len(first_costs))

    for step in range(1, len(clusters)):
        prev_combos = states_per_cluster[step - 1]
        curr_combos = states_per_cluster[step]
        prev_costs = dp_costs[step - 1]
        step_costs: list[float] = []
        step_back: list[int] = []

        for curr_combo in curr_combos:
            curr_anchor = _anchor_fret(curr_combo, 2.0)
            curr_local = _local_cost(curr_combo)
            best_total = float("inf")
            best_prev_idx = 0

            for prev_idx, prev_combo in enumerate(prev_combos):
                prev_anchor = _anchor_fret(prev_combo, 2.0)
                shift_penalty = abs(curr_anchor - prev_anchor) * 1.6
                total = prev_costs[prev_idx] + curr_local + shift_penalty
                if total < best_total:
                    best_total = total
                    best_prev_idx = prev_idx

            step_costs.append(best_total)
            step_back.append(best_prev_idx)

        dp_costs.append(step_costs)
        back_ptrs.append(step_back)

    # Reconstruct optimal path
    best_last = min(range(len(dp_costs[-1])), key=lambda i: dp_costs[-1][i])
    chosen_indices = [0] * len(clusters)
    idx = best_last
    for step in range(len(clusters) - 1, -1, -1):
        chosen_indices[step] = idx
        idx = back_ptrs[step][idx]

    solved: list[SolvedPosition] = []
    for cluster, combo_idx, combos in zip(clusters, chosen_indices, states_per_cluster):
        combo = combos[combo_idx]
        string_fret_map = {s: f for s, f in combo}
        left_fingers = assign_left_fingers_for_map(string_fret_map)
        used_right: set[RightAction] = set()

        # Assign bass strings first so thumb grabs lowest string cleanly
        paired = sorted(zip(cluster, combo), key=lambda item: -item[1][0])
        for raw, (string_num, fret) in paired:
            r_act = assign_right_hand_pluck(string_num, used_right)
            used_right.add(r_act)
            solved.append(
                SolvedPosition(
                    raw=raw,
                    string=string_num,
                    fret=fret,
                    left_finger=left_fingers.get(string_num, 0),
                    right_action=r_act,
                )
            )

    return solved
