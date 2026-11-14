"""Generate a custom multi-resolution Windows .ico (Acoustic Guitar Lakeside Sunset badge) for RealGuitarGame."""

from __future__ import annotations

import math
from pathlib import Path
import struct

from tools.generate_ui_textures import _clamp01, _sd_box, _sd_circle, _sd_segment, _smoothstep, encode_rgba_png

REPO_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_APP_ICON_PATH = REPO_ROOT / "ue5" / "RealGuitarGame" / "Build" / "Windows" / "Application.ico"
ICON_SIZES: tuple[int, ...] = (16, 24, 32, 40, 48, 64, 256)


def _mix_rgb(c0: tuple[float, float, float], c1: tuple[float, float, float], t: float) -> tuple[float, float, float]:
    t = _clamp01(t)
    return (
        c0[0] + (c1[0] - c0[0]) * t,
        c0[1] + (c1[1] - c0[1]) * t,
        c0[2] + (c1[2] - c0[2]) * t,
    )


def _smin(a: float, b: float, k: float) -> float:
    h = _clamp01(0.5 + 0.5 * (b - a) / k)
    return a * h + b * (1.0 - h) - k * h * (1.0 - h)


def _eval_app_icon_pixel(nx: float, ny: float) -> tuple[int, int, int, int]:
    """Evaluate normalized [-1, +1]^2 coordinate for the Acoustic Sanctuary sunset guitar emblem."""
    # Rounded squircle tile SDF + outer soft shadow / warm golden rim
    sd_tile = _sd_box(nx, ny, 0.0, 0.0, 0.87, 0.87, 0.25)
    tile_alpha = 1.0 - _smoothstep(-0.008, 0.008, sd_tile)
    if tile_alpha <= 0.002:
        return (0, 0, 0, 0)

    # Background: smooth twilight sky -> sunset mist horizon -> calm reflective lake
    horizon_y = 0.22
    t_sky = _clamp01((ny + 0.90) / (horizon_y + 0.90))
    sky_top = (18.0, 26.0, 48.0)
    sky_mid = (84.0, 52.0, 82.0)
    sky_hor = (236.0, 128.0, 58.0)
    sky_col = _mix_rgb(
        _mix_rgb(sky_top, sky_mid, _smoothstep(0.0, 0.68, t_sky)),
        sky_hor,
        _smoothstep(0.32, 1.0, t_sky),
    )

    t_water = _clamp01((ny - horizon_y) / (0.92 - horizon_y))
    water_col = _mix_rgb((40.0, 52.0, 68.0), (12.0, 22.0, 34.0), _smoothstep(0.0, 0.85, t_water))
    ripple = 0.5 + 0.5 * math.sin(ny * 48.0)
    sun_band = math.exp(-((nx + 0.14) ** 2) / (0.016 + 0.055 * t_water)) * (1.0 - t_water * 0.68)
    water_col = _mix_rgb(water_col, (245.0, 174.0, 92.0), sun_band * (0.30 + 0.24 * ripple))

    water_blend = _smoothstep(horizon_y - 0.014, horizon_y + 0.014, ny)
    bg = _mix_rgb(sky_col, water_col, water_blend)

    # Setting sun disc behind guitar + soft sunset glow
    sun_d = _sd_circle(nx, ny, -0.14, 0.10, 0.27)
    sun_glow = math.exp(-max(sun_d + 0.27, 0.0) * 3.6) * (0.58 * (1.0 - water_blend) + 0.18 * water_blend)
    bg = _mix_rgb(bg, (252.0, 200.0, 120.0), sun_glow * 0.68)
    sun_core = (1.0 - _smoothstep(-0.012, 0.012, sun_d)) * (1.0 - water_blend)
    bg = _mix_rgb(bg, (255.0, 228.0, 158.0), sun_core * 0.88)

    # Golden outer bevel highlight on the tile border
    rim_band = 1.0 - _smoothstep(0.0, 0.022, abs(sd_tile + 0.016))
    bg = _mix_rgb(bg, (226.0, 172.0, 98.0), rim_band * 0.78)

    # Transform to guitar-local coords (tilted -38 deg so neck reaches upper-right corner)
    gx, gy = nx + 0.04, ny - 0.06
    ang = math.radians(38.0)
    ca, sa = math.cos(ang), math.sin(ang)
    lx = gx * ca - gy * sa
    ly = gx * sa + gy * ca

    # Smooth figure-8 guitar body SDF using C-infinity Gaussian waist contour
    def _guitar_body_sd(px: float, py: float) -> float:
        low = _sd_circle(px, py, -0.17, 0.0, 0.25)
        upp = _sd_circle(px, py, 0.11, 0.0, 0.19)
        waist = 0.032 * math.exp(-((px + 0.025) / 0.072) ** 2)
        return _smin(low, upp, 0.13) + waist

    # Soft shadow behind guitar
    sd_body_sh = _guitar_body_sd(lx - 0.03, ly - 0.04)
    sd_neck_sh = _sd_box(lx - 0.03, ly - 0.04, 0.44, 0.0, 0.28, 0.044, 0.012)
    sd_guitar_sh = min(sd_body_sh, sd_neck_sh)
    bg = _mix_rgb(bg, (8.0, 10.0, 16.0), (1.0 - _smoothstep(-0.02, 0.09, sd_guitar_sh)) * 0.58)

    sd_body = _guitar_body_sd(lx, ly)

    # Neck shaft + flared headstock
    sd_neck = _sd_box(lx, ly, 0.43, 0.0, 0.24, 0.042, 0.008)
    sd_head = _sd_box(lx, ly, 0.71, 0.0, 0.085, 0.066, 0.018)

    col = bg
    # Draw neck & headstock base (warm dark mahogany)
    neck_a = 1.0 - _smoothstep(-0.008, 0.008, min(sd_neck, sd_head))
    col = _mix_rgb(col, (76.0, 40.0, 24.0), neck_a)

    # Tuning pegs on headstock
    for px in (0.67, 0.715, 0.76):
        for py in (-0.075, 0.075):
            peg_a = 1.0 - _smoothstep(-0.008, 0.008, _sd_circle(lx, ly, px, py, 0.016))
            col = _mix_rgb(col, (232.0, 212.0, 174.0), peg_a)

    # Draw spruce guitar body with rich mahogany rim bevel
    body_a = 1.0 - _smoothstep(-0.008, 0.008, sd_body)
    if body_a > 0.0:
        rim_t = 1.0 - _smoothstep(-0.032, -0.012, sd_body)
        radial = _clamp01(math.hypot(lx + 0.05, ly) / 0.36)
        spruce = _mix_rgb((240.0, 186.0, 108.0), (196.0, 114.0, 52.0), radial)
        mahogany_edge = (96.0, 42.0, 22.0)
        body_col = _mix_rgb(mahogany_edge, spruce, rim_t)
        col = _mix_rgb(col, body_col, body_a)

    # Ebony fretboard from soundhole to nut
    fb_half_w = 0.046 - 0.012 * _clamp01((lx - 0.14) / 0.48)
    sd_fb = _sd_box(lx, ly, 0.385, 0.0, 0.255, fb_half_w, 0.005)
    fb_a = 1.0 - _smoothstep(-0.007, 0.007, sd_fb)
    col = _mix_rgb(col, (34.0, 26.0, 24.0), fb_a)

    # Fret wire glints across fretboard
    if fb_a > 0.15:
        for fx in (0.19, 0.25, 0.315, 0.38, 0.445, 0.51, 0.575, 0.632):
            fw = 1.0 - _smoothstep(0.0, 0.006, _sd_segment(lx, ly, fx, -fb_half_w * 0.85, fx, fb_half_w * 0.85))
            col = _mix_rgb(col, (220.0, 206.0, 182.0), fw * 0.7)

    # Soundhole + golden rosette ring
    sh_d = _sd_circle(lx, ly, 0.05, 0.0, 0.072)
    rosette = 1.0 - _smoothstep(0.0, 0.012, abs(sh_d - 0.016))
    col = _mix_rgb(col, (242.0, 204.0, 134.0), rosette * 0.85)
    col = _mix_rgb(col, (18.0, 14.0, 14.0), 1.0 - _smoothstep(-0.007, 0.007, sh_d))

    # Ebony bridge + cream saddle highlight
    sd_bridge = _sd_box(lx, ly, -0.19, 0.0, 0.026, 0.095, 0.008)
    col = _mix_rgb(col, (36.0, 24.0, 20.0), 1.0 - _smoothstep(-0.007, 0.007, sd_bridge))
    sd_saddle = _sd_segment(lx, ly, -0.182, -0.062, -0.182, 0.062)
    col = _mix_rgb(col, (244.0, 232.0, 204.0), (1.0 - _smoothstep(0.0, 0.007, sd_saddle)) * 0.9)

    # 6 golden strings from bridge (-0.182) to nut (0.635)
    for s_idx in range(6):
        sy = ((s_idx / 5.0) - 0.5) * 0.058
        str_d = _sd_segment(lx, ly, -0.182, sy, 0.635, sy * 0.72)
        str_a = 1.0 - _smoothstep(0.0, 0.0055, str_d)
        col = _mix_rgb(col, (250.0, 224.0, 168.0), str_a * 0.82)

    r = int(_clamp01(col[0] / 255.0) * 255)
    g = int(_clamp01(col[1] / 255.0) * 255)
    b = int(_clamp01(col[2] / 255.0) * 255)
    a = int(_clamp01(tile_alpha) * 255)
    return (r, g, b, a)


