"""Download curated CC0 photorealistic 3D environment & seating assets from Poly Haven's public API."""

from __future__ import annotations

from concurrent.futures import ThreadPoolExecutor, as_completed
import hashlib
import json
from pathlib import Path
from typing import Any
import urllib.request

REPO_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_EXTERNAL_DIR = REPO_ROOT / "ue5" / "RealGuitarGame" / "Content" / "ExternalAssets" / "PolyHaven"
USER_AGENT = "RealGuitarGame/0.2 (UE5 Acoustic Sanctuary Asset Fetcher)"

CURATED_GLTF_ASSETS: tuple[str, ...] = (
    "painted_wooden_bench",
    "WoodenChair_01",
    "wooden_lantern_01",
    "round_wooden_table_01",
    "modular_wooden_pier",
    "pine_sapling_small",
    "dead_tree_trunk",
    "tree_stump_01",
    "fern_02",
    "shrub_02",
    "rock_moss_set_01",
    "shrub_01",
    "stone_fire_pit",
)


def _fetch_bytes(url: str, timeout: float = 30.0) -> bytes:
    req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
    with urllib.request.urlopen(req, timeout=timeout) as resp:
        return resp.read()


def _verify_md5(path: Path, expected_md5: str | None) -> bool:
    if not path.exists():
        return False
    if not expected_md5:
        return path.stat().st_size > 0
    return hashlib.md5(path.read_bytes()).hexdigest().lower() == expected_md5.lower()


def extract_gltf_download_plan(
    asset_id: str,
    files_payload: dict[str, Any],
    preferred_resolutions: tuple[str, ...] = ("2k", "1k", "4k"),
) -> list[tuple[str, str, str | None]]:
    """Build list of (relative_output_path, url, md5) from Poly Haven /files/<id> JSON."""
    gltf_block = files_payload.get("gltf")
    if not isinstance(gltf_block, dict):
        raise ValueError(f"Asset '{asset_id}' has no glTF bundle available.")

    chosen_entry: dict[str, Any] | None = None
    for res in preferred_resolutions:
        res_block = gltf_block.get(res)
        if isinstance(res_block, dict) and isinstance(res_block.get("gltf"), dict):
            chosen_entry = res_block["gltf"]
            break

    if chosen_entry is None:
        raise ValueError(f"Asset '{asset_id}' missing preferred glTF resolutions {preferred_resolutions}.")

    main_url = str(chosen_entry["url"])
    main_filename = Path(main_url.split("?", 1)[0]).name or f"{asset_id}.gltf"
    plan: list[tuple[str, str, str | None]] = [
        (main_filename, main_url, chosen_entry.get("md5")),
    ]

    includes = chosen_entry.get("include", {})
    if isinstance(includes, dict):
        for rel_path, meta in includes.items():
            if isinstance(meta, dict) and meta.get("url"):
                plan.append((str(rel_path).replace("\\", "/"), str(meta["url"]), meta.get("md5")))
    return plan


def download_asset(
    asset_id: str,
    dest_root: Path = DEFAULT_EXTERNAL_DIR,
    preferred_resolutions: tuple[str, ...] = ("2k", "1k", "4k"),
) -> Path:
    """Download one Poly Haven glTF bundle + dependencies into dest_root/<asset_id>/."""
    files_url = f"https://api.polyhaven.com/files/{asset_id}"
    payload = json.loads(_fetch_bytes(files_url).decode("utf-8"))
    plan = extract_gltf_download_plan(asset_id, payload, preferred_resolutions)

    asset_dir = dest_root / asset_id
    asset_dir.mkdir(parents=True, exist_ok=True)
    main_gltf_path = asset_dir / plan[0][0]

    for rel_path, url, md5 in plan:
        out_file = asset_dir / rel_path
        out_file.parent.mkdir(parents=True, exist_ok=True)
        if _verify_md5(out_file, md5):
            continue
        out_file.write_bytes(_fetch_bytes(url, timeout=60.0))

    return main_gltf_path


def download_all_curated_assets(
    dest_root: Path = DEFAULT_EXTERNAL_DIR,
    asset_ids: tuple[str, ...] = CURATED_GLTF_ASSETS,
    max_workers: int = 4,
) -> dict[str, Path]:
    """Download all curated Poly Haven CC0 glTF assets concurrently."""
    dest_root.mkdir(parents=True, exist_ok=True)
    results: dict[str, Path] = {}
    with ThreadPoolExecutor(max_workers=max_workers) as pool:
        future_map = {pool.submit(download_asset, aid, dest_root): aid for aid in asset_ids}
        for fut in as_completed(future_map):
            aid = future_map[fut]
            gltf_path = fut.result()
            results[aid] = gltf_path
            print(f"[OK] CC0 Asset '{aid}' -> {gltf_path.relative_to(REPO_ROOT)}")
    return results


if __name__ == "__main__":
    download_all_curated_assets()
