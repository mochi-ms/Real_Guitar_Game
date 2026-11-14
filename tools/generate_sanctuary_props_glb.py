"""Procedural glTF 2.0 (.glb) 3D modeling builder for AAA sanctuary architecture & terrain."""

from __future__ import annotations

import json
import math
from pathlib import Path
import struct

REPO_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_CUSTOM_MODELS_DIR = (
    REPO_ROOT / "ue5" / "RealGuitarGame" / "Content" / "Models" / "Custom"
)

Vec2 = tuple[float, float]
Vec3 = tuple[float, float, float]


def _sub(a: Vec3, b: Vec3) -> Vec3:
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def _cross(a: Vec3, b: Vec3) -> Vec3:
    return (
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0],
    )


def _norm(v: Vec3) -> Vec3:
    m = math.sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2])
    return (0.0, 1.0, 0.0) if m <= 1e-8 else (v[0] / m, v[1] / m, v[2] / m)


class UVBufferedPart:
    """Single-material mesh primitive buffer with positions, normals, UV0, and uint32 indices."""

    def __init__(self, name: str, material_idx: int) -> None:
        self.name = name
        self.material_idx = material_idx
        self.positions: list[Vec3] = []
        self.normals: list[Vec3] = []
        self.uvs: list[Vec2] = []
        self.indices: list[int] = []

    def add_tri(
        self,
        a: Vec3,
        b: Vec3,
        c: Vec3,
        uv_a: Vec2 = (0.0, 0.0),
        uv_b: Vec2 = (1.0, 0.0),
        uv_c: Vec2 = (1.0, 1.0),
        n_a: Vec3 | None = None,
        n_b: Vec3 | None = None,
        n_c: Vec3 | None = None,
    ) -> None:
        face_n = _norm(_cross(_sub(b, a), _sub(c, a)))
        base = len(self.positions)
        self.positions.extend((a, b, c))
        self.normals.extend((n_a or face_n, n_b or face_n, n_c or face_n))
        self.uvs.extend((uv_a, uv_b, uv_c))
        self.indices.extend((base, base + 1, base + 2))

    def add_quad(
        self,
        p00: Vec3,
        p10: Vec3,
        p11: Vec3,
        p01: Vec3,
        uv00: Vec2 = (0.0, 0.0),
        uv10: Vec2 = (1.0, 0.0),
        uv11: Vec2 = (1.0, 1.0),
        uv01: Vec2 = (0.0, 1.0),
    ) -> None:
        n = _norm(_cross(_sub(p10, p00), _sub(p11, p00)))
        base = len(self.positions)
        self.positions.extend((p00, p10, p11, p01))
        self.normals.extend((n, n, n, n))
        self.uvs.extend((uv00, uv10, uv11, uv01))
        self.indices.extend((base, base + 1, base + 2, base, base + 2, base + 3))

    def add_box(
        self,
        cx: float,
        cy: float,
        cz: float,
        hx: float,
        hy: float,
        hz: float,
        uv_scale: float = 0.5,
    ) -> None:
        """Axis-aligned box with outward-facing CCW quads and world-metric UVs."""
        x0, x1 = cx - hx, cx + hx
        y0, y1 = cy - hy, cy + hy
        z0, z1 = cz - hz, cz + hz
        ux, uy, uz = hx * 2.0 * uv_scale, hy * 2.0 * uv_scale, hz * 2.0 * uv_scale

        # +Y top, -Y bottom
        self.add_quad((x0, y1, z1), (x1, y1, z1), (x1, y1, z0), (x0, y1, z0), (0, 0), (ux, 0), (ux, uz), (0, uz))
        self.add_quad((x0, y0, z0), (x1, y0, z0), (x1, y0, z1), (x0, y0, z1), (0, 0), (ux, 0), (ux, uz), (0, uz))
        # +Z front, -Z back
        self.add_quad((x0, y0, z1), (x1, y0, z1), (x1, y1, z1), (x0, y1, z1), (0, 0), (ux, 0), (ux, uy), (0, uy))
        self.add_quad((x1, y0, z0), (x0, y0, z0), (x0, y1, z0), (x1, y1, z0), (0, 0), (ux, 0), (ux, uy), (0, uy))
        # +X right, -X left
        self.add_quad((x1, y0, z1), (x1, y0, z0), (x1, y1, z0), (x1, y1, z1), (0, 0), (uz, 0), (uz, uy), (0, uy))
        self.add_quad((x0, y0, z0), (x0, y0, z1), (x0, y1, z1), (x0, y1, z0), (0, 0), (uz, 0), (uz, uy), (0, uy))

    def add_cylinder_y(
        self,
        cx: float,
        y0: float,
        cz: float,
        y1: float,
        r0: float,
        r1: float,
        segments: int = 16,
        uv_tile: float = 1.0,
    ) -> None:
        ring0: list[Vec3] = []
        ring1: list[Vec3] = []
        for i in range(segments + 1):
            ang = 2.0 * math.pi * (i % segments) / segments
            ca, sa = math.cos(ang), math.sin(ang)
            ring0.append((cx + ca * r0, y0, cz + sa * r0))
            ring1.append((cx + ca * r1, y1, cz + sa * r1))
        for i in range(segments):
            u0 = (i / segments) * uv_tile
            u1 = ((i + 1) / segments) * uv_tile
            self.add_quad(
                ring0[i],
                ring0[i + 1],
                ring1[i + 1],
                ring1[i],
                (u0, 0.0),
                (u1, 0.0),
                (u1, uv_tile),
                (u0, uv_tile),
            )
        # End caps
        c_top = (cx, y1, cz)
        c_bot = (cx, y0, cz)
        for i in range(segments):
            self.add_tri(c_top, ring1[i + 1], ring1[i], (0.5, 0.5), (1.0, 0.5), (0.5, 1.0))
            self.add_tri(c_bot, ring0[i], ring0[i + 1], (0.5, 0.5), (1.0, 0.5), (0.5, 1.0))

    def add_uv_sphere(
        self,
        cx: float,
        cy: float,
        cz: float,
        radius: float,
        stacks: int = 8,
        slices: int = 12,
    ) -> None:
        rings: list[list[Vec3]] = []
        for s in range(stacks + 1):
            phi = math.pi * s / stacks
            y = cy + math.cos(phi) * radius
            r_s = math.sin(phi) * radius
            ring = []
            for j in range(slices + 1):
                theta = 2.0 * math.pi * (j % slices) / slices
                ring.append((cx + math.cos(theta) * r_s, y, cz + math.sin(theta) * r_s))
            rings.append(ring)
        for s in range(stacks):
            for j in range(slices):
                u0, u1 = j / slices, (j + 1) / slices
                v0, v1 = s / stacks, (s + 1) / stacks
                self.add_quad(
                    rings[s][j],
                    rings[s + 1][j],
                    rings[s + 1][j + 1],
                    rings[s][j + 1],
                    (u0, v0),
                    (u0, v1),
                    (u1, v1),
                    (u1, v0),
                )


