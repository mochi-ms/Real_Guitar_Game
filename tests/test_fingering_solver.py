import unittest

from engine.fingering_solver import (
    RawMidiNote,
    assign_left_fingers_for_shape,
    candidate_positions,
    solve_fingering_sequence,
)
from engine.models import STANDARD_TUNING_MIDI, fret_to_midi


class TestFingeringSolver(unittest.TestCase):
    def test_candidate_positions_open_and_fretted(self) -> None:
        cands = candidate_positions(64)  # E4
        self.assertIn((1, 0), cands)
        self.assertIn((2, 5), cands)

    def test_simultaneous_chord_avoids_string_collisions(self) -> None:
        # C major triad + bass: C3(48), E3(52), G3(55), C4(60), E4(64)
        chord_notes = [
            RawMidiNote(0.0, 1.0, 48),
            RawMidiNote(0.0, 1.0, 52),
            RawMidiNote(0.0, 1.0, 55),
            RawMidiNote(0.0, 1.0, 60),
            RawMidiNote(0.0, 1.0, 64),
        ]
        solved = solve_fingering_sequence(chord_notes)
        self.assertEqual(len(solved), 5)
        strings = [s.string for s in solved]
        self.assertEqual(len(set(strings)), 5)
        for item in solved:
            self.assertEqual(
                fret_to_midi(item.string, item.fret, STANDARD_TUNING_MIDI),
                item.raw.midi_note,
            )

    def test_chord_shape_finger_assignment_open_c(self) -> None:
        # C chord (string 1..6): 0, 1, 0, 2, 3, -1
        fingers = assign_left_fingers_for_shape([0, 1, 0, 2, 3, -1])
        self.assertEqual(fingers[0], 0)  # open high E
        self.assertEqual(fingers[1], 1)  # B string fret 1 -> index
        self.assertEqual(fingers[2], 0)  # open G
        self.assertEqual(fingers[3], 2)  # D string fret 2 -> middle
        self.assertEqual(fingers[4], 3)  # A string fret 3 -> ring
        self.assertEqual(fingers[5], 0)  # muted low E


if __name__ == "__main__":
    unittest.main()
