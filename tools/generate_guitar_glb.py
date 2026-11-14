"""Generate a scale-accurate PBR Acoustic Guitar 3D model (.glb / glTF 2.0) calibrated to UGuitarFretboardComponent."""

from __future__ import annotations

from dataclasses import dataclass, field
import json
import math
from pathlib import Path
import struct

SCALE_LENGTH_M = 0.648
NUT_SPAN_M = 0.036
BRIDGE_SPAN_M = 0.054
FRETBOARD_SURFACE_Y = 0.0


def fret_x_m(fret: int) -> float:
    if fret <= 0:
        return 0.0
    return SCALE_LENGTH_M * (1.0 - (2.0 ** (-fret / 12.0)))


def string_z_m(string_1_to_6: int, x_m: float) -> float:
    alpha = max(0.0, min(1.0, x_m / SCALE_LENGTH_M))
    span = NUT_SPAN_M + (BRIDGE_SPAN_M - NUT_SPAN_M) * alpha
    norm = ((string_1_to_6 - 1) / 5.0) - 0.5
    return norm * span


@dataclass
class MeshPart:
    name: str
    material_idx: int
    positions: list[tuple[float, float, float]] = field(default_factory=list)
    normals: list[tuple[float, float, float]] = field(default_factory=list)
    indices: list[int] = field(default_factory=list)

    def add_tri(
        self,
        p0: tuple[float, float, float],
        p1: tuple[float, float, float],
        p2: tuple[float, float, float],
        n: tuple[float, float, float] | None = None,
    ) -> None:
        if n is None:
            ux, uy, uz = p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2]
            vx, vy, vz = p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2]
            nx = uy * vz - uz * vy
            ny = uz * vx - ux * vz
            nz = ux * vy - uy * vx
            ln = math.sqrt(nx * nx + ny * ny + nz * nz) or 1.0
            n = (nx / ln, ny / ln, nz / ln)
        base = len(self.positions)
        self.positions.extend((p0, p1, p2))
        self.normals.extend((n, n, n))
        self.indices.extend((base, base + 1, base + 2))

    def add_quad(
        self,
        p0: tuple[float, float, float],
        p1: tuple[float, float, float],
        p2: tuple[float, float, float],
        p3: tuple[float, float, float],
    ) -> None:
        self.add_tri(p0, p1, p2)
        self.add_tri(p0, p2, p3)

    def add_box(self, cx: float, cy: float, cz: float, sx: float, sy: float, sz: float) -> None:
        hx, hy, hz = sx * 0.5, sy * 0.5, sz * 0.5
        v = [
            (cx - hx, cy - hy, cz - hz),
            (cx + hx, cy - hy, cz - hz),
            (cx + hx, cy + hy, cz - hz),
            (cx - hx, cy + hy, cz - hz),
            (cx - hx, cy - hy, cz + hz),
            (cx + hx, cy - hy, cz + hz),
            (cx + hx, cy + hy, cz + hz),
            (cx - hx, cy + hy, cz + hz),
        ]
        # Top (+Y), Bottom (-Y), Front (+Z), Back (-Z), Right (+X), Left (-X)
        self.add_quad(v[3], v[7], v[6], v[2])
        self.add_quad(v[0], v[1], v[5], v[4])
        self.add_quad(v[4], v[5], v[6], v[7])
        self.add_quad(v[1], v[0], v[3], v[2])
        self.add_quad(v[5], v[1], v[2], v[6])
        self.add_quad(v[0], v[4], v[7], v[3])

    def add_cylinder_x(
        self,
        x0: float,
        x1: float,
        y: float,
        z0: float,
        z1: float,
        radius: float,
        segments: int = 10,
    ) -> None:
        dx = x1 - x0
        dz = z1 - z0
        ln = math.sqrt(dx * dx + dz * dz) or 1.0
        # Perpendicular basis in (Y, Z-ish)
        perp_z = -dx / ln
        perp_x = dz / ln
        ring0: list[tuple[float, float, float]] = []
        ring1: list[tuple[float, float, float]] = []
        for i in range(segments):
            ang = (2.0 * math.pi * i) / segments
            cy = math.sin(ang) * radius
            cp = math.cos(ang) * radius
            ring0.append((x0 + cp * perp_x, y + cy, z0 + cp * perp_z))
            ring1.append((x1 + cp * perp_x, y + cy, z1 + cp * perp_z))
        for i in range(segments):
            ni = (i + 1) % segments
            self.add_quad(ring0[i], ring1[i], ring1[ni], ring0[ni])

    def add_cylinder_z(
        self,
        x: float,
        y: float,
        z0: float,
        z1: float,
        radius: float,
        segments: int = 10,
    ) -> None:
        ring0: list[tuple[float, float, float]] = []
        ring1: list[tuple[float, float, float]] = []
        for i in range(segments):
            ang = (2.0 * math.pi * i) / segments
            dx = math.cos(ang) * radius
            dy = math.sin(ang) * radius
            ring0.append((x + dx, y + dy, z0))
            ring1.append((x + dx, y + dy, z1))
        for i in range(segments):
            ni = (i + 1) % segments
            self.add_quad(ring0[i], ring0[ni], ring1[ni], ring1[i])

    def add_disc_y(
        self,
        cx: float,
        cy: float,
        cz: float,
        radius: float,
        segments: int = 24,
    ) -> None:
        center = (cx, cy, cz)
        pts = [
            (
                cx + math.cos(2.0 * math.pi * i / segments) * radius,
                cy,
                cz + math.sin(2.0 * math.pi * i / segments) * radius,
            )
            for i in range(segments)
        ]
        for i in range(segments):
            ni = (i + 1) % segments
            self.add_tri(center, pts[ni], pts[i], n=(0.0, 1.0, 0.0))