def _export_glb_parts(
    output_path: Path,
    mesh_name: str,
    materials: list[dict[str, object]],
    parts: list[UVBufferedPart],
) -> Path:
    output_path.parent.mkdir(parents=True, exist_ok=True)
    bin_buf = bytearray()
    buffer_views: list[dict[str, object]] = []
    accessors: list[dict[str, object]] = []
    primitives: list[dict[str, object]] = []

    def append_bytes(payload: bytes, target: int) -> int:
        while len(bin_buf) % 4 != 0:
            bin_buf.append(0)
        offset = len(bin_buf)
        bin_buf.extend(payload)
        bv_idx = len(buffer_views)
        buffer_views.append(
            {"buffer": 0, "byteOffset": offset, "byteLength": len(payload), "target": target}
        )
        return bv_idx

    for part in parts:
        if not part.indices:
            continue
        pos_bytes = b"".join(struct.pack("<fff", *p) for p in part.positions)
        norm_bytes = b"".join(struct.pack("<fff", *n) for n in part.normals)
        uv_bytes = b"".join(struct.pack("<ff", *uv) for uv in part.uvs)
        idx_bytes = b"".join(struct.pack("<I", i) for i in part.indices)

        bv_pos = append_bytes(pos_bytes, 34962)
        bv_norm = append_bytes(norm_bytes, 34962)
        bv_uv = append_bytes(uv_bytes, 34962)
        bv_idx = append_bytes(idx_bytes, 34963)

        xs = [p[0] for p in part.positions]
        ys = [p[1] for p in part.positions]
        zs = [p[2] for p in part.positions]

        acc_pos = len(accessors)
        accessors.append(
            {
                "bufferView": bv_pos,
                "componentType": 5126,
                "count": len(part.positions),
                "type": "VEC3",
                "min": [round(min(xs), 6), round(min(ys), 6), round(min(zs), 6)],
                "max": [round(max(xs), 6), round(max(ys), 6), round(max(zs), 6)],
            }
        )
        acc_norm = len(accessors)
        accessors.append(
            {"bufferView": bv_norm, "componentType": 5126, "count": len(part.normals), "type": "VEC3"}
        )
        acc_uv = len(accessors)
        accessors.append(
            {"bufferView": bv_uv, "componentType": 5126, "count": len(part.uvs), "type": "VEC2"}
        )
        acc_idx = len(accessors)
        accessors.append(
            {"bufferView": bv_idx, "componentType": 5125, "count": len(part.indices), "type": "SCALAR"}
        )

        primitives.append(
            {
                "attributes": {"POSITION": acc_pos, "NORMAL": acc_norm, "TEXCOORD_0": acc_uv},
                "indices": acc_idx,
                "material": part.material_idx,
            }
        )

    while len(bin_buf) % 4 != 0:
        bin_buf.append(0)

    gltf = {
        "asset": {"version": "2.0", "generator": "RealGuitarGame_Sanctuary3DBuilder"},
        "scene": 0,
        "scenes": [{"name": mesh_name, "nodes": [0]}],
        "nodes": [{"name": mesh_name, "mesh": 0}],
        "meshes": [{"name": mesh_name, "primitives": primitives}],
        "materials": materials,
        "buffers": [{"byteLength": len(bin_buf)}],
        "bufferViews": buffer_views,
        "accessors": accessors,
    }
    json_bytes = json.dumps(gltf, separators=(",", ":")).encode("utf-8")
    while len(json_bytes) % 4 != 0:
        json_bytes += b" "

    total_len = 12 + 8 + len(json_bytes) + 8 + len(bin_buf)
    header = struct.pack("<III", 0x46546C67, 2, total_len)
    json_chunk = struct.pack("<II", len(json_bytes), 0x4E4F534A) + json_bytes
    bin_chunk = struct.pack("<II", len(bin_buf), 0x004E4942) + bytes(bin_buf)
    output_path.write_bytes(header + json_chunk + bin_chunk)
    return output_path


