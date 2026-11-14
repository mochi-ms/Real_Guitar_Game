"""UE 5.8 PBR Material & Texture Factory for RealGuitarGame Sanctuary."""

from __future__ import annotations

from pathlib import Path
import unreal


def ensure_dir(content_path: str) -> None:
    if not unreal.EditorAssetLibrary.does_directory_exist(content_path):
        unreal.EditorAssetLibrary.make_directory(content_path)


def load_asset_if_exists(asset_path: str):
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        return unreal.EditorAssetLibrary.load_asset(asset_path)
    return None


def import_asset(
    asset_tools: unreal.AssetTools,
    src_file: Path,
    dest_path: str,
    dest_name: str | None = None,
    replace_existing: bool = False,
) -> list[str]:
    task = unreal.AssetImportTask()
    task.filename = str(src_file)
    task.destination_path = dest_path
    if dest_name:
        task.destination_name = dest_name
    task.automated = True
    task.save = True
    task.replace_existing = replace_existing
    asset_tools.import_asset_tasks([task])
    return list(task.imported_object_paths or [])


def configure_texture_asset(tex: unreal.Texture2D, mode: str) -> None:
    """Configure normal / linear / UI / color texture compression and sRGB flags cleanly."""
    if not tex:
        return
    if mode == "normal":
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
        tex.set_editor_property("srgb", False)
    elif mode == "linear":
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
        tex.set_editor_property("srgb", False)
    elif mode == "ui":
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
        tex.set_editor_property("srgb", True)
        tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    else:
        tex.set_editor_property("srgb", True)
    unreal.EditorAssetLibrary.save_loaded_asset(tex)


def import_sanctuary_textures(asset_tools: unreal.AssetTools, content_dir: Path) -> dict[str, unreal.Texture2D]:
    """Import PBR texture maps (forest ground, oak deck, rock cliff, water normals, ember glow)."""
    ensure_dir("/Game/Materials/Textures")
    tex_root = content_dir / "Materials" / "Textures"
    imported: dict[str, unreal.Texture2D] = {}

    if not tex_root.exists():
        return imported

    for file_path in sorted(tex_root.rglob("*")):
        if not file_path.is_file() or file_path.suffix.lower() not in {".png", ".jpg", ".jpeg"}:
            continue
        rel = file_path.relative_to(tex_root)
        sub_parts = list(rel.parts[:-1])
        dest_folder = "/Game/Materials/Textures" + (
            "/" + "/".join(sub_parts) if sub_parts else ""
        )
        ensure_dir(dest_folder)
        asset_name = file_path.stem
        obj_path = f"{dest_folder}/{asset_name}.{asset_name}"

        tex = load_asset_if_exists(obj_path)
        if not tex:
            paths = import_asset(asset_tools, file_path, dest_folder, asset_name, replace_existing=False)
            if paths:
                tex = load_asset_if_exists(paths[0])
            if not tex:
                tex = load_asset_if_exists(obj_path)

        if isinstance(tex, unreal.Texture2D):
            lower_name = asset_name.lower()
            if "_nor_" in lower_name or "_n1" in lower_name or "_n2" in lower_name:
                configure_texture_asset(tex, "normal")
            elif "_rough" in lower_name or "_ao" in lower_name:
                configure_texture_asset(tex, "linear")
            else:
                configure_texture_asset(tex, "color")
            key = "/".join(sub_parts + [asset_name]) if sub_parts else asset_name
            imported[key] = tex
            imported[asset_name] = tex

    return imported


def _get_or_create_material(asset_tools: unreal.AssetTools, folder: str, name: str) -> unreal.Material:
    ensure_dir(folder)
    mat_path = f"{folder}/{name}"
    mat = load_asset_if_exists(mat_path)
    if not mat:
        mat = asset_tools.create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew())
    return mat


def _make_tex_sample(
    mat: unreal.Material,
    tex: unreal.Texture2D,
    uv_node: unreal.MaterialExpression | None,
    sampler_type: unreal.MaterialSamplerType,
    x: int,
    y: int,
) -> unreal.MaterialExpressionTextureSample:
    mat_edit = unreal.MaterialEditingLibrary
    node = mat_edit.create_material_expression(mat, unreal.MaterialExpressionTextureSample, x, y)
    node.set_editor_property("texture", tex)
    node.set_editor_property("sampler_type", sampler_type)
    if uv_node:
        mat_edit.connect_material_expressions(uv_node, "", node, "UVs")
    return node