def _body_half_width(t: float) -> float:
    """Dreadnought acoustic guitar half-width profile for normalized body coordinate t in [0..1]."""
    # t=0: neck-body joint (~14th fret, x=0.36m); t=1: tail block (x=0.86m)
    if t <= 0.0 or t >= 1.0:
        return 0.028
    # Smooth waist + upper bout + lower bout curve
    upper = 0.145 * math.sin(min(1.0, t / 0.45) * math.pi) ** 0.72
    waist_dip = -0.036 * math.exp(-((t - 0.39) / 0.10) ** 2)
    lower = 0.198 * math.sin(max(0.0, (t - 0.25) / 0.75) * math.pi) ** 0.62
    end_round = math.sin(t * math.pi) ** 0.35
    return max(0.028, (max(upper, lower) + waist_dip) * (0.45 + 0.55 * end_round))


def build_acoustic_guitar_parts() -> tuple[list[dict[str, object]], list[MeshPart]]:
    """Build materials and named mesh parts calibrated to 64.8cm scale length."""
    materials: list[dict[str, object]] = [
        {
            "name": "Material_SpruceTop",
            "pbrMetallicRoughness": {
                "baseColorFactor": [0.85, 0.66, 0.42, 1.0],
                "metallicFactor": 0.0,
                "roughnessFactor": 0.36,
            },
        },
        {
            "name": "Material_Mahogany",
            "pbrMetallicRoughness": {
                "baseColorFactor": [0.36, 0.14, 0.07, 1.0],
                "metallicFactor": 0.0,
                "roughnessFactor": 0.42,
            },
        },
        {
            "name": "Material_Ebony",
            "pbrMetallicRoughness": {
                "baseColorFactor": [0.09, 0.08, 0.08, 1.0],
                "metallicFactor": 0.0,
                "roughnessFactor": 0.50,
            },
        },
        {
            "name": "Material_FretChrome",
            "pbrMetallicRoughness": {
                "baseColorFactor": [0.82, 0.84, 0.86, 1.0],
                "metallicFactor": 0.92,
                "roughnessFactor": 0.22,
            },
        },
        {
            "name": "Material_Bone",
            "pbrMetallicRoughness": {
                "baseColorFactor": [0.94, 0.91, 0.84, 1.0],
                "metallicFactor": 0.0,
                "roughnessFactor": 0.35,
            },
        },
        {
            "name": "Material_SoundholeDark",
            "pbrMetallicRoughness": {
                "baseColorFactor": [0.02, 0.015, 0.015, 1.0],
                "metallicFactor": 0.0,
                "roughnessFactor": 0.95,
            },
        },
        {
            "name": "Material_BronzeString",
            "pbrMetallicRoughness": {
                "baseColorFactor": [0.78, 0.56, 0.32, 1.0],
                "metallicFactor": 0.95,
                "roughnessFactor": 0.24,
            },
        },
    ]

    parts: list[MeshPart] = []

    # 1. Dreadnought body top (spruce) + back/sides (mahogany)
    body_top = MeshPart("GuitarBody_Top", 0)
    body_sides = MeshPart("GuitarBody_BackSides", 1)
    x_joint = fret_x_m(14)  # ~0.359 m
    body_len = 0.505
    body_depth = 0.105
    top_y = -0.0055
    bot_y = top_y - body_depth

    slices = 44
    ring_pts: list[tuple[float, float]] = []
    for i in range(slices + 1):
        t = i / slices
        x = x_joint + t * body_len
        hw = _body_half_width(t)
        ring_pts.append((x, hw))

    for i in range(slices):
        x0, w0 = ring_pts[i]
        x1, w1 = ring_pts[i + 1]
        # Top plate (spruce)
        body_top.add_quad((x0, top_y, -w0), (x0, top_y, w0), (x1, top_y, w1), (x1, top_y, -w1))
        # Bottom plate (mahogany)
        body_sides.add_quad((x0, bot_y, w0), (x0, bot_y, -w0), (x1, bot_y, -w1), (x1, bot_y, w1))
        # Left & right ribs
        body_sides.add_quad((x0, bot_y, -w0), (x0, top_y, -w0), (x1, top_y, -w1), (x1, bot_y, -w1))
        body_sides.add_quad((x0, top_y, w0), (x0, bot_y, w0), (x1, bot_y, w1), (x1, top_y, w1))

    # End caps
    x_s, w_s = ring_pts[0]
    x_e, w_e = ring_pts[-1]
    body_sides.add_quad((x_s, bot_y, -w_s), (x_s, bot_y, w_s), (x_s, top_y, w_s), (x_s, top_y, -w_s))
    body_sides.add_quad((x_e, bot_y, w_e), (x_e, bot_y, -w_e), (x_e, top_y, -w_e), (x_e, top_y, w_e))
    parts.extend([body_top, body_sides])

    # 2. Soundhole & decorative rosette centered at X = 0.485 m
    soundhole = MeshPart("Soundhole", 5)
    soundhole.add_disc_y(0.485, top_y + 0.0005, 0.0, 0.049, segments=32)
    parts.append(soundhole)

    # 3. Fretboard (ebony, tapered width from nut to 20th fret)
    fretboard = MeshPart("Fretboard", 2)
    fb_x0 = 0.0
    fb_x1 = fret_x_m(20) + 0.012
    fb_top = FRETBOARD_SURFACE_Y
    fb_bot = top_y
    w_nut = 0.0215
    w_end = 0.0285
    fretboard.add_quad((fb_x0, fb_top, -w_nut), (fb_x0, fb_top, w_nut), (fb_x1, fb_top, w_end), (fb_x1, fb_top, -w_end))
    fretboard.add_quad((fb_x0, fb_bot, w_nut), (fb_x0, fb_bot, -w_nut), (fb_x1, fb_bot, -w_end), (fb_x1, fb_bot, w_end))
    fretboard.add_quad((fb_x0, fb_bot, -w_nut), (fb_x0, fb_top, -w_nut), (fb_x1, fb_top, -w_end), (fb_x1, fb_bot, -w_end))
    fretboard.add_quad((fb_x0, fb_top, w_nut), (fb_x0, fb_bot, w_nut), (fb_x1, fb_bot, w_end), (fb_x1, fb_top, w_end))
    parts.append(fretboard)

    # 4. Neck shaft + Headstock (mahogany)
    neck = MeshPart("GuitarNeck", 1)
    neck.add_box(x_joint * 0.5, fb_bot - 0.010, 0.0, x_joint, 0.020, 0.044)
    neck.add_box(-0.085, fb_bot - 0.004, 0.0, 0.170, 0.015, 0.072)
    parts.append(neck)

    # 5. Bone Nut at X = 0 + Bridge & Saddle at X = 0.648
    nut = MeshPart("Nut", 4)
    nut.add_box(-0.002, fb_top + 0.0018, 0.0, 0.004, 0.0036, 0.043)
    parts.append(nut)

    bridge = MeshPart("BridgeBase", 2)
    bridge.add_box(SCALE_LENGTH_M + 0.006, top_y + 0.004, 0.0, 0.034, 0.008, 0.148)
    parts.append(bridge)

    saddle = MeshPart("BridgeSaddle", 4)
    saddle.add_box(SCALE_LENGTH_M, top_y + 0.0085, 0.0, 0.0035, 0.005, 0.072)
    parts.append(saddle)

    # 6. 20 Nickel-Silver Fret Wires at exact scale-length positions
    frets_mesh = MeshPart("FretWires", 3)
    for f in range(1, 21):
        fx = fret_x_m(f)
        alpha = max(0.0, min(1.0, fx / fb_x1))
        hw = w_nut + (w_end - w_nut) * alpha
        frets_mesh.add_cylinder_z(fx, fb_top + 0.0008, -hw, hw, 0.0009, segments=8)
    parts.append(frets_mesh)

    # 7. Fretboard Pearl Inlay Dots (3, 5, 7, 9, 12x2, 15, 17)
    inlays = MeshPart("FretInlays", 4)
    for f in (3, 5, 7, 9, 12, 15, 17):
        mid_x = 0.5 * (fret_x_m(f - 1) + fret_x_m(f))
        if f == 12:
            inlays.add_disc_y(mid_x, fb_top + 0.0003, -0.009, 0.0026, segments=14)
            inlays.add_disc_y(mid_x, fb_top + 0.0003, 0.009, 0.0026, segments=14)
        else:
            inlays.add_disc_y(mid_x, fb_top + 0.0003, 0.0, 0.0028, segments=14)
    parts.append(inlays)

    # 8. 6 Individual Strings (String_1 High E .. String_6 Low E)
    string_y = fb_top + 0.0032
    for s in range(1, 7):
        s_part = MeshPart(f"String_{s}", 6 if s >= 3 else 3)
        z0 = string_z_m(s, 0.0)
        z1 = string_z_m(s, SCALE_LENGTH_M)
        radius = 0.00035 + (s - 1) * 0.00012
        s_part.add_cylinder_x(0.0, SCALE_LENGTH_M, string_y, z0, z1, radius, segments=8)
        parts.append(s_part)

    # 9. 6 Chrome Tuning Pegs on Headstock
    pegs = MeshPart("TuningPegs", 3)
    for idx in range(3):
        px = -0.045 - idx * 0.036
        pegs.add_box(px, fb_bot - 0.002, -0.042, 0.014, 0.010, 0.018)
        pegs.add_box(px, fb_bot - 0.002, 0.042, 0.014, 0.010, 0.018)
    parts.append(pegs)

    return materials, parts