def _smoothstep(a: float, b: float, x: float) -> float:
    if abs(b - a) < 1e-8:
        return 0.0
    t = max(0.0, min(1.0, (x - a) / (b - a)))
    return t * t * (3.0 - 2.0 * t)


def terrain_height_m(x: float, z: float) -> float:
    """
    Sculpted lakeside glade + underwater basin + forested hills + distant mountain amphitheater.
    Origin (0, 0) sits near shoreline deck; +X opens into the cove bay, -X/sides rise into woodland.
    """
    # Distance from sanctuary center clearing
    dist_plaza = math.hypot(x + 2.5, z)
    # Cove water bowl centered at (+22m, 0m) opening lakeward
    dist_lake = math.hypot(x - 22.0, z * 0.72)

    # 1. Undulating woodland hillside
    undulation = (
        1.15 * math.sin(x * 0.075 + 0.4) * math.cos(z * 0.085 - 0.3)
        + 0.48 * math.sin(x * 0.19 - z * 0.14)
        + 0.22 * math.cos(x * 0.36 + z * 0.29)
    )
    forest_lift = _smoothstep(10.0, 42.0, dist_plaza) * (3.8 + max(0.0, undulation))
    base_y = 0.22 + forest_lift + 0.35 * _smoothstep(6.0, 20.0, dist_plaza) * undulation

    # 2. Carve underwater lake basin (below Z=0 water plane)
    in_lake = 1.0 - _smoothstep(14.5, 21.5, dist_lake)
    cove_lip = (1.0 - _smoothstep(2.0, 14.0, math.hypot(x - 6.0, z))) * _smoothstep(1.5, 5.5, x)
    lake_depth = max(in_lake * 2.9, cove_lip * 1.65)
    y = base_y * (1.0 - max(in_lake, cove_lip)) - lake_depth

    # 3. Level the player's lakeside clearing around Gazebo (-4..+1m, z in -5..+5m)
    plaza_blend = 1.0 - _smoothstep(5.5, 11.5, math.hypot(x + 2.2, z))
    y = y * (1.0 - plaza_blend) + 0.22 * plaza_blend

    # 4. Distant 360° mountain silhouette ring (r = 68m..140m)
    r_world = math.hypot(x, z)
    if r_world > 62.0:
        ang = math.atan2(z, x)
        ridge_factor = _smoothstep(62.0, 128.0, r_world)
        # Keep the distant bay vista lower along +X (-0.35..+0.35 rad) for sunset glow, towering flanks elsewhere
        bay_opening = 1.0 - 0.45 * math.exp(-(ang / 0.55) ** 2)
        peaks = (
            18.0
            + 12.0 * abs(math.sin(ang * 3.0 + 0.7))
            + 7.5 * abs(math.cos(ang * 7.0 - 1.1))
            + 4.5 * math.sin(r_world * 0.14)
        )
        y = max(y, -0.5) + ridge_factor * bay_opening * peaks
    return y