def _render_master_rgba(master_size: int = 512) -> list[tuple[int, int, int, int]]:
    pixels: list[tuple[int, int, int, int]] = []
    half = master_size * 0.5
    for y in range(master_size):
        ny = (y + 0.5 - half) / half
        for x in range(master_size):
            nx = (x + 0.5 - half) / half
            pixels.append(_eval_app_icon_pixel(nx, ny))
    return pixels


def _downsample_rgba(master: list[tuple[int, int, int, int]], master_size: int, target_size: int) -> list[bytes]:
    block = master_size // target_size
    inv = 1.0 / (block * block)
    rows: list[bytes] = []
    for ty in range(target_size):
        row = bytearray(target_size * 4)
        sy0 = ty * block
        for tx in range(target_size):
            sx0 = tx * block
            ra = ga = ba = aa = 0
            for dy in range(block):
                base = (sy0 + dy) * master_size + sx0
                for dx in range(block):
                    pr, pg, pb, pa = master[base + dx]
                    ra += pr * pa
                    ga += pg * pa
                    ba += pb * pa
                    aa += pa
            avg_a = int(round(aa * inv))
            idx = tx * 4
            if avg_a > 0:
                row[idx] = min(255, int(round(ra / aa)))
                row[idx + 1] = min(255, int(round(ga / aa)))
                row[idx + 2] = min(255, int(round(ba / aa)))
                row[idx + 3] = avg_a
        rows.append(bytes(row))
    return rows


