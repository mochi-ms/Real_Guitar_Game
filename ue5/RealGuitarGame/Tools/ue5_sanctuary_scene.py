"""AAA Lakeside Sanctuary Scene Assembler for AcousticSanctuary.umap."""

from __future__ import annotations

import math
from pathlib import Path
import random
import unreal

import ue5_material_factory as mat_fac


def make_rotator(pitch: float = 0.0, yaw: float = 0.0, roll: float = 0.0) -> unreal.Rotator:
    r = unreal.Rotator()
    r.pitch = pitch
    r.yaw = yaw
    r.roll = roll
    return r


def rotate_xy(x: float, y: float, yaw_deg: float) -> tuple[float, float]:
    rad = math.radians(yaw_deg)
    c = math.cos(rad)
    s = math.sin(rad)
    return (x * c - y * s, x * s + y * c)


def yaw_for_dir(dx: float, dy: float) -> float:
    return math.degrees(math.atan2(dy, dx))


def set_complex_collision(mesh: unreal.StaticMesh) -> None:
    if not mesh:
        return
    body = mesh.get_editor_property("body_setup")
    if body:
        body.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
        unreal.EditorAssetLibrary.save_loaded_asset(mesh)


def list_meshes_in_folder(folder: str) -> dict[str, unreal.StaticMesh]:
    out: dict[str, unreal.StaticMesh] = {}
    if not unreal.EditorAssetLibrary.does_directory_exist(folder):
        return out
    for path in unreal.EditorAssetLibrary.list_assets(folder, recursive=True, include_folder=False):
        obj = mat_fac.load_asset_if_exists(path)
        if isinstance(obj, unreal.StaticMesh):
            out[obj.get_name()] = obj
    return out


def import_all_sanctuary_meshes(
    asset_tools: unreal.AssetTools,
    content_dir: Path,
) -> tuple[dict[str, unreal.StaticMesh], dict[str, dict[str, unreal.StaticMesh]]]:
    """Ensure all Custom GLBs and Poly Haven 2K models are imported into Unreal packages."""
    custom_meshes: dict[str, unreal.StaticMesh] = {}
    custom_dir = content_dir / "Models" / "Custom"
    if custom_dir.exists():
        for glb in sorted(custom_dir.glob("*.glb")):
            dest_folder = f"/Game/Models/Custom/{glb.stem}"
            meshes = list_meshes_in_folder(dest_folder)
            if not meshes:
                mat_fac.ensure_dir(dest_folder)
                mat_fac.import_asset(asset_tools, glb, dest_folder, replace_existing=False)
                meshes = list_meshes_in_folder(dest_folder)
            for name, sm in meshes.items():
                set_complex_collision(sm)
                custom_meshes[name] = sm

    poly_meshes: dict[str, dict[str, unreal.StaticMesh]] = {}
    poly_root = content_dir / "ExternalAssets" / "PolyHaven"
    if poly_root.exists():
        for folder in sorted(p for p in poly_root.iterdir() if p.is_dir()):
            dest_folder = f"/Game/Models/Props/{folder.name}"
            meshes = list_meshes_in_folder(dest_folder)
            if not meshes:
                gltfs = list(folder.glob("*.gltf")) + list(folder.glob("*.glb"))
                if gltfs:
                    mat_fac.ensure_dir(dest_folder)
                    mat_fac.import_asset(asset_tools, gltfs[0], dest_folder, replace_existing=False)
                    meshes = list_meshes_in_folder(dest_folder)
            for sm in meshes.values():
                if "modular_wooden_pier" in folder.name.lower():
                    set_complex_collision(sm)
            if meshes:
                poly_meshes[folder.name] = meshes

    return custom_meshes, poly_meshes


def pick_mesh(mesh_map: dict[str, unreal.StaticMesh], preferred: str | None = None) -> unreal.StaticMesh | None:
    if not mesh_map:
        return None
    if preferred and preferred in mesh_map:
        return mesh_map[preferred]
    if preferred:
        for k, v in mesh_map.items():
            if preferred.lower() in k.lower():
                return v
    return next(iter(mesh_map.values()))


