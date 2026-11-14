"""Download additional free CC0 3D foliage/cliff/prop models and 2K PBR textures from Poly Haven."""

from __future__ import annotations

from concurrent.futures import ThreadPoolExecutor, as_completed
import hashlib
import json
from pathlib import Path
from typing import Any
import urllib.request

from tools.download_cc0_assets import (
    DEFAULT_EXTERNAL_DIR,
    REPO_ROOT,
    USER_AGENT,
    download_asset,
)

DEFAULT_TEXTURES_DIR = (
    REPO_ROOT / "ue5" / "RealGuitarGame" / "Content" / "Materials" / "Textures"
)

EXTRA_SANCTUARY_GLTF_ASSETS: tuple[str, ...] = (
    "pine_tree_01",
    "fir_tree_01",
    "fir_sapling_medium",
    "coastal_cliff_01",
    "namaqualand_boulders_01",
    "grass_medium_01",
    "flower_ursinia",
    "moss_01",
    "street_lamp_01",
    "Lantern_01",
    "folding_wooden_stool",
    "wooden_barrels_01",
    "mountainside",
)

CURATED_PBR_TEXTURES: tuple[str, ...] = (
    "forest_ground_04",
    "wood_floor_deck",
    "rock_face_03",
)

_TEXTURE_CHANNEL_PREFS: tuple[tuple[str, tuple[str, ...], str], ...] = (
    ("Diffuse", ("jpg", "png"), "diff"),
    ("nor_dx", ("png", "jpg"), "nor_dx"),
    ("Rough", ("png", "jpg"), "rough"),
    ("AO", ("jpg", "png"), "ao"),
)


def extract_texture_download_plan(
    asset_id: str,
    files_payload: dict[str, Any],
    preferred_resolutions: tuple[str, ...] = ("2k", "1k"),
) -> list[tuple[str, str, str | None]]:
    """Build (relative_filename, url, md5) tuples for PBR Diffuse/Normal/Roughness/AO channels."""
    plan: list[tuple[str, str, str | None]] = []
    for channel, fmt_order, short_tag in _TEXTURE_CHANNEL_PREFS:
        channel_block = files_payload.get(channel)
        if not isinstance(channel_block, dict):
            continue
        selected_res: dict[str, Any] | None = None
        res_label = "2k"
        for res in preferred_resolutions:
            candidate = channel_block.get(res)
            if isinstance(candidate, dict):
                selected_res = candidate
                res_label = res
                break
        if selected_res is None:
            continue
        for fmt in fmt_order:
            entry = selected_res.get(fmt)
            if isinstance(entry, dict) and entry.get("url"):
                rel_name = f"{asset_id}_{short_tag}_{res_label}.{fmt}"
                plan.append((rel_name, str(entry["url"]), entry.get("md5")))
                break
    return plan


def _download_checked_file(url: str, dest: Path, expected_md5: str | None) -> Path:
    dest.parent.mkdir(parents=True, exist_ok=True)
    if dest.exists() and expected_md5:
        digest = hashlib.md5(dest.read_bytes()).hexdigest()
        if digest.lower() == expected_md5.lower():
            return dest
    req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
    with urllib.request.urlopen(req, timeout=60) as resp:
        data = resp.read()
    dest.write_bytes(data)
    return dest


def download_pbr_texture(
    asset_id: str,
    dest_root: Path = DEFAULT_TEXTURES_DIR,
    preferred_resolutions: tuple[str, ...] = ("2k", "1k"),
) -> Path:
    """Download PBR channel maps for a Poly Haven texture asset."""
    api_url = f"https://api.polyhaven.com/files/{asset_id}"
    req = urllib.request.Request(api_url, headers={"User-Agent": USER_AGENT})
    with urllib.request.urlopen(req, timeout=30) as resp:
        payload = json.loads(resp.read().decode("utf-8"))

    plan = extract_texture_download_plan(asset_id, payload, preferred_resolutions)
    if not plan:
        raise RuntimeError(f"No downloadable PBR maps found for Poly Haven texture '{asset_id}'")

    asset_dir = dest_root / asset_id
    for rel_name, url, md5 in plan:
        _download_checked_file(url, asset_dir / rel_name, md5)
    return asset_dir


def download_sanctuary_pack(
    models_root: Path = DEFAULT_EXTERNAL_DIR,
    textures_root: Path = DEFAULT_TEXTURES_DIR,
    max_workers: int = 4,
) -> tuple[list[Path], list[Path]]:
    """Fetch all extended sanctuary glTF models and 2K PBR surface textures concurrently."""
    model_dirs: list[Path] = []
    texture_dirs: list[Path] = []
    with ThreadPoolExecutor(max_workers=max_workers) as pool:
        model_futs = {
            pool.submit(download_asset, asset_id, models_root): asset_id
            for asset_id in EXTRA_SANCTUARY_GLTF_ASSETS
        }
        tex_futs = {
            pool.submit(download_pbr_texture, tex_id, textures_root): tex_id
            for tex_id in CURATED_PBR_TEXTURES
        }
        for fut in as_completed(model_futs):
            aid = model_futs[fut]
            out_dir = fut.result()
            model_dirs.append(out_dir)
            print(f"[OK] Model   -> {aid} ({out_dir})")
        for fut in as_completed(tex_futs):
            tid = tex_futs[fut]
            out_dir = fut.result()
            texture_dirs.append(out_dir)
            print(f"[OK] Texture -> {tid} ({out_dir})")
    return model_dirs, texture_dirs


if __name__ == "__main__":
    download_sanctuary_pack()