def build_pbr_textured_material(
    asset_tools: unreal.AssetTools,
    folder: str,
    name: str,
    diff_tex: unreal.Texture2D | None,
    norm_tex: unreal.Texture2D | None,
    rough_tex: unreal.Texture2D | None,
    ao_tex: unreal.Texture2D | None = None,
    uv_tiling: float = 1.0,
    roughness_mult: float = 1.0,
    specular: float = 0.40,
    tint_color: tuple[float, float, float, float] = (1.0, 1.0, 1.0, 1.0),
) -> unreal.Material:
    """Create a rich PBR textured UE5 material with UV tiling, normal, roughness, and AO."""
    mat = _get_or_create_material(asset_tools, folder, name)
    mat_edit = unreal.MaterialEditingLibrary
    mat_edit.delete_all_material_expressions(mat)

    uv_node = mat_edit.create_material_expression(mat, unreal.MaterialExpressionTextureCoordinate, -950, 0)
    uv_node.set_editor_property("u_tiling", uv_tiling)
    uv_node.set_editor_property("v_tiling", uv_tiling)

    if diff_tex:
        diff_samp = _make_tex_sample(mat, diff_tex, uv_node, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, -620, -320)
        if tint_color != (1.0, 1.0, 1.0, 1.0):
            tint_const = mat_edit.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -620, -520)
            tint_const.set_editor_property("constant", unreal.LinearColor(*tint_color))
            mul = mat_edit.create_material_expression(mat, unreal.MaterialExpressionMultiply, -300, -360)
            mat_edit.connect_material_expressions(diff_samp, "RGB", mul, "A")
            mat_edit.connect_material_expressions(tint_const, "", mul, "B")
            mat_edit.connect_material_property(mul, "", unreal.MaterialProperty.MP_BASE_COLOR)
        else:
            mat_edit.connect_material_property(diff_samp, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    else:
        col = mat_edit.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -300, -320)
        col.set_editor_property("constant", unreal.LinearColor(*tint_color))
        mat_edit.connect_material_property(col, "", unreal.MaterialProperty.MP_BASE_COLOR)

    if norm_tex:
        norm_samp = _make_tex_sample(mat, norm_tex, uv_node, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL, -620, -40)
        mat_edit.connect_material_property(norm_samp, "RGB", unreal.MaterialProperty.MP_NORMAL)

    if rough_tex:
        rough_samp = _make_tex_sample(mat, rough_tex, uv_node, unreal.MaterialSamplerType.SAMPLERTYPE_MASKS, -620, 220)
        if abs(roughness_mult - 1.0) > 1e-3:
            mult_r = mat_edit.create_material_expression(mat, unreal.MaterialExpressionMultiply, -320, 220)
            r_scale = mat_edit.create_material_expression(mat, unreal.MaterialExpressionConstant, -560, 360)
            r_scale.set_editor_property("r", roughness_mult)
            mat_edit.connect_material_expressions(rough_samp, "R", mult_r, "A")
            mat_edit.connect_material_expressions(r_scale, "", mult_r, "B")
            mat_edit.connect_material_property(mult_r, "", unreal.MaterialProperty.MP_ROUGHNESS)
        else:
            mat_edit.connect_material_property(rough_samp, "R", unreal.MaterialProperty.MP_ROUGHNESS)
    else:
        r_const = mat_edit.create_material_expression(mat, unreal.MaterialExpressionConstant, -320, 220)
        r_const.set_editor_property("r", 0.72 * roughness_mult)
        mat_edit.connect_material_property(r_const, "", unreal.MaterialProperty.MP_ROUGHNESS)

    if ao_tex:
        ao_samp = _make_tex_sample(mat, ao_tex, uv_node, unreal.MaterialSamplerType.SAMPLERTYPE_MASKS, -620, 480)
        mat_edit.connect_material_property(ao_samp, "R", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)

    spec_const = mat_edit.create_material_expression(mat, unreal.MaterialExpressionConstant, -320, 420)
    spec_const.set_editor_property("r", specular)
    mat_edit.connect_material_property(spec_const, "", unreal.MaterialProperty.MP_SPECULAR)

    mat_edit.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(f"{folder}/{name}")
    return mat


