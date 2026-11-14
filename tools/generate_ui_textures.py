"""Pure-Python anti-aliased RGBA PNG UI icon & 9-slice dark-glass panel generator for UE5 UMG."""

from __future__ import annotations

import math
from pathlib import Path
import struct
import zlib
from typing import Callable, Iterable

REPO_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_UI_DIR = REPO_ROOT / "ue5" / "RealGuitarGame" / "Content" / "UI" / "Textures"

PixelEval = Callable[[float, float], tuple[int, int, int, int]]


def _clamp01(v: float) -> float:
    return 0.0 if v < 0.0 else (1.0 if v > 1.0 else v)


def _smoothstep(edge0: float, edge1: float, x: float) -> float:
    t = _clamp01((x - edge0) / (edge1 - edge0))
    return t * t * (3.0 - 2.0 * t)


def _sd_circle(px: float, py: float, cx: float, cy: float, r: float) -> float:
    return math.hypot(px - cx, py - cy) - r


def _sd_box(px: float, py: float, cx: float, cy: float, hx: float, hy: float, r: float = 0.0) -> float:
    dx = abs(px - cx) - hx + r
    dy = abs(py - cy) - hy + r
    outside = math.hypot(max(dx, 0.0), max(dy, 0.0))
    inside = min(max(dx, dy), 0.0)
    return outside + inside - r


def _sd_segment(px: float, py: float, ax: float, ay: float, bx: float, by: float) -> float:
    vx, vy = bx - ax, by - ay
    wx, wy = px - ax, py - ay
    denom = vx * vx + vy * vy
    h = 0.0 if denom <= 1e-8 else _clamp01((wx * vx + wy * vy) / denom)
    return math.hypot(wx - vx * h, wy - vy * h)


def _sd_triangle(px: float, py: float, p0: tuple[float, float], p1: tuple[float, float], p2: tuple[float, float]) -> float:
    e0 = (p1[0] - p0[0], p1[1] - p0[1])
    e1 = (p2[0] - p1[0], p2[1] - p1[1])
    e2 = (p0[0] - p2[0], p0[1] - p2[1])
    v0 = (px - p0[0], py - p0[1])
    v1 = (px - p1[0], py - p1[1])
    v2 = (px - p2[0], py - p2[1])

    def _clamp_dot(vx: float, vy: float, ex: float, ey: float) -> tuple[float, float]:
        denom = ex * ex + ey * ey
        t = 0.0 if denom <= 1e-8 else _clamp01((vx * ex + vy * ey) / denom)
        return (vx - ex * t, vy - ey * t)

    pq0 = _clamp_dot(*v0, *e0)
    pq1 = _clamp_dot(*v1, *e1)
    pq2 = _clamp_dot(*v2, *e2)
    s = 1.0 if (e0[0] * e2[1] - e0[1] * e2[0]) >= 0.0 else -1.0

    d0 = (pq0[0] ** 2 + pq0[1] ** 2, s * (v0[0] * e0[1] - v0[1] * e0[0]))
    d1 = (pq1[0] ** 2 + pq1[1] ** 2, s * (v1[0] * e1[1] - v1[1] * e1[0]))
    d2 = (pq2[0] ** 2 + pq2[1] ** 2, s * (v2[0] * e2[1] - v2[1] * e2[0]))

    dist2 = min(d0[0], d1[0], d2[0])
    min_cross = min(d0[1], d1[1], d2[1])
    return -math.sqrt(dist2) if min_cross > 0.0 else math.sqrt(dist2)


def _alpha_from_sd(sd: float, aa: float = 1.4) -> int:
    return int(round(255.0 * (1.0 - _smoothstep(-aa, aa, sd))))


def _white_icon(sd: float) -> tuple[int, int, int, int]:
    return (248, 244, 236, _alpha_from_sd(sd))


def encode_rgba_png(width: int, height: int, rgba_rows: Iterable[bytes]) -> bytes:
    """Encode raw RGBA scanlines into a standards-compliant PNG byte payload."""
    def chunk(tag: bytes, data: bytes) -> bytes:
        body = tag + data
        return struct.pack(">I", len(data)) + body + struct.pack(">I", zlib.crc32(body) & 0xFFFFFFFF)

    ihdr = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    raw = bytearray()
    for row in rgba_rows:
        raw.append(0)  # Filter type 0 (None)
        raw.extend(row)
    idat = zlib.compress(bytes(raw), level=6)
    return b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", ihdr) + chunk(b"IDAT", idat) + chunk(b"IEND", b"")


def render_png(path: Path, width: int, height: int, evaluator: PixelEval) -> Path:
    path.parent.mkdir(parents=True, exist_ok=True)
    rows: list[bytes] = []
    for y in range(height):
        row = bytearray(width * 4)
        fy = y + 0.5
        for x in range(width):
            r, g, b, a = evaluator(x + 0.5, fy)
            idx = x * 4
            row[idx : idx + 4] = bytes((r, g, b, a))
        rows.append(bytes(row))
    path.write_bytes(encode_rgba_png(width, height, rows))
    return path


def _eval_play(x: float, y: float) -> tuple[int, int, int, int]:
    d = _sd_triangle(x, y, (42.0, 28.0), (42.0, 100.0), (96.0, 64.0)) - 4.0
    return _white_icon(d)


def _eval_pause(x: float, y: float) -> tuple[int, int, int, int]:
    d1 = _sd_box(x, y, 47.0, 64.0, 9.0, 32.0, 4.0)
    d2 = _sd_box(x, y, 81.0, 64.0, 9.0, 32.0, 4.0)
    return _white_icon(min(d1, d2))


