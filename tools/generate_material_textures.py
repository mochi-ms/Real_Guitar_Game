"""Pure-Python seamless tileable water normal maps and warm campfire ember glow PNGs."""

from __future__ import annotations

import math
from pathlib import Path

from tools.generate_ui_textures import REPO_ROOT, render_png

DEFAULT_MATERIAL_TEX_DIR = (
    REPO_ROOT / "ue5" / "RealGuitarGame" / "Content" / "Materials" / "Textures"
)

# Multi-octave periodic harmonic wave tuples: (freq_u, freq_v, amplitude, phase)
_WAVE_SET_1: tuple[tuple[int, int, float, float], ...] = (
    (2, 3, 0.36, 0.2),
    (-3, 2, 0.30, 1.4),
    (5, -4, 0.18, 2.1),
    (-7, -5, 0.10, 0.7),
    (9, 6, 0.06, 2.9),
)

_WAVE_SET_2: tuple[tuple[int, int, float, float], ...] = (
    (3, -2, 0.34, 0.9),
    (2, 4, 0.28, 2.4),
    (-6, 3, 0.19, 1.1),
    (8, 7, 0.11, 0.3),
    (-11, 5, 0.07, 1.8),
)


def _clamp_byte(v: float) -> int:
    return max(0, min(255, int(round(v))))


def _eval_periodic_normal(
    u: float,
    v: float,
    waves: tuple[tuple[int, int, float, float], ...],
    slope_scale: float = 0.085,
) -> tuple[int, int, int, int]:
    """Compute analytic tangent-space normal vector from periodic wave sum (DX/UE compatible)."""
    dh_du = 0.0
    dh_dv = 0.0
    two_pi = 2.0 * math.pi
    for fu, fv, amp, phase in waves:
        arg = two_pi * (fu * u + fv * v) + phase
        cos_a = math.cos(arg)
        dh_du += amp * fu * two_pi * cos_a
        dh_dv += amp * fv * two_pi * cos_a

    nx = -dh_du * slope_scale
    ny = -dh_dv * slope_scale
    nz = 1.0
    inv_len = 1.0 / math.sqrt(nx * nx + ny * ny + nz * nz)
    nx *= inv_len
    ny *= inv_len
    nz *= inv_len

    return (
        _clamp_byte((nx * 0.5 + 0.5) * 255.0),
        _clamp_byte((ny * 0.5 + 0.5) * 255.0),
        _clamp_byte((nz * 0.5 + 0.5) * 255.0),
        255,
    )


def _eval_ember_glow(x: float, y: float, size: float = 128.0) -> tuple[int, int, int, int]:
    dx = (x - size * 0.5) / (size * 0.45)
    dy = (y - size * 0.5) / (size * 0.45)
    r = min(1.0, math.hypot(dx, dy))
    core = max(0.0, 1.0 - r) ** 2.4
    halo = max(0.0, 1.0 - r * r) ** 1.8
    intensity = min(1.0, 0.65 * core + 0.45 * halo)
    red = _clamp_byte(255.0 * min(1.0, 0.95 + 0.05 * core))
    green = _clamp_byte(255.0 * (0.38 * halo + 0.55 * core))
    blue = _clamp_byte(255.0 * (0.06 * halo + 0.25 * (core ** 2)))
    alpha = _clamp_byte(255.0 * intensity)
    return (red, green, blue, alpha)


def generate_sanctuary_material_textures(
    out_dir: Path = DEFAULT_MATERIAL_TEX_DIR,
) -> list[Path]:
    """Write seamless water normal maps and campfire glow texture to disk."""
    out_dir.mkdir(parents=True, exist_ok=True)
    n_size = 256
    inv = 1.0 / n_size

    p1 = render_png(
        out_dir / "T_Water_N1.png",
        n_size,
        n_size,
        lambda x, y: _eval_periodic_normal(x * inv, y * inv, _WAVE_SET_1),
    )
    p2 = render_png(
        out_dir / "T_Water_N2.png",
        n_size,
        n_size,
        lambda x, y: _eval_periodic_normal(x * inv, y * inv, _WAVE_SET_2),
    )
    p3 = render_png(
        out_dir / "T_EmberGlow.png",
        128,
        128,
        lambda x, y: _eval_ember_glow(x, y, 128.0),
    )
    return [p1, p2, p3]


if __name__ == "__main__":
    for path in generate_sanctuary_material_textures():
        print(f"[OK] Material texture -> {path.relative_to(REPO_ROOT)}")
