"""
Unreal Engine 5.8 Editor Python Automation Script.
Run via UnrealEditor-Cmd.exe -run=pythonscript -script=... or Tools -> Execute Python Script.
Imports unified AcousticGuitar_Full GLB, lightweight Poly Haven CC0 2K nature/furniture glTF props,
UI icon textures, WAV multi-samples, creates Enhanced Input assets,
and builds the complete playable AcousticSanctuary showcase level (.umap).
"""

from __future__ import annotations

import math
from pathlib import Path
import unreal


def ensure_dir(content_path: str) -> None:
    if not unreal.EditorAssetLibrary.does_directory_exist(content_path):
        unreal.EditorAssetLibrary.make_directory(content_path)


def make_rotator(pitch: float = 0.0, yaw: float = 0.0, roll: float = 0.0) -> unreal.Rotator:
    """Explicitly assign rotator axes to avoid UE Python positional argument ambiguity."""
    rot = unreal.Rotator()
    rot.pitch = pitch
    rot.yaw = yaw
    rot.roll = roll
    return rot


def rotate_xy_by_yaw(x: float, y: float, yaw_deg: float) -> tuple[float, float]:
    """Rotate a 2D vector by UE Left-Handed Z-Up Yaw (degrees)."""
    rad = math.radians(yaw_deg)
    c = math.cos(rad)
    s = math.sin(rad)
    return (x * c - y * s, x * s + y * c)


def find_facing_yaw(local_front_dir: unreal.Vector, desired_world_dir: unreal.Vector) -> float:
    """Find integer yaw (degrees) that aligns a mesh's local front direction closest to desired_world_dir."""
    goal_len = math.hypot(desired_world_dir.x, desired_world_dir.y) or 1.0
    gx, gy = desired_world_dir.x / goal_len, desired_world_dir.y / goal_len
    best_yaw = 0.0
    best_dot = -2.0
    for yaw in range(0, 360, 5):
        wx, wy = rotate_xy_by_yaw(local_front_dir.x, local_front_dir.y, float(yaw))
        dot = wx * gx + wy * gy
        if dot > best_dot:
            best_dot = dot
            best_yaw = float(yaw)
    return best_yaw


def create_input_action(
    asset_tools: unreal.AssetTools,
    folder: str,
    name: str,
    value_type: unreal.InputActionValueType,
) -> unreal.InputAction:
    full_path = f"{folder}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(full_path):
        return unreal.EditorAssetLibrary.load_asset(full_path)
    ia = asset_tools.create_asset(name, folder, unreal.InputAction, unreal.InputAction_Factory())
    ia.set_editor_property("value_type", value_type)
    unreal.EditorAssetLibrary.save_asset(full_path)
    return ia


def import_file(
    asset_tools: unreal.AssetTools,
    src_file: Path,
    dest_path: str,
    asset_name: str | None = None,
) -> list[str]:
    if not src_file.exists():
        unreal.log_warning(f"Missing file to import: {src_file}")
        return []
    task = unreal.AssetImportTask()
    task.filename = str(src_file)
    task.destination_path = dest_path
    if asset_name:
        task.destination_name = asset_name
    task.automated = True
    task.save = True
    task.replace_existing = True
    asset_tools.import_asset_tasks([task])
    return list(task.imported_object_paths or [])


def find_all_static_meshes(folder: str) -> dict[str, unreal.StaticMesh]:
    meshes: dict[str, unreal.StaticMesh] = {}
    if not unreal.EditorAssetLibrary.does_directory_exist(folder):
        return meshes
    for asset_path in sorted(unreal.EditorAssetLibrary.list_assets(folder, recursive=True)):
        asset = unreal.EditorAssetLibrary.load_asset(asset_path)
        if isinstance(asset, unreal.StaticMesh):
            meshes[asset.get_name()] = asset
    return meshes


def pick_primary_mesh(meshes: dict[str, unreal.StaticMesh], preferred_name: str) -> unreal.StaticMesh | None:
    if not meshes:
        return None
    if preferred_name in meshes:
        return meshes[preferred_name]
    for k, m in meshes.items():
        if preferred_name.lower() in k.lower() and not any(
            skip in k.lower() for skip in ("bolt", "door", "handle", "planks", "poles")
        ):
            return m
    return next(iter(meshes.values()))