def generate_windows_app_icon(
    out_ico: Path = DEFAULT_APP_ICON_PATH,
    sizes: tuple[int, ...] = ICON_SIZES,
) -> Path:
    """Write a Windows Vista+ multi-resolution PNG-in-ICO file usable by UE5 BuildTool & Windows shortcuts."""
    out_ico.parent.mkdir(parents=True, exist_ok=True)
    master_size = 512
    master = _render_master_rgba(master_size)

    # Pre-render frames exact integer divisors of 512; bilinear for non-power-of-two (24, 40, 48)
    frames: list[tuple[int, bytes]] = []
    for sz in sizes:
        if master_size % sz == 0:
            rows = _downsample_rgba(master, master_size, sz)
        else:
            # Supersample 2x grid cleanly around target size
            rows = []
            half = sz * 0.5
            for y in range(sz):
                row = bytearray(sz * 4)
                for x in range(sz):
                    ra = ga = ba = aa = 0
                    for sub_y in (0.25, 0.75):
                        for sub_x in (0.25, 0.75):
                            nx = (x + sub_x - half) / half
                            ny = (y + sub_y - half) / half
                            pr, pg, pb, pa = _eval_app_icon_pixel(nx, ny)
                            ra += pr * pa
                            ga += pg * pa
                            ba += pb * pa
                            aa += pa
                    avg_a = int(round(aa * 0.25))
                    idx = x * 4
                    if avg_a > 0:
                        row[idx] = min(255, int(round(ra / aa)))
                        row[idx + 1] = min(255, int(round(ga / aa)))
                        row[idx + 2] = min(255, int(round(ba / aa)))
                        row[idx + 3] = avg_a
                rows.append(bytes(row))
        frames.append((sz, encode_rgba_png(sz, sz, rows)))

    # Pack ICONDIR (6 bytes) + ICONDIRENTRY[] (16 bytes each) + PNG payloads
    count = len(frames)
    header = struct.pack("<HHH", 0, 1, count)
    offset = 6 + 16 * count
    entries = bytearray()
    payloads = bytearray()
    for sz, png_bytes in frames:
        wb = 0 if sz >= 256 else sz
        hb = 0 if sz >= 256 else sz
        entries.extend(struct.pack("<BBBBHHII", wb, hb, 0, 0, 1, 32, len(png_bytes), offset))
        payloads.extend(png_bytes)
        offset += len(png_bytes)

    out_ico.write_bytes(header + bytes(entries) + bytes(payloads))
    return out_ico


if __name__ == "__main__":
    ico = generate_windows_app_icon()
    print(f"[OK] Generated UE5 Windows app icon -> {ico.relative_to(REPO_ROOT)} ({ico.stat().st_size} bytes)")