def build_animated_lake_material(
    asset_tools: unreal.AssetTools,
    folder: str,
    name: str,
    n1_tex: unreal.Texture2D | None,
    n2_tex: unreal.Texture2D | None,
) -> unreal.Material:
    """Create mirror-reflective calm mountain lake shader with dual panning normals & Fresnel sky tint."""
    mat = _get_or_create_material(asset_tools, folder, name)
    mat_edit = unreal.MaterialEditingLibrary
    mat_edit.delete_all_material_expressions(mat)

    deep_col = mat_edit.create_material_expression(mat, unreal.MaterialExpressionConstant4Vector, -720, -320)
    deep_col.set_editor_property("constant", unreal.LinearColor(0.008, 0.028, 0.042, 1.0))

    shallow_horizon = mat_edit.create_material_expression(mat, unreal.MaterialExpressionConstant4Vector, -720, -160)
    shallow_horizon.set_editor_property("constant", unreal.LinearColor(0.14, 0.16, 0.18, 1.0))

    fresnel = mat_edit.create_material_expression(mat, unreal.MaterialExpressionFresnel, -720, -20)
    fresnel.set_editor_property("exponent", 4.0)
    fresnel.set_editor_property("base_reflect_fraction", 0.04)

    lerp_col = mat_edit.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, -400, -220)
    mat_edit.connect_material_expressions(deep_col, "", lerp_col, "A")
    mat_edit.connect_material_expressions(shallow_horizon, "", lerp_col, "B")
    mat_edit.connect_material_expressions(fresnel, "", lerp_col, "Alpha")
    mat_edit.connect_material_property(lerp_col, "", unreal.MaterialProperty.MP_BASE_COLOR)

    rough = mat_edit.create_material_expression(mat, unreal.MaterialExpressionConstant, -320, 40)
    rough.set_editor_property("r", 0.035)
    mat_edit.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)

    metal = mat_edit.create_material_expression(mat, unreal.MaterialExpressionConstant, -320, 120)
    metal.set_editor_property("r", 0.06)
    mat_edit.connect_material_property(metal, "", unreal.MaterialProperty.MP_METALLIC)

    spec = mat_edit.create_material_expression(mat, unreal.MaterialExpressionConstant, -320, 200)
    spec.set_editor_property("r", 0.90)
    mat_edit.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)

    if n1_tex and n2_tex:
        uv_coord = mat_edit.create_material_expression(mat, unreal.MaterialExpressionTextureCoordinate, -1200, 320)
        uv_coord.set_editor_property("u_tiling", 34.0)
        uv_coord.set_editor_property("v_tiling", 34.0)

        panner1 = mat_edit.create_material_expression(mat, unreal.MaterialExpressionPanner, -960, 260)
        panner1.set_editor_property("speed_x", 0.016)
        panner1.set_editor_property("speed_y", 0.010)
        mat_edit.connect_material_expressions(uv_coord, "", panner1, "Coordinate")

        panner2 = mat_edit.create_material_expression(mat, unreal.MaterialExpressionPanner, -960, 480)
        panner2.set_editor_property("speed_x", -0.012)
        panner2.set_editor_property("speed_y", 0.019)
        mat_edit.connect_material_expressions(uv_coord, "", panner2, "Coordinate")

        samp1 = _make_tex_sample(mat, n1_tex, panner1, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL, -700, 240)
        samp2 = _make_tex_sample(mat, n2_tex, panner2, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL, -700, 460)

        add_n = mat_edit.create_material_expression(mat, unreal.MaterialExpressionAdd, -420, 340)
        mat_edit.connect_material_expressions(samp1, "RGB", add_n, "A")
        mat_edit.connect_material_expressions(samp2, "RGB", add_n, "B")

        flat_up = mat_edit.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -420, 520)
        flat_up.set_editor_property("constant", unreal.LinearColor(0.0, 0.0, 1.0, 1.0))

        gentle_alpha = mat_edit.create_material_expression(mat, unreal.MaterialExpressionConstant, -420, 620)
        gentle_alpha.set_editor_property("r", 0.22)

        blend_n = mat_edit.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, -200, 380)
        mat_edit.connect_material_expressions(flat_up, "", blend_n, "A")
        mat_edit.connect_material_expressions(add_n, "", blend_n, "B")
        mat_edit.connect_material_expressions(gentle_alpha, "", blend_n, "Alpha")
        mat_edit.connect_material_property(blend_n, "", unreal.MaterialProperty.MP_NORMAL)

    mat_edit.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(f"{folder}/{name}")
    return mat


def build_ember_material(
    asset_tools: unreal.AssetTools,
    folder: str,
    name: str,
    ember_tex: unreal.Texture2D | None,
) -> unreal.Material:
    """Create glowing campfire ember material for the lakeside stone fire pit."""
    mat = _get_or_create_material(asset_tools, folder, name)
    mat_edit = unreal.MaterialEditingLibrary
    mat_edit.delete_all_material_expressions(mat)

    base = mat_edit.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -420, -180)
    base.set_editor_property("constant", unreal.LinearColor(0.08, 0.03, 0.015, 1.0))
    mat_edit.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)

    emissive_tint = mat_edit.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -620, 40)
    emissive_tint.set_editor_property("constant", unreal.LinearColor(12.0, 4.2, 0.9, 1.0))

    if ember_tex:
        samp = _make_tex_sample(mat, ember_tex, None, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, -660, 220)
        mul = mat_edit.create_material_expression(mat, unreal.MaterialExpressionMultiply, -340, 120)
        mat_edit.connect_material_expressions(emissive_tint, "", mul, "A")
        mat_edit.connect_material_expressions(samp, "RGB", mul, "B")
        mat_edit.connect_material_property(mul, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    else:
        mat_edit.connect_material_property(emissive_tint, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    mat_edit.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(f"{folder}/{name}")
    return mat
