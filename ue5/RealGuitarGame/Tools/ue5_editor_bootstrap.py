"""
UE5 Headless Editor Bootstrap Script for RealGuitarGame.

Imports guitars, 26 Poly Haven CC0 photogrammetry models, 4 custom architectural/terrain GLBs,
2K PBR textures, UI icon textures, synthesized WAV multi-samples & ambience loop, creates
Enhanced Input assets and animated lake/PBR materials, and builds /Game/Maps/AcousticSanctuary.
"""

from __future__ import annotations

from pathlib import Path
import sys
import unreal

TOOLS_DIR = Path(__file__).resolve().parent
if str(TOOLS_DIR) not in sys.path:
    sys.path.insert(0, str(TOOLS_DIR))

import ue5_material_factory as mat_fac
import ue5_sanctuary_scene as scene


def create_input_action(
    asset_tools: unreal.AssetTools,
    folder: str,
    name: str,
    value_type: unreal.InputActionValueType,
) -> unreal.InputAction:
    full_path = f"{folder}/{name}"
    existing = mat_fac.load_asset_if_exists(full_path)
    if existing:
        return existing
    ia = asset_tools.create_asset(name, folder, unreal.InputAction, unreal.InputAction_Factory())
    ia.set_editor_property("value_type", value_type)
    unreal.EditorAssetLibrary.save_asset(full_path)
    return ia


