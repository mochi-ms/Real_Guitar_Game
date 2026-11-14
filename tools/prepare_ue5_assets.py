"""Prepare compiled scores, calibrated 3D guitar GLB, and acoustic multi-sample WAV bank for UE5."""

from __future__ import annotations

from pathlib import Path

from engine.compiler import compile_score
from engine.humanizer import humanize_performance
from engine.soundbank import export_ue5_multisample_bank
from tools.generate_guitar_glb import export_acoustic_guitar_combined_glb, export_acoustic_guitar_glb
from tools.generate_ui_textures import generate_ui_texture_pack

REPO_ROOT = Path(__file__).resolve().parent.parent
UE5_CONTENT = REPO_ROOT / "ue5" / "RealGuitarGame" / "Content"


def prepare_all(include_audio_samples: bool = True) -> None:
    scores_dir = UE5_CONTENT / "Scores"
    scores_dir.mkdir(parents=True, exist_ok=True)

    samples_dir = REPO_ROOT / "samples"
    supported = {".musicxml", ".xml", ".mxl", ".gp", ".gpif", ".gp3", ".gp4", ".gp5", ".mid", ".midi", ".rgchart"}

    for src in sorted(samples_dir.iterdir()):
        if src.is_file() and src.suffix.lower() in supported:
            perf = humanize_performance(compile_score(src))
            out_json = scores_dir / f"{src.stem}.performance.json"
            out_json.write_text(perf.to_json(), encoding="utf-8")
            print(f"[OK] Compiled score -> {out_json.relative_to(REPO_ROOT)}")

    glb_path = UE5_CONTENT / "Models" / "AcousticGuitar_Reference.glb"
    export_acoustic_guitar_glb(glb_path)
    full_glb_path = UE5_CONTENT / "Models" / "AcousticGuitar_Full.glb"
    export_acoustic_guitar_combined_glb(full_glb_path)
    print(f"[OK] Exported calibrated 3D guitar GLBs -> {glb_path.name}, {full_glb_path.name}")

    ui_paths = generate_ui_texture_pack(UE5_CONTENT / "UI" / "Textures")
    print(f"[OK] Generated {len(ui_paths)} UI icon/panel PNG textures -> Content/UI/Textures/")

    if include_audio_samples:
        map_path = export_ue5_multisample_bank(UE5_CONTENT / "Audio")
        print(f"[OK] Exported UE5 multi-sample WAV bank -> {map_path.relative_to(REPO_ROOT)}")


if __name__ == "__main__":
    prepare_all(include_audio_samples=True)