def _terrain_normal(x: float, z: float, eps: float = 0.35) -> Vec3:
    dh_dx = (terrain_height_m(x + eps, z) - terrain_height_m(x - eps, z)) / (2.0 * eps)
    dh_dz = (terrain_height_m(x, z + eps) - terrain_height_m(x, z - eps)) / (2.0 * eps)
    return _norm((-dh_dx, 1.0, -dh_dz))


def build_shore_and_mountain_basin_glb(output_path: Path | None = None) -> Path:
    """Export ShoreAndMountainBasin.glb (260m x 260m radial-sculpted shore & skyline peaks)."""
    materials = [
        {
            "name": "Mat_MossyForestGround",
            "pbrMetallicRoughness": {
                "baseColorFactor": [0.22, 0.30, 0.16, 1.0],
                "metallicFactor": 0.0,
                "roughnessFactor": 0.88,
            },
        },
        {
            "name": "Mat_DistantMountains",
            "pbrMetallicRoughness": {
                "baseColorFactor": [0.16, 0.19, 0.23, 1.0],
                "metallicFactor": 0.0,
                "roughnessFactor": 0.92,
            },
        },
    ]
    ground = UVBufferedPart("ShoreGround", 0)
    peaks = UVBufferedPart("MountainHorizon", 1)

    grid_n = 76
    half_extent = 135.0
    step = (2.0 * half_extent) / grid_n
    uv_tile_m = 4.5

    pts: list[list[Vec3]] = []
    norms: list[list[Vec3]] = []
    for ix in range(grid_n + 1):
        row_p: list[Vec3] = []
        row_n: list[Vec3] = []
        x = -half_extent + ix * step
        for iz in range(grid_n + 1):
            z = -half_extent + iz * step
            y = terrain_height_m(x, z)
            row_p.append((x, y, z))
            row_n.append(_terrain_normal(x, z))
        pts.append(row_p)
        norms.append(row_n)

    for ix in range(grid_n):
        for iz in range(grid_n):
            p00, p01, p11, p10 = pts[ix][iz], pts[ix][iz + 1], pts[ix + 1][iz + 1], pts[ix + 1][iz]
            n00, n01, n11, n10 = norms[ix][iz], norms[ix][iz + 1], norms[ix + 1][iz + 1], norms[ix + 1][iz]
            cx = 0.25 * (p00[0] + p01[0] + p11[0] + p10[0])
            cz = 0.25 * (p00[2] + p01[2] + p11[2] + p10[2])
            target = peaks if math.hypot(cx, cz) >= 68.0 else ground

            uv00 = (p00[0] / uv_tile_m, p00[2] / uv_tile_m)
            uv01 = (p01[0] / uv_tile_m, p01[2] / uv_tile_m)
            uv11 = (p11[0] / uv_tile_m, p11[2] / uv_tile_m)
            uv10 = (p10[0] / uv_tile_m, p10[2] / uv_tile_m)

            target.add_tri(p00, p01, p11, uv00, uv01, uv11, n00, n01, n11)
            target.add_tri(p00, p11, p10, uv00, uv11, uv10, n00, n11, n10)

    out = output_path or (DEFAULT_CUSTOM_MODELS_DIR / "ShoreAndMountainBasin.glb")
    return _export_glb_parts(out, "ShoreAndMountainBasin", materials, [ground, peaks])