def trace_shore_z(world, x: float, y: float, fallback_z: float = 25.0) -> float:
    hit = unreal.SystemLibrary.line_trace_single(
        world,
        unreal.Vector(x, y, 8000.0),
        unreal.Vector(x, y, -3500.0),
        unreal.TraceTypeQuery.ECC_VISIBILITY,
        True,
        [],
        unreal.DrawDebugTrace.NONE,
        True,
    )
    if hit:
        return float(hit.to_tuple()[4].z)
    return fallback_z


def spawn_mesh_direct(
    actor_sub: unreal.EditorActorSubsystem,
    mesh: unreal.StaticMesh,
    location: unreal.Vector,
    yaw_deg: float = 0.0,
    scale: unreal.Vector = unreal.Vector(1.0, 1.0, 1.0),
    label: str | None = None,
    mat_overrides: dict[int, unreal.MaterialInterface] | None = None,
    disable_collision: bool = False,
) -> unreal.StaticMeshActor | None:
    if not mesh:
        return None
    act = actor_sub.spawn_actor_from_class(unreal.StaticMeshActor, location, make_rotator(yaw=yaw_deg))
    if not act:
        return None
    comp = act.static_mesh_component
    comp.set_static_mesh(mesh)
    act.set_actor_scale3d(scale)
    if mat_overrides:
        for slot_idx, mat in mat_overrides.items():
            if mat:
                comp.set_material(slot_idx, mat)
    if disable_collision:
        comp.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    if label:
        act.set_actor_label(label)
    return act


def spawn_grounded_prop(
    actor_sub: unreal.EditorActorSubsystem,
    world,
    mesh: unreal.StaticMesh,
    x: float,
    y: float,
    yaw_deg: float = 0.0,
    scale_xyz: float = 1.0,
    sink_cm: float = 3.0,
    fixed_z: float | None = None,
    label: str | None = None,
    disable_collision: bool = False,
) -> unreal.StaticMeshActor | None:
    """Spawn a Poly Haven mesh with baked Blender pivot offset compensated cleanly onto ground Z."""
    if not mesh:
        return None
    bb = mesh.get_bounding_box()
    cx_local = 0.5 * (bb.min.x + bb.max.x) * scale_xyz
    cy_local = 0.5 * (bb.min.y + bb.max.y) * scale_xyz
    rx, ry = rotate_xy(cx_local, cy_local, yaw_deg)
    gz = fixed_z if fixed_z is not None else trace_shore_z(world, x, y, 25.0)
    actor_z = gz - sink_cm - (bb.min.z * scale_xyz)
    return spawn_mesh_direct(
        actor_sub,
        mesh,
        unreal.Vector(x - rx, y - ry, actor_z),
        yaw_deg=yaw_deg,
        scale=unreal.Vector(scale_xyz, scale_xyz, scale_xyz),
        label=label,
        disable_collision=disable_collision,
    )


def spawn_point_light(
    actor_sub: unreal.EditorActorSubsystem,
    loc: unreal.Vector,
    intensity: float,
    color: tuple[float, float, float],
    radius: float,
    label: str,
) -> None:
    light = actor_sub.spawn_actor_from_class(unreal.PointLight, loc, make_rotator())
    if light and light.point_light_component:
        plc = light.point_light_component
        plc.set_intensity(intensity)
        plc.set_light_color(unreal.LinearColor(*color, 1.0))
        plc.set_attenuation_radius(radius)
        plc.set_source_radius(14.0)
        plc.set_cast_shadows(False)
        light.set_actor_label(label)


def spawn_seat_actor(
    actor_sub: unreal.EditorActorSubsystem,
    world,
    x: float,
    y: float,
    face_dx: float,
    face_dy: float,
    seat_mesh: unreal.StaticMesh | None,
    seat_scale: float,
    seat_mesh_yaw_offset: float,
    sit_height_offset: float,
    prompt_text: str,
    label: str,
    fixed_z: float | None = None,
) -> None:
    yaw = yaw_for_dir(face_dx, face_dy)
    gz = fixed_z if fixed_z is not None else trace_shore_z(world, x, y, 30.0)
    seat = actor_sub.spawn_actor_from_class(
        unreal.InteractableSeatActor,
        unreal.Vector(x, y, gz),
        make_rotator(yaw=yaw),
    )
    if not seat:
        return
    seat.set_actor_label(label)
    seat.set_editor_property("sit_prompt_text", unreal.Text(prompt_text))

    if seat_mesh and seat.seat_mesh:
        bb = seat_mesh.get_bounding_box()
        cx = 0.5 * (bb.min.x + bb.max.x) * seat_scale
        cy = 0.5 * (bb.min.y + bb.max.y) * seat_scale
        rx, ry = rotate_xy(cx, cy, seat_mesh_yaw_offset)
        seat.seat_mesh.set_static_mesh(seat_mesh)
        seat.seat_mesh.set_relative_scale3d(unreal.Vector(seat_scale, seat_scale, seat_scale))
        seat.seat_mesh.set_relative_rotation(make_rotator(yaw=seat_mesh_yaw_offset), False, True)
        seat.seat_mesh.set_relative_location(
            unreal.Vector(-rx, -ry, -bb.min.z * seat_scale),
            False,
            True,
        )
    if seat.sit_anchor:
        seat.sit_anchor.set_relative_location(unreal.Vector(6.0, 0.0, sit_height_offset), False, True)
        seat.sit_anchor.set_relative_rotation(make_rotator(yaw=0.0), False, True)


