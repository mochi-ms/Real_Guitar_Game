from pathlib import Path
import struct
import tempfile
import unittest
import wave

from engine.audio_synth import synthesize_performance_to_wav
from engine.parsers.chord_chart_parser import parse_chord_chart_data


class TestAudioSynth(unittest.TestCase):
    def test_synthesize_wav_produces_non_silent_stereo_pcm(self) -> None:
        perf = parse_chord_chart_data(
            {
                "title": "Test Strum",
                "bpm": 110,
                "style": "strum",
                "pattern": "8beat_folk",
                "measures": ["C", "G"],
            }
        )
        with tempfile.TemporaryDirectory() as tmp:
            wav_path = Path(tmp) / "test.wav"
            synthesize_performance_to_wav(perf, wav_path, sample_rate=22050)
            self.assertTrue(wav_path.exists())
            with wave.open(str(wav_path), "rb") as wf:
                self.assertEqual(wf.getnchannels(), 2)
                self.assertEqual(wf.getsampwidth(), 2)
                self.assertEqual(wf.getframerate(), 22050)
                frames = wf.readframes(min(wf.getnframes(), 22050))
            samples = struct.unpack(f"<{len(frames) // 2}h", frames)
            peak = max(abs(s) for s in samples)
            self.assertGreater(peak, 2000)


if __name__ == "__main__":
    unittest.main()
