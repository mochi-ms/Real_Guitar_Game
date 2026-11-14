"""Multi-sample acoustic guitar WAV generator for UE5 Harmonix FusionPatch / MetaSounds."""

from __future__ import annotations

import json
from pathlib import Path
import random

from engine.audio_synth import synthesize_performance_to_wav
from engine.models import GuitarEvent, GuitarPerformance, midi_to_note_name


def export_ue5_multisample_bank(
    output_dir: str | Path,
    min_midi: int = 40,
    max_midi: int = 84,
    step: int = 2,
    sample_rate: int = 44100,
) -> Path:
    """
    Export individual single-note acoustic guitar WAVs across MIDI range E2..C6
    plus a multisample_map.json for UE5 Harmonix FusionPatch / MetaSound WavePlayer mapping.
    """
    out_root = Path(output_dir)
    samples_dir = out_root / "Samples"
    samples_dir.mkdir(parents=True, exist_ok=True)

    mappings: list[dict[str, object]] = []
    velocity_layers = [("soft", 0.45, 1, 75), ("hard", 0.90, 76, 127)]

    for midi_note in range(min_midi, max_midi + 1, step):
        note_name = midi_to_note_name(midi_note).replace("#", "s")
        for layer_name, vel, min_vel, max_vel in velocity_layers:
            wav_name = f"Acoustic_{midi_note:03d}_{note_name}_{layer_name}.wav"
            wav_path = samples_dir / wav_name

            # Map roughly to closest string for realistic timbre
            string_num = max(1, min(6, 6 - ((midi_note - 40) // 5)))
            perf = GuitarPerformance(
                title=wav_name,
                source_format="rgchart",
                bpm=60.0,
                events=[
                    GuitarEvent(
                        id=0,
                        time_sec=0.0,
                        duration_sec=2.2,
                        string=string_num,
                        fret=0,
                        midi_note=midi_note,
                        velocity=vel,
                        left_finger=0,
                        right_action="pluck_index",
                        articulation="let_ring",
                    )
                ],
            )
            synthesize_performance_to_wav(
                perf,
                wav_path,
                sample_rate=sample_rate,
                seed=midi_note * 17 + (1 if layer_name == "soft" else 2),
            )
            mappings.append(
                {
                    "root_midi": midi_note,
                    "min_midi": max(0, midi_note - (step - 1)),
                    "max_midi": min(127, midi_note + (step - 1)),
                    "min_velocity": min_vel,
                    "max_velocity": max_vel,
                    "layer": layer_name,
                    "wav_file": f"Samples/{wav_name}",
                }
            )

    # Add muted percussive chuck sample
    chuck_name = "Acoustic_Percussive_Chuck.wav"
    chuck_perf = GuitarPerformance(
        title=chuck_name,
        source_format="rgchart",
        bpm=120.0,
        events=[
            GuitarEvent(
                id=0,
                time_sec=0.0,
                duration_sec=0.08,
                string=s,
                fret=0,
                midi_note=45 + (6 - s) * 4,
                velocity=0.72,
                left_finger=0,
                right_action="percussive_slap",
                articulation="dead_note",
            )
            for s in (4, 3, 2)
        ],
    )
    synthesize_performance_to_wav(chuck_perf, samples_dir / chuck_name, sample_rate=sample_rate, seed=random.Random(99).randint(1, 999))

    map_path = out_root / "multisample_map.json"
    map_path.write_text(
        json.dumps(
            {
                "instrument": "AcousticSteelGuitar",
                "sample_rate": sample_rate,
                "percussive_chuck": f"Samples/{chuck_name}",
                "zones": mappings,
            },
            indent=2,
        ),
        encoding="utf-8",
    )
    return map_path