def build_sanctuary_level(
    asset_tools: unreal.AssetTools,
    content_dir: Path,
    custom_meshes: dict[str, unreal.StaticMesh],
    poly_meshes: dict[str, dict[str, unreal.StaticMesh]],
    materials: dict[str, unreal.MaterialInterface],
    ambience_sound: unreal.SoundBase | None,
) -> bool:
    """Construct and save photorealistic /Game/Maps/AcousticSanctuary."""
    map_path = "/Game/Maps/AcousticSanctuary"
    tmp_reset = "/Game/Maps/_TmpReset"
    level_sub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    ed_sub = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)

    if unreal.EditorAssetLibrary.does_asset_exist(map_path):
        level_sub.load_level(map_path)
        keep_classes = {
            "WorldSettings",
            "Brush",
            "DefaultPhysicsVolume",
            "GameplayDebuggerCategoryReplicator",
        }
        for actor in list(actor_sub.get_all_level_actors()):
            if actor and actor.get_class().get_name() not in keep_classes:
                actor_sub.destroy_actor(actor)
    else:
        level_sub.new_level(map_path)
    if unreal.EditorAssetLibrary.does_asset_exist(tmp_reset):
        unreal.EditorAssetLibrary.delete_asset(tmp_reset)
    world = ed_sub.get_editor_world()

    # 1. Sky Atmosphere, Volumetric Cloud & ACES Post Process
    actor_sub.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0), make_rotator())
    actor_sub.spawn_actor_from_class(unreal.VolumetricCloud, unreal.Vector(0, 0, 0), make_rotator())
    ppv = actor_sub.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 0), make_rotator())
    if ppv:
        ppv.set_editor_property("unbound", True)
        pp_settings = ppv.get_editor_property("settings")
        pp_settings.set_editor_property("override_bloom_intensity", True)
        pp_settings.set_editor_property("bloom_intensity", 0.55)
        pp_settings.set_editor_property("override_vignette_intensity", True)
        pp_settings.set_editor_property("vignette_intensity", 0.34)
        pp_settings.set_editor_property("override_auto_exposure_bias", True)
        pp_settings.set_editor_property("auto_exposure_bias", 0.15)
        ppv.set_editor_property("settings", pp_settings)

    # 2. Sculpted 270m Shoreline + Underwater Lakebed + 360° Mountain Horizon
    shore_mesh = custom_meshes.get("ShoreAndMountainBasin")
    spawn_mesh_direct(
        actor_sub,
        shore_mesh,
        unreal.Vector(0.0, 0.0, 0.0),
        label="ShoreAndMountainBasin",
        mat_overrides={
            0: materials.get("shore"),
            1: materials.get("rock"),
        },
    )

    # 3. Calm Reflective Lake Water Surface (at Z = 0, no collision so lakebed dips smoothly)
    plane_mesh = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Plane.Plane")
    sphere_mesh = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Sphere.Sphere")
    spawn_mesh_direct(
        actor_sub,
        plane_mesh,
        unreal.Vector(2200.0, 0.0, 0.0),
        scale=unreal.Vector(340.0, 340.0, 1.0),
        label="CalmLakeSurface",
        mat_overrides={0: materials.get("lake")},
        disable_collision=True,
    )

    # 4. Hero Lakeside Gazebo Pavilion Stage (-320, 0, Z=12 -> deck top Z=40)
    gazebo_x, gazebo_y, gazebo_z = -320.0, 0.0, 12.0
    deck_top_z = 40.0
    spawn_mesh_direct(
        actor_sub,
        custom_meshes.get("LakesideGazebo"),
        unreal.Vector(gazebo_x, gazebo_y, gazebo_z),
        yaw_deg=0.0,
        label="LakesideGazebo",
        mat_overrides={0: materials.get("deck")},
    )
    spawn_mesh_direct(
        actor_sub,
        custom_meshes.get("AcousticStageRug"),
        unreal.Vector(gazebo_x + 5.0, gazebo_y, deck_top_z + 0.15),
        yaw_deg=12.0,
        label="AcousticStageRug",
    )
    spawn_mesh_direct(
        actor_sub,
        custom_meshes.get("MusicStand"),
        unreal.Vector(-180.0, -95.0, deck_top_z),
        yaw_deg=155.0,
        label="MusicStand",
    )

    # 5. Seamless Wooden Lake Pier extending +X over calm cove water
    pier_meshes = poly_meshes.get("modular_wooden_pier", {})
    pier_loc = unreal.Vector(-15.0, 0.0, deck_top_z - 267.4)
    for sec_name in (
        "modular_wooden_pier_section_01",
        "modular_wooden_pier_section_02",
        "modular_wooden_pier_section_03",
        "modular_wooden_pier_planks",
        "modular_wooden_pier_poles",
    ):
        sm = pick_mesh(pier_meshes, sec_name)
        if sm:
            spawn_mesh_direct(
                actor_sub,
                sm,
                pier_loc,
                yaw_deg=-90.0,
                label=f"LakePier_{sec_name}",
            )

    # Pier tip lantern + weathered dock barrels
    if "wooden_lantern_01" in poly_meshes:
        lset = poly_meshes["wooden_lantern_01"]
        for part in ("wooden_lantern_01", "wooden_lantern_01_door", "wooden_lantern_01_handle"):
            sm = lset.get(part)
            if sm:
                spawn_mesh_direct(actor_sub, sm, unreal.Vector(810.0, 88.0, deck_top_z + 1.5), yaw_deg=-130.0, label=f"PierLantern_{part}")
        spawn_point_light(actor_sub, unreal.Vector(810.0, 88.0, deck_top_z + 24.0), 1700.0, (1.0, 0.68, 0.34), 760.0, "PierLanternLight")

    if "wooden_barrels_01" in poly_meshes:
        bset = poly_meshes["wooden_barrels_01"]
        spawn_grounded_prop(actor_sub, world, pick_mesh(bset, "barrel01"), 18.0, 158.0, yaw_deg=24.0, sink_cm=2.0, label="PierBarrel_01")
        spawn_grounded_prop(actor_sub, world, pick_mesh(bset, "barrel02"), 62.0, 205.0, yaw_deg=-48.0, sink_cm=2.0, label="PierBarrel_02")

    # 6. Cozy Campfire Clearing & Rustic Gathering (-560, -520)
    fire_x, fire_y = -560.0, -520.0
    fire_z = trace_shore_z(world, fire_x, fire_y, 26.0)
    if "stone_fire_pit" in poly_meshes:
        spawn_grounded_prop(
            actor_sub,
            world,
            pick_mesh(poly_meshes["stone_fire_pit"]),
            fire_x,
            fire_y,
            yaw_deg=15.0,
            sink_cm=3.0,
            fixed_z=fire_z,
            label="CampfireStonePit",
        )
    if sphere_mesh:
        spawn_mesh_direct(
            actor_sub,
            sphere_mesh,
            unreal.Vector(fire_x, fire_y, fire_z + 10.0),
            scale=unreal.Vector(0.46, 0.46, 0.14),
            label="CampfireGlowingEmbers",
            mat_overrides={0: materials.get("ember")},
            disable_collision=True,
        )

    if "dead_tree_trunk" in poly_meshes:
        spawn_grounded_prop(actor_sub, world, pick_mesh(poly_meshes["dead_tree_trunk"]), -430.0, -645.0, yaw_deg=38.0, sink_cm=3.0, label="CampfireLogSeat")
    if "tree_stump_01" in poly_meshes:
        spawn_grounded_prop(actor_sub, world, pick_mesh(poly_meshes["tree_stump_01"]), -670.0, -575.0, yaw_deg=70.0, sink_cm=4.0, label="CampfireStump")

    if "round_wooden_table_01" in poly_meshes:
        t_x, t_y = -595.0, -355.0
        t_z = trace_shore_z(world, t_x, t_y, 26.0)
        tset = poly_meshes["round_wooden_table_01"]
        for tpart in tset.values():
            spawn_grounded_prop(actor_sub, world, tpart, t_x, t_y, yaw_deg=20.0, scale_xyz=0.82, sink_cm=2.0, fixed_z=t_z, label=f"CampTable_{tpart.get_name()}")
        if "Lantern_01" in poly_meshes:
            for lpart in poly_meshes["Lantern_01"].values():
                spawn_grounded_prop(actor_sub, world, lpart, t_x, t_y, yaw_deg=-25.0, scale_xyz=1.0, sink_cm=0.0, fixed_z=t_z + 82.0, label=f"TableLantern_{lpart.get_name()}")
            spawn_point_light(actor_sub, unreal.Vector(t_x, t_y, t_z + 102.0), 1200.0, (1.0, 0.70, 0.36), 540.0, "TableLanternLight")

    # Footpath Street Lamps
    if "street_lamp_01" in poly_meshes:
        sl_mesh = pick_mesh(poly_meshes["street_lamp_01"])
        for lx, ly, lyaw, lidx in [(-650.0, 295.0, -25.0, 1), (-165.0, 390.0, -145.0, 2)]:
            lz = trace_shore_z(world, lx, ly, 26.0)
            spawn_grounded_prop(actor_sub, world, sl_mesh, lx, ly, yaw_deg=lyaw, scale_xyz=0.92, sink_cm=2.0, fixed_z=lz, label=f"PathLamp_{lidx}")
            spawn_point_light(actor_sub, unreal.Vector(lx, ly, lz + 325.0), 1800.0, (1.0, 0.72, 0.38), 780.0, f"PathLampLight_{lidx}")

    # 7. Three Scenic Playable Seats (Gazebo Bench, Pier Stool, Campfire Chair)
    if "painted_wooden_bench" in poly_meshes:
        spawn_seat_actor(
            actor_sub,
            world,
            gazebo_x - 30.0,
            gazebo_y,
            1.0,
            0.0,
            pick_mesh(poly_meshes["painted_wooden_bench"], "painted_wooden_bench"),
            seat_scale=1.0,
            seat_mesh_yaw_offset=-90.0,
            sit_height_offset=46.0,
            prompt_text="[E] 호숫가 정자 벤치에 앉아 연주",
            label="GazeboBenchSeat",
            fixed_z=deck_top_z,
        )

    if "folding_wooden_stool" in poly_meshes:
        spawn_seat_actor(
            actor_sub,
            world,
            745.0,
            -38.0,
            1.0,
            0.0,
            pick_mesh(poly_meshes["folding_wooden_stool"]),
            seat_scale=1.0,
            seat_mesh_yaw_offset=-90.0,
            sit_height_offset=44.0,
            prompt_text="[E] 호수 부두 스툴에 앉아 물소리와 연주",
            label="PierStoolSeat",
            fixed_z=deck_top_z,
        )

    if "WoodenChair_01" in poly_meshes:
        cx, cy = -695.0, -430.0
        spawn_seat_actor(
            actor_sub,
            world,
            cx,
            cy,
            fire_x - cx,
            fire_y - cy,
            pick_mesh(poly_meshes["WoodenChair_01"], "WoodenChair_01"),
            seat_scale=0.58,
            seat_mesh_yaw_offset=-90.0,
            sit_height_offset=44.0,
            prompt_text="[E] 모닥불 의자에 앉아 따뜻하게 연주",
            label="CampfireChairSeat",
        )

    # 8. Coastal Cliffs framing the cove bay & Distant 360° Mountain Silhouettes
    if "coastal_cliff_01" in poly_meshes:
        c_mesh = pick_mesh(poly_meshes["coastal_cliff_01"])
        for cx, cy, cyaw, cscale, csink, cid in [
            (2250.0, -2750.0, 22.0, 0.75, 160.0, "South"),
            (2400.0, 2850.0, 158.0, 0.78, 170.0, "North"),
            (4850.0, -2300.0, 52.0, 0.90, 200.0, "FarSouth"),
        ]:
            spawn_grounded_prop(actor_sub, world, c_mesh, cx, cy, yaw_deg=cyaw, scale_xyz=cscale, sink_cm=csink, label=f"CoveCliff_{cid}")

    if "mountainside" in poly_meshes:
        m_mesh = pick_mesh(poly_meshes["mountainside"])
        for idx, (ang_deg, dist, mscale) in enumerate([
            (-36.0, 8400.0, 7.0),
            (38.0, 8600.0, 7.2),
            (-78.0, 7800.0, 6.5),
            (84.0, 7900.0, 6.8),
            (138.0, 8200.0, 7.4),
            (-136.0, 8100.0, 7.1),
            (178.0, 9200.0, 8.0),
        ]):
            rad = math.radians(ang_deg)
            mx = dist * math.cos(rad)
            my = dist * math.sin(rad)
            spawn_grounded_prop(actor_sub, world, m_mesh, mx, my, yaw_deg=ang_deg + 90.0, scale_xyz=mscale, sink_cm=400.0, label=f"MountainPeak_{idx}")

    # 9. Deterministic Natural Shoreline Boulders, Conifer Forest & Botanical Understory
    rng = random.Random(2026)

    def in_clearing_or_water(px: float, py: float) -> bool:
        if math.hypot(px - gazebo_x, py - gazebo_y) < 620.0:
            return True
        if math.hypot(px - fire_x, py - fire_y) < 380.0:
            return True
        # Keep Pier corridor clear
        if px > -260.0 and px < 1150.0 and abs(py) < 420.0:
            return True
        # Keep cove water center clear for trees/shrubs
        if px > 180.0 and math.hypot(px - 2200.0, py * 0.78) < 1750.0:
            return True
        return False

    tree_choices: list[unreal.StaticMesh] = []
    for key in ("pine_tree_01", "fir_tree_01"):
        if key in poly_meshes:
            tree_choices.extend(poly_meshes[key].values())
    sapling_choices: list[unreal.StaticMesh] = []
    for key in ("fir_sapling_medium", "pine_sapling_small"):
        if key in poly_meshes:
            sapling_choices.extend(poly_meshes[key].values())

    # Layer A: Tall Forest Canopy Rings
    tree_count = 0
    attempts = 0
    while tree_count < 76 and attempts < 600:
        attempts += 1
        ang = rng.uniform(-math.pi, math.pi)
        # Keep bay vista window along +X (-22..+22 deg) open to show sunset lake & mountains
        if abs(ang) < math.radians(24.0):
            continue
        r = rng.uniform(760.0, 5600.0)
        tx = r * math.cos(ang)
        ty = r * math.sin(ang)
        if in_clearing_or_water(tx, ty):
            continue
        tz = trace_shore_z(world, tx, ty, -999.0)
        if tz < 8.0:
            continue
        use_sapling = (r < 1250.0 and rng.random() < 0.55) or rng.random() < 0.25
        pool = sapling_choices if (use_sapling and sapling_choices) else tree_choices
        if not pool:
            continue
        sm = rng.choice(pool)
        sc = rng.uniform(0.82, 1.32) if not use_sapling else rng.uniform(0.85, 1.25)
        spawn_grounded_prop(actor_sub, world, sm, tx, ty, yaw_deg=rng.uniform(0, 360), scale_xyz=sc, sink_cm=14.0, fixed_z=tz, label=f"ForestTree_{tree_count:03d}")
        tree_count += 1

    # Layer B: Mossy Boulders & Shoreline Rocks
    boulder_pool: list[unreal.StaticMesh] = []
    for key in ("rock_moss_set_01", "namaqualand_boulders_01"):
        if key in poly_meshes:
            boulder_pool.extend(poly_meshes[key].values())

    if boulder_pool:
        boulder_count = 0
        for _ in range(180):
            if boulder_count >= 38:
                break
            ang = rng.uniform(-math.pi, math.pi)
            r = rng.uniform(460.0, 2900.0)
            bx = r * math.cos(ang)
            by = r * math.sin(ang)
            if math.hypot(bx - gazebo_x, by - gazebo_y) < 420.0:
                continue
            if px_in_pier := (bx > -220.0 and bx < 1050.0 and abs(by) < 250.0):
                continue
            bz = trace_shore_z(world, bx, by, -999.0)
            if bz < -60.0 or bz > 650.0:
                continue
            sm = rng.choice(boulder_pool)
            sc = rng.uniform(0.65, 1.85)
            spawn_grounded_prop(actor_sub, world, sm, bx, by, yaw_deg=rng.uniform(0, 360), scale_xyz=sc, sink_cm=18.0, fixed_z=bz, label=f"ShoreBoulder_{boulder_count:02d}")
            boulder_count += 1

    # Layer C: Dense Botanical Understory (ferns, shrubs, grass tufts, wildflowers, moss pads)
    foliage_pool: list[tuple[unreal.StaticMesh, float, float]] = []
    if "fern_02" in poly_meshes:
        for m in poly_meshes["fern_02"].values():
            foliage_pool.append((m, 1.1, 1.7))
    if "shrub_01" in poly_meshes:
        for m in poly_meshes["shrub_01"].values():
            foliage_pool.append((m, 1.0, 1.5))
    if "shrub_02" in poly_meshes:
        for m in poly_meshes["shrub_02"].values():
            foliage_pool.append((m, 0.75, 1.25))
    if "flower_ursinia" in poly_meshes:
        for m in list(poly_meshes["flower_ursinia"].values())[:4]:
            foliage_pool.append((m, 1.3, 2.0))
    if "grass_medium_01" in poly_meshes:
        for m in list(poly_meshes["grass_medium_01"].values())[:6]:
            foliage_pool.append((m, 1.4, 2.4))
    if "moss_01" in poly_meshes:
        for m in list(poly_meshes["moss_01"].values())[:4]:
            foliage_pool.append((m, 1.2, 2.0))

    if foliage_pool:
        fol_count = 0
        for _ in range(360):
            if fol_count >= 150:
                break
            ang = rng.uniform(-math.pi, math.pi)
            r = rng.uniform(310.0, 2100.0)
            fx = gazebo_x + r * math.cos(ang)
            fy = gazebo_y + r * math.sin(ang)
            if math.hypot(fx - gazebo_x, fy - gazebo_y) < 335.0:
                continue
            if math.hypot(fx - fire_x, fy - fire_y) < 135.0:
                continue
            if fx > -120.0 and fx < 980.0 and abs(fy) < 190.0:
                continue
            fz = trace_shore_z(world, fx, fy, -999.0)
            if fz < 2.0:
                continue
            sm, sc_min, sc_max = rng.choice(foliage_pool)
            sc = rng.uniform(sc_min, sc_max)
            spawn_grounded_prop(
                actor_sub,
                world,
                sm,
                fx,
                fy,
                yaw_deg=rng.uniform(0, 360),
                scale_xyz=sc,
                sink_cm=3.0,
                fixed_z=fz,
                label=f"Understory_{fol_count:03d}",
                disable_collision=True,
            )
            fol_count += 1

    # 10. Sanctuary Atmosphere Actor (Sun, SkyLight, Fog, Gazebo Lamp, Flickering Campfire Light & Ambience)
    atmo = actor_sub.spawn_actor_from_class(unreal.SanctuaryAtmosphereActor, unreal.Vector(0, 0, 0), make_rotator())
    if atmo:
        atmo.set_actor_label("SanctuaryAtmosphere")
        if atmo.warm_bench_lamp:
            atmo.warm_bench_lamp.set_relative_location(unreal.Vector(gazebo_x + 20.0, gazebo_y, deck_top_z + 245.0), False, True)
        if atmo.campfire_light:
            atmo.campfire_light.set_relative_location(unreal.Vector(fire_x, fire_y, fire_z + 55.0), False, True)
        if ambience_sound:
            if isinstance(ambience_sound, unreal.SoundWave):
                ambience_sound.set_editor_property("looping", True)
                unreal.EditorAssetLibrary.save_loaded_asset(ambience_sound)
            atmo.set_editor_property("ambient_loop_sound", ambience_sound)

    # 11. PlayerStart standing cleanly on front Gazebo stage looking diagonally across Rug -> Pier -> Sunset Lake
    ps = actor_sub.spawn_actor_from_class(
        unreal.PlayerStart,
        unreal.Vector(-210.0, 95.0, deck_top_z + 92.0),
        make_rotator(yaw=-14.0),
    )
    if ps:
        ps.set_actor_label("SanctuaryPlayerStart")

    return bool(unreal.EditorLoadingAndSavingUtils.save_map(world, map_path))