def _eval_next(x: float, y: float) -> tuple[int, int, int, int]:
    tri = _sd_triangle(x, y, (34.0, 32.0), (34.0, 96.0), (80.0, 64.0)) - 2.0
    bar = _sd_box(x, y, 90.0, 64.0, 6.0, 32.0, 3.0)
    return _white_icon(min(tri, bar))


def _eval_prev(x: float, y: float) -> tuple[int, int, int, int]:
    bar = _sd_box(x, y, 38.0, 64.0, 6.0, 32.0, 3.0)
    tri = _sd_triangle(x, y, (94.0, 32.0), (94.0, 96.0), (48.0, 64.0)) - 2.0
    return _white_icon(min(bar, tri))


def _eval_loop(x: float, y: float) -> tuple[int, int, int, int]:
    ring = abs(_sd_circle(x, y, 64.0, 64.0, 30.0)) - 5.5
    gap = _sd_box(x, y, 88.0, 40.0, 14.0, 14.0)
    arc = max(ring, -gap)
    arrow = _sd_triangle(x, y, (80.0, 26.0), (98.0, 36.0), (78.0, 46.0))
    return _white_icon(min(arc, arrow))


def _eval_camera(x: float, y: float) -> tuple[int, int, int, int]:
    body = _sd_box(x, y, 64.0, 68.0, 36.0, 24.0, 7.0)
    top_bump = _sd_box(x, y, 64.0, 42.0, 15.0, 8.0, 4.0)
    lens_outer = _sd_circle(x, y, 64.0, 68.0, 14.0)
    lens_inner = _sd_circle(x, y, 64.0, 68.0, 8.0)
    shell = min(body, top_bump)
    shell_with_cut = max(shell, -max(lens_outer, -lens_inner))
    return _white_icon(shell_with_cut)


def _eval_settings(x: float, y: float) -> tuple[int, int, int, int]:
    dx, dy = x - 64.0, y - 64.0
    r = math.hypot(dx, dy)
    ang = math.atan2(dy, dx)
    tooth = 4.5 * math.cos(8.0 * ang)
    outer = (r - 27.0) - max(-2.0, tooth)
    hole = 12.0 - r
    return _white_icon(max(outer, hole))


def _eval_mood_sunset(x: float, y: float) -> tuple[int, int, int, int]:
    sun = _sd_circle(x, y, 64.0, 64.0, 22.0)
    horizon_cut = 68.0 - y
    half_sun = max(sun, -horizon_cut)
    bar1 = _sd_segment(x, y, 24.0, 76.0, 104.0, 76.0) - 4.0
    bar2 = _sd_segment(x, y, 36.0, 90.0, 92.0, 90.0) - 3.5
    return _white_icon(min(half_sun, bar1, bar2))


def _eval_mood_night(x: float, y: float) -> tuple[int, int, int, int]:
    moon_outer = _sd_circle(x, y, 60.0, 66.0, 28.0)
    moon_cut = _sd_circle(x, y, 73.0, 56.0, 24.0)
    crescent = max(moon_outer, -moon_cut)
    star = _sd_circle(x, y, 92.0, 38.0, 4.5)
    return _white_icon(min(crescent, star))


def _eval_sit_bench(x: float, y: float) -> tuple[int, int, int, int]:
    backrest = _sd_box(x, y, 64.0, 46.0, 36.0, 8.0, 3.0)
    seat = _sd_box(x, y, 64.0, 68.0, 40.0, 6.0, 3.0)
    leg_l = _sd_box(x, y, 34.0, 82.0, 5.0, 18.0, 2.0)
    leg_r = _sd_box(x, y, 94.0, 82.0, 5.0, 18.0, 2.0)
    return _white_icon(min(backrest, seat, leg_l, leg_r))


def _eval_glass_panel_9slice(x: float, y: float) -> tuple[int, int, int, int]:
    sd_panel = _sd_box(x, y, 128.0, 128.0, 120.0, 120.0, 22.0)
    fill_a = 1.0 - _smoothstep(-1.5, 1.5, sd_panel)
    rim_band = 1.0 - _smoothstep(0.0, 2.2, abs(sd_panel + 1.5))
    r = int(round(16 * (1.0 - rim_band) + 224 * rim_band))
    g = int(round(16 * (1.0 - rim_band) + 169 * rim_band))
    b = int(round(20 * (1.0 - rim_band) + 109 * rim_band))
    a = int(round(255.0 * fill_a * (0.86 * (1.0 - rim_band) + 0.92 * rim_band)))
    return (r, g, b, a)


def generate_ui_texture_pack(out_dir: Path = DEFAULT_UI_DIR) -> list[Path]:
    """Generate crisp anti-aliased UI icons and dark-glass 9-slice panel PNGs."""
    specs: list[tuple[str, int, int, PixelEval]] = [
        ("T_UI_Play.png", 128, 128, _eval_play),
        ("T_UI_Pause.png", 128, 128, _eval_pause),
        ("T_UI_Next.png", 128, 128, _eval_next),
        ("T_UI_Prev.png", 128, 128, _eval_prev),
        ("T_UI_Loop.png", 128, 128, _eval_loop),
        ("T_UI_Camera.png", 128, 128, _eval_camera),
        ("T_UI_Settings.png", 128, 128, _eval_settings),
        ("T_UI_MoodSunset.png", 128, 128, _eval_mood_sunset),
        ("T_UI_MoodNight.png", 128, 128, _eval_mood_night),
        ("T_UI_SitBench.png", 128, 128, _eval_sit_bench),
        ("T_UI_GlassPanel_9Slice.png", 256, 256, _eval_glass_panel_9slice),
    ]
    return [render_png(out_dir / name, w, h, fn) for name, w, h, fn in specs]


if __name__ == "__main__":
    written = generate_ui_texture_pack()
    for p in written:
        print(f"[OK] UI texture -> {p.relative_to(REPO_ROOT)}")