def build_lakeside_gazebo_glb(output_path: Path | None = None) -> Path:
    """Export open-air timber stage pergola pavilion casting dappled golden rafters light."""
    materials = [
        {
            "name": "Mat_OiledOakDeck",
            "pbrMetallicRoughness": {
                "baseColorFactor": [0.34, 0.21, 0.12, 1.0],
                "metallicFactor": 0.0,
                "roughnessFactor": 0.62,
            },
        },
        {
            "name": "Mat_AgedCedarFrame",
            "pbrMetallicRoughness": {
                "baseColorFactor": [0.24, 0.14, 0.08, 1.0],
                "metallicFactor": 0.0,
                "roughnessFactor": 0.72,
            },
        },
        {
            "name": "Mat_DarkIronJoinery",
            "pbrMetallicRoughness": {
                "baseColorFactor": [0.10, 0.11, 0.12, 1.0],
                "metallicFactor": 0.85,
                "roughnessFactor": 0.36,
            },
        },
        {
            "name": "Mat_WarmFestoonBulb",
            "pbrMetallicRoughness": {
                "baseColorFactor": [1.0, 0.85, 0.52, 1.0],
                "metallicFactor": 0.0,
                "roughnessFactor": 0.20,
            },
            "emissiveFactor": [1.0, 0.72, 0.32],
        },
    ]
    deck = UVBufferedPart("DeckPlanks", 0)
    timber = UVBufferedPart("TimberPavilion", 1)
    iron = UVBufferedPart("IronBrackets", 2)
    bulbs = UVBufferedPart("FestoonBulbs", 3)

    # Deck planks (4.8m wide in Z, 4.2m deep in X, raised 0.28m)
    num_planks = 18
    plank_w = 4.8 / num_planks
    for i in range(num_planks):
        zc = -2.4 + (i + 0.5) * plank_w
        deck.add_box(0.0, 0.24, zc, 2.1, 0.04, plank_w * 0.46, uv_scale=0.6)
    # Stone/timber foundation skirt + lakeward steps (+X front)
    timber.add_box(0.0, 0.10, 0.0, 2.15, 0.10, 2.45)
    deck.add_box(2.35, 0.14, 0.0, 0.28, 0.04, 1.6)
    deck.add_box(2.75, 0.06, 0.0, 0.28, 0.04, 1.6)

    # 4 corner pillars + iron shoes
    for px in (-1.85, 1.85):
        for pz in (-2.15, 2.15):
            iron.add_box(px, 0.34, pz, 0.11, 0.06, 0.11)
            timber.add_box(px, 1.72, pz, 0.085, 1.40, 0.085)

    # Top perimeter beams
    for pz in (-2.15, 2.15):
        timber.add_box(0.0, 3.06, pz, 2.25, 0.10, 0.09)
    for px in (-1.85, 1.85):
        timber.add_box(px, 3.12, 0.0, 0.09, 0.10, 2.45)

    # Open pergola rafters casting architectural sunset beam shadows
    for i in range(11):
        xr = -1.95 + i * 0.39
        timber.add_box(xr, 3.24, 0.0, 0.045, 0.08, 2.48)

    # Back acoustic privacy slat screen (-X rear) leaving center window open
    for k in range(13):
        yb = 0.55 + k * 0.16
        timber.add_box(-1.85, yb, 0.0, 0.025, 0.045, 2.05)

    # Warm festoon lantern bulbs along the +X lakeside eaves
    for b in range(7):
        bz = -1.8 + b * 0.60
        sag = 0.16 * (1.0 - ((b - 3) / 3.0) ** 2)
        iron.add_cylinder_y(1.88, 2.98 - sag, bz, 3.05 - sag, 0.012, 0.012, segments=8)
        bulbs.add_uv_sphere(1.88, 2.94 - sag, bz, 0.048, stacks=6, slices=10)

    out = output_path or (DEFAULT_CUSTOM_MODELS_DIR / "LakesideGazebo.glb")
    return _export_glb_parts(out, "LakesideGazebo", materials, [deck, timber, iron, bulbs])


