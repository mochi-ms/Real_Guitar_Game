from pathlib import Path
import struct
import tempfile
import unittest
import zipfile

from engine.compiler import compile_score
from engine.parsers.midi_parser import encode_vlq

REPO_ROOT = Path(__file__).resolve().parent.parent


def _build_test_midi_bytes() -> bytes:
    ppqn = 480
    # Tempo 100 BPM = 600,000 us/quarter
    events = bytearray()
    events.extend(encode_vlq(0) + b"\xFF\x51\x03" + (600_000).to_bytes(3, "big"))
    # Play A2(45), E3(52), A3(57), C#4(61), E4(64) arpeggio
    pitches = [45, 52, 57, 61, 64]
    for idx, p in enumerate(pitches):
        events.extend(encode_vlq(0 if idx == 0 else 240) + bytes([0x90, p, 96]))
        events.extend(encode_vlq(240) + bytes([0x80, p, 0]))
    events.extend(encode_vlq(0) + b"\xFF\x2F\x00")

    header = b"MThd" + struct.pack(">IHHH", 6, 0, 1, ppqn)
    track = b"MTrk" + struct.pack(">I", len(events)) + bytes(events)
    return header + track


def _build_test_gp_archive(path: Path) -> None:
    gpif_xml = """<?xml version="1.0" encoding="utf-8"?>
<GPIF>
  <Score><Title>Test GP Track</Title><Artist>Tester</Artist></Score>
  <MasterTrack>
    <Automations>
      <Automation><Type>Tempo</Type><Value>96 2</Value></Automation>
    </Automations>
  </MasterTrack>
  <Tracks>
    <Track id="0">
      <Name>Acoustic</Name>
      <Properties>
        <Property name="Tuning"><Pitches>40 45 50 55 59 64</Pitches></Property>
      </Properties>
    </Track>
  </Tracks>
  <MasterBars>
    <MasterBar><Time>4/4</Time><Bars>0</Bars></MasterBar>
  </MasterBars>
  <Bars><Bar id="0"><Voices>0</Voices></Bar></Bars>
  <Voices><Voice id="0"><Beats>0 1</Beats></Voice></Voices>
  <Beats>
    <Beat id="0">
      <Rhythm ref="0"/>
      <Notes>0</Notes>
    </Beat>
    <Beat id="1">
      <Rhythm ref="0"/>
      <Notes>1 2</Notes>
      <Properties><Property name="Brush"><Direction>Down</Direction></Property></Properties>
    </Beat>
  </Beats>
  <Notes>
    <Note id="0">
      <Properties>
        <Property name="String"><String>1</String></Property>
        <Property name="Fret"><Fret>3</Fret></Property>
      </Properties>
    </Note>
    <Note id="1">
      <Properties>
        <Property name="String"><String>3</String></Property>
        <Property name="Fret"><Fret>2</Fret></Property>
      </Properties>
    </Note>
    <Note id="2">
      <Properties>
        <Property name="String"><String>4</String></Property>
        <Property name="Fret"><Fret>1</Fret></Property>
      </Properties>
    </Note>
  </Notes>
  <Rhythms>
    <Rhythm id="0"><NoteValue>Quarter</NoteValue></Rhythm>
  </Rhythms>
</GPIF>
"""
    with zipfile.ZipFile(path, "w") as zf:
        zf.writestr("Content/score.gpif", gpif_xml.encode("utf-8"))


class TestScoreParsers(unittest.TestCase):
    def test_musicxml_fingerstyle_sample(self) -> None:
        perf = compile_score(REPO_ROOT / "samples" / "fingerstyle_canon_intro.musicxml")
        self.assertEqual(perf.source_format, "musicxml")
        self.assertEqual(perf.play_style, "hybrid")  # fingerstyle + final G down arpeggiate strum
        self.assertGreater(len(perf.events), 25)
        first = perf.events[0]
        self.assertEqual(first.string, 5)
        self.assertEqual(first.fret, 3)
        self.assertEqual(first.midi_note, 48)  # C3

    def test_rgchart_hybrid_sample(self) -> None:
        perf = compile_score(REPO_ROOT / "samples" / "acoustic_ballad_strum.rgchart")
        self.assertEqual(perf.source_format, "rgchart")
        self.assertEqual(perf.play_style, "hybrid")
        self.assertEqual(len(perf.chords), 8)
        actions = {e.right_action for e in perf.events}
        self.assertIn("pluck_thumb", actions)
        self.assertIn("strum_down", actions)
        self.assertIn("strum_up", actions)

    def test_midi_binary_parsing(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            mid_path = Path(tmp) / "arp.mid"
            mid_path.write_bytes(_build_test_midi_bytes())
            perf = compile_score(mid_path)
            self.assertEqual(perf.source_format, "midi")
            self.assertAlmostEqual(perf.bpm, 100.0, places=1)
            self.assertEqual([e.midi_note for e in perf.events], [45, 52, 57, 61, 64])

    def test_guitar_pro_gp_archive(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            gp_path = Path(tmp) / "test.gp"
            _build_test_gp_archive(gp_path)
            perf = compile_score(gp_path)
            self.assertEqual(perf.source_format, "guitar_pro")
            self.assertEqual(perf.bpm, 96.0)
            self.assertEqual(len(perf.events), 3)
            # GPIF string 1 (A string) -> guitar string 5, fret 3 -> C3 (48)
            self.assertEqual(perf.events[0].string, 5)
            self.assertEqual(perf.events[0].fret, 3)
            self.assertEqual(perf.events[0].midi_note, 48)
            self.assertEqual(perf.events[1].right_action, "strum_down")


if __name__ == "__main__":
    unittest.main()
