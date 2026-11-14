"""
Unreal Engine 5 Editor Python Automation Script.
Run inside UE5 Editor via: Tools -> Execute Python Script
Creates Enhanced Input assets, imports AcousticGuitar_Reference.glb & WAV samples,
and builds a showcase level with a bench seat + GuitarPerformerActor.
"""

from __future__ import annotations

from pathlib import Path
import unreal


def ensure_dir(content_path: str) -> None:
    if not unreal.EditorAssetLibrary.does_directory_exist(content_path):
        unreal.EditorAssetLibrary.make_directory(content_path)


def create_input_action(asset_tools: unreal.AssetTools, folder: str, name: str, value_type: unreal.InputActionValueType) -> unreal.InputAction:
    full_path = f"{folder}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(full_path):
        return unreal.EditorAssetLibrary.load_asset(full_path)
    ia = asset_tools.create_asset(name, folder, unreal.InputAction, unreal.InputActionFactory())
    ia.set_editor_property("value_type", value_type)
    unreal.EditorAssetLibrary.save_asset(full_path)
    return ia


def import_file(asset_tools: unreal.AssetTools, src_file: Path, dest_path: str, asset_name: str | None = None) -> None:
    if not src_file.exists():
        unreal.log_warning(f"Missing file to import: {src_file}")
        return
    task = unreal.AssetImportTask()
    task.filename = str(src_file)
    task.destination_path = dest_path
    if asset_name:
        task.destination_name = asset_name
    task.automated = True
    task.save = True
    task.replace_existing = True
    asset_tools.import_asset_tasks([task])


def bootstrap() -> None:
    project_dir = Path(unreal.Paths.project_dir()).resolve()
    content_dir = project_dir / "Content"
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

    for sub in ("/Game/Input", "/Game/Models", "/Game/Audio/Samples", "/Game/UI/Textures", "/Game/Maps", "/Game/Blueprints"):
        ensure_dir(sub)

    # 1. Enhanced Input Actions + Mapping Context
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
    if not unreal.EditorAssetLibrary.does_asset_exist(imc_path):
        imc = asset_tools.create_asset("IMC_AcousticChill", "/Game/Input", unreal.InputMappingContext, unreal.InputMappingContext_Factory())
        imc.map_key(ia_interact, unreal.Key(key_name="E"))
        imc.map_key(ia_play, unreal.Key(key_name="SpaceBar"))
        imc.map_key(ia_cam, unreal.Key(key_name="C"))
        imc.map_key(ia_next, unreal.Key(key_name="N"))
        imc.map_key(ia_pause, unreal.Key(key_name="Escape"))
        imc.map_key(ia_hud, unreal.Key(key_name="H"))
        imc.map_key(ia_mood, unreal.Key(key_name="T"))
        unreal.EditorAssetLibrary.save_asset(imc_path)

    # 2. Import Reference Acoustic Guitar GLB
    glb_file = content_dir / "Models" / "AcousticGuitar_Reference.glb"
    import_file(asset_tools, glb_file, "/Game/Models", "AcousticGuitar_Reference")

    # 3. Import generated crisp UI icon & 9-slice panel textures
    ui_tex_dir = content_dir / "UI" / "Textures"
    if ui_tex_dir.exists():
        for png in sorted(ui_tex_dir.glob("*.png")):
            import_file(asset_tools, png, "/Game/UI/Textures")

    # 4. Import WAV multi-samples if generated
    samples_dir = content_dir / "Audio" / "Samples"
    if samples_dir.exists():
        for wav in sorted(samples_dir.glob("*.wav")):
            import_file(asset_tools, wav, "/Game/Audio/Samples")

    # 5. Spawn starter GuitarPerformerActor, InteractableSeatActor, and SanctuaryAtmosphereActor
    actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actor_sub.spawn_actor_from_class(
        unreal.SanctuaryAtmosphereActor, unreal.Vector(0.0, 0.0, 0.0), unreal.Rotator(0.0, 0.0, 0.0)
    )
    performer = actor_sub.spawn_actor_from_class(
        unreal.GuitarPerformerActor, unreal.Vector(220.0, 0.0, 92.0), unreal.Rotator(0.0, 0.0, 0.0)
    )
    seat = actor_sub.spawn_actor_from_class(
        unreal.InteractableSeatActor, unreal.Vector(140.0, 0.0, 0.0), unreal.Rotator(0.0, 0.0, 0.0)
    )
    if seat and performer:
        seat.set_editor_property("linked_performer", performer)

    unreal.log("RealGuitarGame editor bootstrap complete.")


if __name__ == "__main__":
    bootstrap()