def build_music_stand_glb(output_path: Path | None = None) -> Path:
    """Export crafted walnut & brass guitarist music stand with open ivory chord sheet."""
    materials = [
        {
            "name": "Mat_AgedBrass",
            "pbrMetallicRoughness": {
                "baseColorFactor": [0.76, 0.58, 0.28, 1.0],
                "metallicFactor": 0.88,
                "roughnessFactor": 0.30,
            },
        },
        {
            "name": "Mat_WalnutDesk",
            "pbrMetallicRoughness": {
                "baseColorFactor": [0.22, 0.11, 0.06, 1.0],
                "metallicFactor": 0.0,
                "roughnessFactor": 0.38,
            },
        },
        {
            "name": "Mat_IvoryScore",
            "pbrMetallicRoughness": {
                "baseColorFactor": [0.95, 0.91, 0.82, 1.0],
                "metallicFactor": 0.0,
                "roughnessFactor": 0.78,
            },
        },
    ]
    brass = UVBufferedPart("BrassFrame", 0)
    walnut = UVBufferedPart("WalnutDesk", 1)
    paper = UVBufferedPart("SheetMusic", 2)

    # Tripod feet
    for i in range(3):
        ang = 2.0 * math.pi * i / 3.0
        fx, fz = math.cos(ang) * 0.24, math.sin(ang) * 0.24
        brass.add_box(fx * 0.5, 0.04, fz * 0.5, 0.14 if i == 0 else 0.06, 0.018, 0.025)
        brass.add_cylinder_y(fx, 0.0, fz, 0.03, 0.018, 0.014, segments=8)
    # Stem
    brass.add_cylinder_y(0.0, 0.06, 0.0, 0.58, 0.018, 0.018, segments=12)
    brass.add_cylinder_y(0.0, 0.56, 0.0, 1.02, 0.012, 0.012, segments=12)
    brass.add_cylinder_y(0.0, 0.56, 0.0, 0.61, 0.025, 0.025, segments=12)

    # Tilted walnut desk board & ledge
    tilt = math.radians(-26.0)
    ct, st = math.cos(tilt), math.sin(tilt)

    def rot_desk(lx: float, ly: float, lz: float) -> Vec3:
        return (lx * ct - ly * st, 1.08 + lx * st + ly * ct, lz)

    def add_tilted_plate(cx: float, cy: float, cz: float, hx: float, hy: float, hz: float, part: UVBufferedPart) -> None:
        corners = [
            rot_desk(cx - hx, cy - hy, cz - hz),
            rot_desk(cx + hx, cy - hy, cz - hz),
            rot_desk(cx + hx, cy + hy, cz - hz),
            rot_desk(cx - hx, cy + hy, cz - hz),
            rot_desk(cx - hx, cy - hy, cz + hz),
            rot_desk(cx + hx, cy - hy, cz + hz),
            rot_desk(cx + hx, cy + hy, cz + hz),
            rot_desk(cx - hx, cy + hy, cz + hz),
        ]
        # Front/Back (+X facing performer / -X)
        part.add_quad(corners[1], corners[5], corners[6], corners[2])
        part.add_quad(corners[4], corners[0], corners[3], corners[7])
        part.add_quad(corners[3], corners[2], corners[6], corners[7])
        part.add_quad(corners[4], corners[5], corners[1], corners[0])
        part.add_quad(corners[5], corners[4], corners[7], corners[6])
        part.add_quad(corners[0], corners[1], corners[2], corners[3])

    add_tilted_plate(0.0, 0.0, 0.0, 0.010, 0.165, 0.24, walnut)
    add_tilted_plate(-0.026, -0.155, 0.0, 0.028, 0.010, 0.24, walnut)
    # Open parchment chord chart
    add_tilted_plate(-0.014, 0.01, -0.105, 0.004, 0.135, 0.095, paper)
    add_tilted_plate(-0.014, 0.01, 0.105, 0.004, 0.135, 0.095, paper)

    out = output_path or (DEFAULT_CUSTOM_MODELS_DIR / "MusicStand.glb")
    return _export_glb_parts(out, "MusicStand", materials, [brass, walnut, paper])