def bootstrap() -> None:
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    content_dir = Path(unreal.Paths.project_content_dir())

    for d in (
        "/Game/Input",
        "/Game/Models",
        "/Game/Models/Props",
        "/Game/Models/Custom",
        "/Game/Materials",
        "/Game/Materials/Textures",
        "/Game/Audio",
        "/Game/Audio/Samples",
        "/Game/UI/Textures",
        "/Game/Maps",
        "/Game/Blueprints",
    ):
        mat_fac.ensure_dir(d)

    # 1. Enhanced Input Assets
    ia_move = create_input_action(asset_tools, "/Game/Input", "IA_Move", unreal.InputActionValueType.AXIS2D)
    ia_look = create_input_action(asset_tools, "/Game/Input", "IA_Look", unreal.InputActionValueType.AXIS2D)
    ia_interact = create_input_action(asset_tools, "/Game/Input", "IA_Interact", unreal.InputActionValueType.BOOLEAN)
    ia_play = create_input_action(asset_tools, "/Game/Input", "IA_TogglePlay", unreal.InputActionValueType.BOOLEAN)
    ia_cam = create_input_action(asset_tools, "/Game/Input", "IA_CycleCamera", unreal.InputActionValueType.BOOLEAN)
    ia_next = create_input_action(asset_tools, "/Game/Input", "IA_NextSong", unreal.InputActionValueType.BOOLEAN)
    ia_pause = create_input_action(asset_tools, "/Game/Input", "IA_PauseMenu", unreal.InputActionValueType.BOOLEAN)
    ia_hud = create_input_action(asset_tools, "/Game/Input", "IA_ToggleHud", unreal.InputActionValueType.BOOLEAN)
    ia_mood = create_input_action(asset_tools, "/Game/Input", "IA_CycleMood", unreal.InputActionValueType.BOOLEAN)

    imc_path = "/Game/Input/IMC_AcousticChill"
    imc = mat_fac.load_asset_if_exists(imc_path)
    if not imc:
        imc = asset_tools.create_asset(
            "IMC_AcousticChill",
            "/Game/Input",
            unreal.InputMappingContext,
            unreal.InputMappingContext_Factory(),
        )
        for key_name, action in (
            ("W", ia_move),
            ("S", ia_move),
            ("A", ia_move),
            ("D", ia_move),
            ("Mouse2D", ia_look),
            ("E", ia_interact),
            ("P", ia_play),
            ("C", ia_cam),
            ("N", ia_next),
            ("Escape", ia_pause),
            ("H", ia_hud),
            ("T", ia_mood),
        ):
            imc.map_key(action, unreal.Key(key_name))
        unreal.EditorAssetLibrary.save_asset(imc_path)

    # 2. Import Acoustic Guitar Full mesh if missing
    guitar_dest = "/Game/Models/AcousticGuitar_Full"
    full_glb = content_dir / "Models" / "AcousticGuitar_Full.glb"
    if full_glb.exists() and not unreal.EditorAssetLibrary.does_directory_exist(guitar_dest):
        mat_fac.ensure_dir(guitar_dest)
        mat_fac.import_asset(asset_tools, full_glb, guitar_dest, "AcousticGuitar_Full")

    # 3. Import crisp UI textures (re-configure glass 9-slice & crisp icons)
    ui_tex_dir = content_dir / "UI" / "Textures"
    if ui_tex_dir.exists():
        for png in sorted(ui_tex_dir.glob("*.png")):
            tex_path = f"/Game/UI/Textures/{png.stem}.{png.stem}"
            tex = mat_fac.load_asset_if_exists(tex_path)
            if not tex:
                mat_fac.import_asset(asset_tools, png, "/Game/UI/Textures", png.stem, replace_existing=False)
                tex = mat_fac.load_asset_if_exists(tex_path)
            if isinstance(tex, unreal.Texture2D):
                mat_fac.configure_texture_asset(tex, "ui")

    # 4. Import Multi-Sample Guitar WAVs & Sanctuary Ambience WAV
    samples_dir = content_dir / "Audio" / "Samples"
    if samples_dir.exists():
        for wav in sorted(samples_dir.glob("*.wav")):
            wav_dest = f"/Game/Audio/Samples/{wav.stem}.{wav.stem}"
            if not unreal.EditorAssetLibrary.does_asset_exist(wav_dest):
                mat_fac.import_asset(asset_tools, wav, "/Game/Audio/Samples", wav.stem, replace_existing=False)

    ambience_wav = content_dir / "Audio" / "Sanctuary_Ambience.wav"
    ambience_asset_path = "/Game/Audio/Sanctuary_Ambience.Sanctuary_Ambience"
    ambience_sound = mat_fac.load_asset_if_exists(ambience_asset_path)
    if not ambience_sound and ambience_wav.exists():
        mat_fac.import_asset(asset_tools, ambience_wav, "/Game/Audio", "Sanctuary_Ambience", replace_existing=False)
        ambience_sound = mat_fac.load_asset_if_exists(ambience_asset_path)

    # 5. Import 2K PBR Textures & Build Photorealistic + Animated Lake Materials
    tex_map = mat_fac.import_sanctuary_textures(asset_tools, content_dir)
    materials = {
        "shore": mat_fac.build_pbr_textured_material(
            asset_tools,
            "/Game/Materials",
            "M_MossyShore",
            tex_map.get("forest_ground_04_diff_2k"),
            tex_map.get("forest_ground_04_nor_dx_2k"),
            tex_map.get("forest_ground_04_rough_2k"),
            tex_map.get("forest_ground_04_ao_2k"),
            uv_tiling=1.0,
            specular=0.22,
        ),
        "deck": mat_fac.build_pbr_textured_material(
            asset_tools,
            "/Game/Materials",
            "M_WarmOakDeck",
            tex_map.get("wood_floor_deck_diff_2k"),
            tex_map.get("wood_floor_deck_nor_dx_2k"),
            tex_map.get("wood_floor_deck_rough_2k"),
            tex_map.get("wood_floor_deck_ao_2k"),
            uv_tiling=1.0,
            roughness_mult=0.92,
            specular=0.36,
            tint_color=(0.82, 0.72, 0.62, 1.0),
        ),
        "rock": mat_fac.build_pbr_textured_material(
            asset_tools,
            "/Game/Materials",
            "M_RockyCliff",
            tex_map.get("rock_face_03_diff_2k"),
            tex_map.get("rock_face_03_nor_dx_2k"),
            tex_map.get("rock_face_03_rough_2k"),
            tex_map.get("rock_face_03_ao_2k"),
            uv_tiling=1.0,
            specular=0.30,
            tint_color=(0.56, 0.58, 0.62, 1.0),
        ),
        "lake": mat_fac.build_animated_lake_material(
            asset_tools,
            "/Game/Materials",
            "M_CalmLake",
            tex_map.get("T_Water_N1"),
            tex_map.get("T_Water_N2"),
        ),
        "ember": mat_fac.build_ember_material(
            asset_tools,
            "/Game/Materials",
            "M_CampfireEmber",
            tex_map.get("T_EmberGlow"),
        ),
    }

    # 6. Import Custom Architectural/Terrain GLBs & All 26 Poly Haven 2K Models
    custom_meshes, poly_meshes = scene.import_all_sanctuary_meshes(asset_tools, content_dir)

    # 7. Assemble & Save AcousticSanctuary.umap
    saved_ok = scene.build_sanctuary_level(
        asset_tools=asset_tools,
        content_dir=content_dir,
        custom_meshes=custom_meshes,
        poly_meshes=poly_meshes,
        materials=materials,
        ambience_sound=ambience_sound,
    )
    unreal.log(f"RealGuitarGame AcousticSanctuary overhaul complete! saved={saved_ok}")


if __name__ == "__main__":
    bootstrap()