def make_solid_color_material(
    asset_tools: unreal.AssetTools,
    folder: str,
    name: str,
    color: tuple[float, float, float, float],
    roughness: float = 0.6,
    metallic: float = 0.0,
) -> unreal.Material:
    mat_path = f"{folder}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(mat_path):
        mat = unreal.EditorAssetLibrary.load_asset(mat_path)
    else:
        mat = asset_tools.create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew())

    mat_edit = unreal.MaterialEditingLibrary
    mat_edit.delete_all_material_expressions(mat)

    col_node = mat_edit.create_material_expression(mat, unreal.MaterialExpressionConstant4Vector, -300, -100)
    col_node.set_editor_property("constant", unreal.LinearColor(*color))
    mat_edit.connect_material_property(col_node, "", unreal.MaterialProperty.MP_BASE_COLOR)

    rough_node = mat_edit.create_material_expression(mat, unreal.MaterialExpressionConstant, -300, 100)
    rough_node.set_editor_property("r", roughness)
    mat_edit.connect_material_property(rough_node, "", unreal.MaterialProperty.MP_ROUGHNESS)

    metal_node = mat_edit.create_material_expression(mat, unreal.MaterialExpressionConstant, -300, 200)
    metal_node.set_editor_property("r", metallic)
    mat_edit.connect_material_property(metal_node, "", unreal.MaterialProperty.MP_METALLIC)

    mat_edit.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(mat_path)
    return mat


def enable_complex_as_simple_collision(mesh: unreal.StaticMesh) -> None:
    """Ensure imported glTF props have accurate line-trace and walking collision."""
    body_setup = mesh.get_editor_property("body_setup")
    if body_setup:
        body_setup.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
        unreal.EditorAssetLibrary.save_loaded_asset(mesh)


def spawn_static_mesh_actor(
    actor_sub: unreal.EditorActorSubsystem,
    mesh: unreal.StaticMesh,
    location: unreal.Vector,
    rotation: unreal.Rotator | None = None,
    scale: unreal.Vector | None = None,
    label: str | None = None,
    material: unreal.MaterialInterface | None = None,
) -> unreal.StaticMeshActor:
    rot = rotation or make_rotator()
    sc = scale or unreal.Vector(1.0, 1.0, 1.0)
    actor = actor_sub.spawn_actor_from_class(unreal.StaticMeshActor, location, rot)
    comp = actor.static_mesh_component
    comp.set_static_mesh(mesh)
    actor.set_actor_scale3d(sc)
    if material:
        for slot in range(comp.get_num_materials()):
            comp.set_material(slot, material)
    if label:
        actor.set_actor_label(label)
    return actor


def spawn_centered_mesh(
    actor_sub: unreal.EditorActorSubsystem,
    mesh: unreal.StaticMesh,
    target_xy: tuple[float, float],
    ground_z: float | None = None,
    top_z: float | None = None,
    yaw: float = 0.0,
    scale: float = 1.0,
    label: str | None = None,
) -> unreal.StaticMeshActor:
    """Place a StaticMesh whose local pivot may be offset by centering XY bounds and aligning bottom/top Z."""
    bb = mesh.get_bounding_box()
    cx = 0.5 * (bb.min.x + bb.max.x) * scale
    cy = 0.5 * (bb.min.y + bb.max.y) * scale
    rot = make_rotator(yaw=yaw)
    sx, sy = rotate_xy_by_yaw(-cx, -cy, yaw)
    if top_z is not None:
        z_pos = top_z - bb.max.z * scale
    else:
        z_pos = (ground_z or 0.0) - bb.min.z * scale
    loc = unreal.Vector(target_xy[0] + sx, target_xy[1] + sy, z_pos)
    return spawn_static_mesh_actor(
        actor_sub,
        mesh,
        loc,
        rot,
        unreal.Vector(scale, scale, scale),
        label=label,
    )


def spawn_warm_point_light(
    actor_sub: unreal.EditorActorSubsystem,
    location: unreal.Vector,
    color_rgba: tuple[float, float, float, float],
    intensity: float,
    radius: float,
    label: str,
) -> None:
    light_actor = actor_sub.spawn_actor_from_class(unreal.PointLight, location, make_rotator())
    if not light_actor:
        return
    light_actor.set_actor_label(label)
    comp = light_actor.point_light_component
    if comp:
        comp.set_intensity(intensity)
        comp.set_attenuation_radius(radius)
        comp.set_light_color(unreal.LinearColor(*color_rgba))