def build_acoustic_stage_rug_glb(output_path: Path | None = None) -> Path:
    """Export textured woven circular guitarist stage rug with stitched border."""
    materials = [
        {
            "name": "Mat_WovenRugCenter",
            "pbrMetallicRoughness": {
                "baseColorFactor": [0.38, 0.15, 0.11, 1.0],
                "metallicFactor": 0.0,
                "roughnessFactor": 0.92,
            },
        },
        {
            "name": "Mat_RugBorderGold",
            "pbrMetallicRoughness": {
                "baseColorFactor": [0.58, 0.42, 0.22, 1.0],
                "metallicFactor": 0.0,
                "roughnessFactor": 0.88,
            },
        },
    ]
    center = UVBufferedPart("RugCenter", 0)
    border = UVBufferedPart("RugBorder", 1)

    center.add_cylinder_y(0.0, 0.0, 0.0, 0.016, 1.32, 1.30, segments=48, uv_tile=2.0)
    border.add_cylinder_y(0.0, 0.0, 0.0, 0.013, 1.48, 1.45, segments=48, uv_tile=4.0)

    out = output_path or (DEFAULT_CUSTOM_MODELS_DIR / "AcousticStageRug.glb")
    return _export_glb_parts(out, "AcousticStageRug", materials, [center, border])


def build_all_custom_sanctuary_glbs(out_dir: Path = DEFAULT_CUSTOM_MODELS_DIR) -> list[Path]:
    """Generate all custom 3D procedural environment and stage GLB meshes."""
    return [
        build_shore_and_mountain_basin_glb(out_dir / "ShoreAndMountainBasin.glb"),
        build_lakeside_gazebo_glb(out_dir / "LakesideGazebo.glb"),
        build_music_stand_glb(out_dir / "MusicStand.glb"),
        build_acoustic_stage_rug_glb(out_dir / "AcousticStageRug.glb"),
    ]


if __name__ == "__main__":
    for path in build_all_custom_sanctuary_glbs():
        print(f"[OK] Custom 3D GLB -> {path.relative_to(REPO_ROOT)} ({path.stat().st_size:,} B)")