def export_acoustic_guitar_glb(output_path: str | Path) -> Path:
    """Write a binary glTF 2.0 (.glb) acoustic guitar file."""
    out = Path(output_path)
    out.parent.mkdir(parents=True, exist_ok=True)
    materials, parts = build_acoustic_guitar_parts()

    bin_buf = bytearray()
    buffer_views: list[dict[str, object]] = []
    accessors: list[dict[str, object]] = []
    meshes: list[dict[str, object]] = []
    nodes: list[dict[str, object]] = []

    def append_aligned(data: bytes) -> tuple[int, int]:
        while len(bin_buf) % 4 != 0:
            bin_buf.append(0)
        offset = len(bin_buf)
        bin_buf.extend(data)
        return offset, len(data)

    for part_idx, part in enumerate(parts):
        pos_bytes = bytearray()
        norm_bytes = bytearray()
        idx_bytes = bytearray()

        min_xyz = [float("inf"), float("inf"), float("inf")]
        max_xyz = [float("-inf"), float("-inf"), float("-inf")]

        for (px, py, pz), (nx, ny, nz) in zip(part.positions, part.normals):
            pos_bytes.extend(struct.pack("<fff", px, py, pz))
            norm_bytes.extend(struct.pack("<fff", nx, ny, nz))
            min_xyz[0], min_xyz[1], min_xyz[2] = min(min_xyz[0], px), min(min_xyz[1], py), min(min_xyz[2], pz)
            max_xyz[0], max_xyz[1], max_xyz[2] = max(max_xyz[0], px), max(max_xyz[1], py), max(max_xyz[2], pz)

        for idx_val in part.indices:
            idx_bytes.extend(struct.pack("<I", idx_val))

        pos_off, pos_len = append_aligned(bytes(pos_bytes))
        norm_off, norm_len = append_aligned(bytes(norm_bytes))
        idx_off, idx_len = append_aligned(bytes(idx_bytes))

        bv_pos = len(buffer_views)
        buffer_views.append({"buffer": 0, "byteOffset": pos_off, "byteLength": pos_len, "target": 34962})
        bv_norm = len(buffer_views)
        buffer_views.append({"buffer": 0, "byteOffset": norm_off, "byteLength": norm_len, "target": 34962})
        bv_idx = len(buffer_views)
        buffer_views.append({"buffer": 0, "byteOffset": idx_off, "byteLength": idx_len, "target": 34963})

        acc_pos = len(accessors)
        accessors.append(
            {
                "bufferView": bv_pos,
                "componentType": 5126,
                "count": len(part.positions),
                "type": "VEC3",
                "min": [round(v, 6) for v in min_xyz],
                "max": [round(v, 6) for v in max_xyz],
            }
        )
        acc_norm = len(accessors)
        accessors.append(
            {
                "bufferView": bv_norm,
                "componentType": 5126,
                "count": len(part.normals),
                "type": "VEC3",
            }
        )
        acc_idx = len(accessors)
        accessors.append(
            {
                "bufferView": bv_idx,
                "componentType": 5125,
                "count": len(part.indices),
                "type": "SCALAR",
            }
        )

        meshes.append(
            {
                "name": part.name,
                "primitives": [
                    {
                        "attributes": {"POSITION": acc_pos, "NORMAL": acc_norm},
                        "indices": acc_idx,
                        "material": part.material_idx,
                    }
                ],
            }
        )
        nodes.append({"name": part.name, "mesh": part_idx})

    while len(bin_buf) % 4 != 0:
        bin_buf.append(0)

    root_idx = len(nodes)
    nodes.append({"name": "AcousticGuitarRoot", "children": list(range(len(parts)))})

    gltf = {
        "asset": {"version": "2.0", "generator": "RealGuitarGame_GLBBuilder"},
        "scene": 0,
        "scenes": [{"name": "AcousticGuitar", "nodes": [root_idx]}],
        "nodes": nodes,
        "meshes": meshes,
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

    out.write_bytes(header + json_chunk + bin_chunk)
    return out


if __name__ == "__main__":
    target = Path(__file__).resolve().parent.parent / "ue5" / "RealGuitarGame" / "Content" / "Models" / "AcousticGuitar_Reference.glb"
    res = export_acoustic_guitar_glb(target)
    print(f"[OK] Exported calibrated 3D Acoustic Guitar GLB -> {res} ({res.stat().st_size} bytes)")