def bootstrap() -> None:
    project_dir = Path(unreal.Paths.project_dir()).resolve()
    content_dir = project_dir / "Content"
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

    for sub in (
        "/Game/Input",
        "/Game/Models",
        "/Game/Models/Props",
        "/Game/Materials",
        "/Game/Audio/Samples",
        "/Game/UI/Textures",
        "/Game/Maps",
        "/Game/Blueprints",
    ):
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
    def make_key(name: str) -> unreal.Key:
        k = unreal.Key()
        k.import_text(name)
        return k

    if not unreal.EditorAssetLibrary.does_asset_exist(imc_path):
        imc = asset_tools.create_asset(
            "IMC_AcousticChill", "/Game/Input", unreal.InputMappingContext, unreal.InputMappingContext_Factory()
        )
        imc.map_key(ia_move, make_key("D"))
        imc.map_key(ia_look, make_key("Mouse2D"))
        imc.map_key(ia_interact, make_key("E"))
        imc.map_key(ia_play, make_key("SpaceBar"))
        imc.map_key(ia_cam, make_key("C"))
        imc.map_key(ia_next, make_key("N"))
        imc.map_key(ia_pause, make_key("Escape"))
        imc.map_key(ia_hud, make_key("H"))
        imc.map_key(ia_mood, make_key("T"))
        unreal.EditorAssetLibrary.save_asset(imc_path)

    # 2. Import Unified Single-Mesh Acoustic Guitar GLB
    guitar_dest = "/Game/Models/AcousticGuitar_Full"
    ensure_dir(guitar_dest)
    guitar_meshes = find_all_static_meshes(guitar_dest)
    if not guitar_meshes:
        full_glb = content_dir / "Models" / "AcousticGuitar_Full.glb"
        import_file(asset_tools, full_glb, "/Game/Models", "AcousticGuitar_Full")
        guitar_meshes = find_all_static_meshes(guitar_dest)

    guitar_mesh = pick_primary_mesh(guitar_meshes, "AcousticGuitar_Full")
    if guitar_mesh:
        bb = guitar_mesh.get_bounding_box()
        unreal.log(f"Imported Unified Guitar '{guitar_mesh.get_path_name()}' BBox: min={bb.min} max={bb.max}")

    # 3. Import curated Poly Haven CC0 glTF 2K models
    polyhaven_dir = content_dir / "ExternalAssets" / "PolyHaven"
    prop_mesh_sets: dict[str, dict[str, unreal.StaticMesh]] = {}
    if polyhaven_dir.exists():
        for asset_folder in sorted(polyhaven_dir.iterdir()):
            if not asset_folder.is_dir():
                continue
            gltfs = list(asset_folder.glob("*.gltf")) + list(asset_folder.glob("*.glb"))
            if not gltfs:
                continue
            dest_sub = f"/Game/Models/Props/{asset_folder.name}"
            ensure_dir(dest_sub)
            meshes_map = find_all_static_meshes(dest_sub)
            if not meshes_map:
                import_file(asset_tools, gltfs[0], dest_sub)
                meshes_map = find_all_static_meshes(dest_sub)
            for m_name, m_obj in meshes_map.items():
                enable_complex_as_simple_collision(m_obj)
            if meshes_map:
                prop_mesh_sets[asset_folder.name] = meshes_map
                primary = pick_primary_mesh(meshes_map, asset_folder.name)
                if primary:
                    bb = primary.get_bounding_box()
                    unreal.log(
                        f"CC0 Asset '{asset_folder.name}' ({len(meshes_map)} meshes), primary='{primary.get_name()}' "
                        f"min={bb.min} max={bb.max}"
                    )

    # 4. Import crisp UI textures
    ui_tex_dir = content_dir / "UI" / "Textures"
    if ui_tex_dir.exists():
        for png in sorted(ui_tex_dir.glob("*.png")):
            tex_dest = f"/Game/UI/Textures/{png.stem}"
            if not unreal.EditorAssetLibrary.does_asset_exist(tex_dest):
                import_file(asset_tools, png, "/Game/UI/Textures")

    # 5. Import WAV multi-samples for polyphonic fallback playback
    samples_dir = content_dir / "Audio" / "Samples"
    if samples_dir.exists():
        for wav in sorted(samples_dir.glob("*.wav")):
            wav_dest = f"/Game/Audio/Samples/{wav.stem}"
            if not unreal.EditorAssetLibrary.does_asset_exist(wav_dest):
                import_file(asset_tools, wav, "/Game/Audio/Samples")

    # 6. Create & populate photorealistic /Game/Maps/AcousticSanctuary level
    map_path = "/Game/Maps/AcousticSanctuary"
    created_world = unreal.EditorLoadingAndSavingUtils.new_blank_map(False)

    actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    # Sky atmosphere, volumetric clouds, post process, and mood lighting controller
    actor_sub.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0), make_rotator())
    actor_sub.spawn_actor_from_class(unreal.VolumetricCloud, unreal.Vector(0, 0, 0), make_rotator())
    ppv = actor_sub.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 0), make_rotator())
    if ppv:
        ppv.set_editor_property("unbound", True)

    actor_sub.spawn_actor_from_class(unreal.SanctuaryAtmosphereActor, unreal.Vector(0, 0, 0), make_rotator())

    # Ground bank, calm reflective lake water, and timber sub-deck safety floor
    plane_mesh = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Plane.Plane")
    cube_mesh = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Cube.Cube")
    shore_mat = make_solid_color_material(asset_tools, "/Game/Materials", "M_MossyShore", (0.10, 0.14, 0.08, 1.0), roughness=0.92)
    lake_mat = make_solid_color_material(asset_tools, "/Game/Materials", "M_CalmLake", (0.015, 0.04, 0.07, 1.0), roughness=0.05, metallic=0.18)
    deck_mat = make_solid_color_material(asset_tools, "/Game/Materials", "M_WarmOakDeck", (0.22, 0.13, 0.07, 1.0), roughness=0.62)

    if plane_mesh:
        spawn_static_mesh_actor(
            actor_sub, plane_mesh, unreal.Vector(-1650.0, 0.0, 0.0),
            scale=unreal.Vector(34.0, 90.0, 1.0), label="MossyShoreBank", material=shore_mat
        )
        spawn_static_mesh_actor(
            actor_sub, plane_mesh, unreal.Vector(3600.0, 0.0, -38.0),
            scale=unreal.Vector(90.0, 140.0, 1.0), label="CalmLakeSurface", material=lake_mat
        )

    deck_top_z = 24.0
    if cube_mesh:
        # Solid timber apron & gentle shore entry ramp cleanly tucked inside pier width
        spawn_static_mesh_actor(
            actor_sub, cube_mesh, unreal.Vector(145.0, 0.0, 10.5),
            scale=unreal.Vector(8.6, 2.35, 0.22), label="PierUnderDeckSlab", material=deck_mat
        )
        ramp = spawn_static_mesh_actor(
            actor_sub, cube_mesh, unreal.Vector(-355.0, 0.0, 10.5),
            rotation=make_rotator(pitch=10.5), scale=unreal.Vector(1.3, 2.3, 0.08),
            label="ShoreEntryRamp", material=deck_mat
        )
        if ramp and ramp.get_actor_forward_vector().z < 0.0:
            ramp.set_actor_rotation(make_rotator(pitch=-10.5), False)

    if "modular_wooden_pier" in prop_mesh_sets:
        pier_set = prop_mesh_sets["modular_wooden_pier"]
        sec2 = pier_set.get("modular_wooden_pier_section_02") or pick_primary_mesh(pier_set, "section_02")
        sec3 = pier_set.get("modular_wooden_pier_section_03") or sec2
        if sec2:
            spawn_centered_mesh(
                actor_sub, sec2, (-145.0, 0.0), top_z=deck_top_z + 1.5, yaw=90.0, scale=1.05, label="LakePier_Shore"
            )
        if sec3:
            spawn_centered_mesh(
                actor_sub, sec3, (145.0, 0.0), top_z=deck_top_z + 1.5, yaw=90.0, scale=1.05, label="LakePier_MainStage"
            )
            spawn_centered_mesh(
                actor_sub, sec3, (435.0, 0.0), top_z=deck_top_z + 1.5, yaw=90.0, scale=1.05, label="LakePier_End"
            )

    # Player spawn on shoreline pier entrance facing toward the guitar & sunset lake
    actor_sub.spawn_actor_from_class(
        unreal.PlayerStart, unreal.Vector(-250.0, 0.0, 96.0), make_rotator(yaw=0.0)
    )

    # Side wooden table (0.62 scale -> ~62 cm height) + warm lantern + Acoustic Guitar performer
    table_top_z = deck_top_z + 62.0
    if "round_wooden_table_01" in prop_mesh_sets:
        tmesh = pick_primary_mesh(prop_mesh_sets["round_wooden_table_01"], "round_wooden_table_01")
        if tmesh:
            tbb = tmesh.get_bounding_box()
            table_scale = 0.62
            spawn_centered_mesh(
                actor_sub, tmesh, (235.0, 15.0), ground_z=deck_top_z, scale=table_scale, label="GuitarSideTable"
            )
            table_top_z = deck_top_z + (tbb.max.z - tbb.min.z) * table_scale

    if "wooden_lantern_01" in prop_mesh_sets:
        lmesh = pick_primary_mesh(prop_mesh_sets["wooden_lantern_01"], "wooden_lantern_01")
        if lmesh:
            spawn_centered_mesh(
                actor_sub, lmesh, (242.0, 48.0), ground_z=table_top_z, scale=1.0, label="WarmLantern"
            )
            spawn_warm_point_light(
                actor_sub,
                unreal.Vector(242.0, 48.0, table_top_z + 20.0),
                (1.0, 0.68, 0.34, 1.0),
                intensity=2800.0,
                radius=460.0,
                label="WarmLanternLight",
            )

    # Orient HeroAcousticGuitar tilted upright (~26 deg toward bench player on -X)
    guitar_rot = make_rotator(yaw=-105.0, roll=-26.0)

    performer = actor_sub.spawn_actor_from_class(
        unreal.GuitarPerformerActor,
        unreal.Vector(222.0, -4.0, table_top_z + 24.0),
        guitar_rot,
    )
    if performer:
        performer.set_actor_label("HeroAcousticGuitar")
        if performer.get_actor_up_vector().x > 0.0:
            performer.set_actor_rotation(make_rotator(yaw=-105.0, roll=26.0), False)
        unreal.log(f"HeroAcousticGuitar UpVector={performer.get_actor_up_vector()}")
        if guitar_mesh:
            performer.guitar_mesh.set_static_mesh(guitar_mesh)

    # Hero Bench Seat facing +X toward the guitar & lake sunset
    bench_front_yaw = find_facing_yaw(unreal.Vector(0.0, -1.0, 0.0), unreal.Vector(1.0, 0.0, 0.0))
    bench = actor_sub.spawn_actor_from_class(
        unreal.InteractableSeatActor,
        unreal.Vector(92.0, -8.0, deck_top_z),
        make_rotator(),
    )
    if bench:
        bench.set_actor_label("SanctuaryBenchSeat")
        bench.set_editor_property("linked_performer", performer)
        if "painted_wooden_bench" in prop_mesh_sets:
            bmesh = pick_primary_mesh(prop_mesh_sets["painted_wooden_bench"], "painted_wooden_bench")
            if bmesh:
                bbb = bmesh.get_bounding_box()
                bench.seat_mesh.set_static_mesh(bmesh)
                bench.seat_mesh.set_relative_rotation(make_rotator(yaw=bench_front_yaw), False, True)
                bench.seat_mesh.set_relative_location(unreal.Vector(0.0, 0.0, -bbb.min.z), False, True)

    # Side Wooden Chair facing toward the guitar
    chair_target_dir = unreal.Vector(222.0 - 165.0, -4.0 - (-125.0), 0.0)
    chair_front_yaw = find_facing_yaw(unreal.Vector(0.0, 1.0, 0.0), chair_target_dir)
    chair = actor_sub.spawn_actor_from_class(
        unreal.InteractableSeatActor,
        unreal.Vector(165.0, -125.0, deck_top_z),
        make_rotator(),
    )
    if chair:
        chair.set_actor_label("LakesideWoodenChair")
        chair.set_editor_property("linked_performer", performer)
        if "WoodenChair_01" in prop_mesh_sets:
            cmesh = pick_primary_mesh(prop_mesh_sets["WoodenChair_01"], "WoodenChair_01")
            if cmesh:
                cbb = cmesh.get_bounding_box()
                chair_scale = 0.58
                chair.seat_mesh.set_static_mesh(cmesh)
                chair.seat_mesh.set_relative_scale3d(unreal.Vector(chair_scale, chair_scale, chair_scale))
                chair.seat_mesh.set_relative_rotation(make_rotator(yaw=chair_front_yaw), False, True)
                chair.seat_mesh.set_relative_location(unreal.Vector(0.0, 0.0, -cbb.min.z * chair_scale), False, True)

    # Helper to place grounded shoreline environment props cleanly by center/minZ
    def place_env(asset_key: str, loc: tuple[float, float, float], yaw: float = 0.0, scale: float = 1.0, sub_name: str | None = None, lbl: str | None = None) -> None:
        if asset_key not in prop_mesh_sets:
            return
        mset = prop_mesh_sets[asset_key]
        m = mset.get(sub_name) if sub_name else pick_primary_mesh(mset, asset_key)
        if m:
            spawn_centered_mesh(actor_sub, m, (loc[0], loc[1]), ground_z=loc[2], yaw=yaw, scale=scale, label=lbl or asset_key)

    # Cozy shoreline campfire stone ring + ember glow
    place_env("stone_fire_pit", (-95.0, 215.0, 0.0), yaw=20.0, scale=1.0, lbl="ShoreFirePit")
    spawn_warm_point_light(
        actor_sub,
        unreal.Vector(-95.0, 215.0, 28.0),
        (1.0, 0.50, 0.18, 1.0),
        intensity=5200.0,
        radius=680.0,
        label="ShoreFireEmberLight",
    )

    # Shoreline pines (~4.5m tall), driftwood, mossy stump, ferns, bushes, and boulders
    place_env("pine_sapling_small", (-460.0, -390.0, 0.0), yaw=15.0, scale=3.4, lbl="PineTree_SW")
    place_env("pine_sapling_small", (-580.0, 360.0, 0.0), yaw=110.0, scale=3.8, lbl="PineTree_NW")
    place_env("pine_sapling_small", (-840.0, -90.0, 0.0), yaw=240.0, scale=4.1, lbl="PineTree_West")
    place_env("pine_sapling_small", (-420.0, 520.0, 0.0), yaw=65.0, scale=3.1, lbl="PineTree_North")
    place_env("dead_tree_trunk", (-165.0, -295.0, 0.0), yaw=42.0, scale=1.1, lbl="ShoreDriftwood")
    place_env("tree_stump_01", (-230.0, 290.0, 0.0), yaw=75.0, scale=1.2, lbl="MossyStump")

    for idx, (fx, fy, fyaw, fsc) in enumerate([
        (-130.0, 155.0, 10.0, 1.2),
        (-195.0, -170.0, 95.0, 1.3),
        (-280.0, 220.0, 180.0, 1.1),
        (-310.0, -210.0, 270.0, 1.2),
    ], start=1):
        place_env("fern_02", (fx, fy, 0.0), yaw=fyaw, scale=fsc, lbl=f"ForestFern_{idx}")

    place_env("shrub_01", (-185.0, 310.0, 0.0), yaw=70.0, scale=1.1, lbl="LakesideShrub_1")
    place_env("shrub_02", (-240.0, -230.0, 0.0), yaw=185.0, scale=1.15, lbl="LakesideShrub_2")
    place_env("shrub_02", (-400.0, 180.0, 0.0), yaw=35.0, scale=1.25, lbl="LakesideShrub_3")

    if "rock_moss_set_01" in prop_mesh_sets:
        rset = prop_mesh_sets["rock_moss_set_01"]
        rock_names = sorted(rset.keys())
        rock_spots = [
            (25.0, 250.0, -10.0, 35.0, 1.15),
            (15.0, -245.0, -10.0, 145.0, 1.05),
            (-65.0, 360.0, -12.0, 220.0, 1.3),
            (-55.0, -350.0, -12.0, 80.0, 1.25),
            (210.0, 290.0, -28.0, 12.0, 1.4),
            (240.0, -280.0, -28.0, 195.0, 1.3),
        ]
        for idx, spot in enumerate(rock_spots):
            rname = rock_names[idx % len(rock_names)]
            place_env("rock_moss_set_01", (spot[0], spot[1], spot[2]), yaw=spot[3], scale=spot[4], sub_name=rname, lbl=f"ShoreRock_{idx + 1}")

    world_to_save = created_world or unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    saved_ok = unreal.EditorLoadingAndSavingUtils.save_map(world_to_save, map_path)
    unreal.log(f"RealGuitarGame AcousticSanctuary level build and save complete! saved={saved_ok}")


if __name__ == "__main__":
    bootstrap()
