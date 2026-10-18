"""Erzeugt die Materialien per MaterialEditingLibrary:
 - M_TileAtlas: ISM-Tiles mit UV-Rechteck + Schaden aus PerInstanceCustomData (5 Floats)
 - M_SpriteLit / M_SpriteUnlit: Paper2D-Sprite-Materialien (SpriteTexture-Parameter, Vertexfarbe, Masked)
 - M_Debris: 3D-Trümmerwürfel mit Farbparameter"""
import unreal
from be_common import *  # noqa: F401,F403

MEL = unreal.MaterialEditingLibrary


def new_material(name):
    mat = create_asset(name, MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    mat.set_editor_property("two_sided", True)
    return mat


def expr(mat, cls, x, y, **props):
    e = MEL.create_material_expression(mat, cls, x, y)
    for k, v in props.items():
        e.set_editor_property(k, v)
    return e


def make_tile_material(lit=True):
    mat = new_material("M_TileAtlas")
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT if lit else unreal.MaterialShadingModel.MSM_UNLIT)
    tex = expr(mat, unreal.MaterialExpressionTextureSampleParameter2D, -300, 0, parameter_name="Atlas", sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    uv = expr(mat, unreal.MaterialExpressionTextureCoordinate, -1100, 0)
    cd = [expr(mat, unreal.MaterialExpressionPerInstanceCustomData, -1100, 150 + i * 80, const_index=i) for i in range(5)]
    mask_u = expr(mat, unreal.MaterialExpressionComponentMask, -900, -40, r=True, g=False, b=False, a=False)
    mask_v = expr(mat, unreal.MaterialExpressionComponentMask, -900, 60, r=False, g=True, b=False, a=False)
    MEL.connect_material_expressions(uv, "", mask_u, "")
    MEL.connect_material_expressions(uv, "", mask_v, "")
    lerp_u = expr(mat, unreal.MaterialExpressionLinearInterpolate, -700, -40)
    lerp_v = expr(mat, unreal.MaterialExpressionLinearInterpolate, -700, 80)
    MEL.connect_material_expressions(cd[0], "", lerp_u, "A")
    MEL.connect_material_expressions(cd[2], "", lerp_u, "B")
    MEL.connect_material_expressions(mask_u, "", lerp_u, "Alpha")
    MEL.connect_material_expressions(cd[1], "", lerp_v, "A")
    MEL.connect_material_expressions(cd[3], "", lerp_v, "B")
    MEL.connect_material_expressions(mask_v, "", lerp_v, "Alpha")
    app = expr(mat, unreal.MaterialExpressionAppendVector, -520, 0)
    MEL.connect_material_expressions(lerp_u, "", app, "A")
    MEL.connect_material_expressions(lerp_v, "", app, "B")
    MEL.connect_material_expressions(app, "", tex, "UVs")
    # Abdunkeln nach Schaden: Farbe * (1 - Damage * 0.85)
    dmg_scale = expr(mat, unreal.MaterialExpressionMultiply, -300, 300, const_b=0.85)
    MEL.connect_material_expressions(cd[4], "", dmg_scale, "A")
    one_minus = expr(mat, unreal.MaterialExpressionOneMinus, -150, 300)
    MEL.connect_material_expressions(dmg_scale, "", one_minus, "")
    mul = expr(mat, unreal.MaterialExpressionMultiply, 0, 100)
    MEL.connect_material_expressions(tex, "RGB", mul, "A")
    MEL.connect_material_expressions(one_minus, "", mul, "B")
    MEL.connect_material_property(mul, "", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(tex, "A", unreal.MaterialProperty.MP_OPACITY_MASK)
    if lit:
        rough = expr(mat, unreal.MaterialExpressionConstant, 0, 250, r=0.9)
        MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.recompile_material(mat)
    return mat


def make_sprite_material(name, lit):
    mat = new_material(name)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT if lit else unreal.MaterialShadingModel.MSM_UNLIT)
    tex = expr(mat, unreal.MaterialExpressionTextureSampleParameter2D, -500, 0, parameter_name="SpriteTexture", sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    vc = expr(mat, unreal.MaterialExpressionVertexColor, -500, 250)
    mul = expr(mat, unreal.MaterialExpressionMultiply, -250, 50)
    MEL.connect_material_expressions(tex, "RGB", mul, "A")
    MEL.connect_material_expressions(vc, "", mul, "B")
    mul_a = expr(mat, unreal.MaterialExpressionMultiply, -250, 200)
    MEL.connect_material_expressions(tex, "A", mul_a, "A")
    MEL.connect_material_expressions(vc, "A", mul_a, "B")
    MEL.connect_material_property(mul, "", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(mul_a, "", unreal.MaterialProperty.MP_OPACITY_MASK)
    if lit:
        rough = expr(mat, unreal.MaterialExpressionConstant, -250, 320, r=0.85)
        MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
        # Optionale Normalmap (vom Pixelize-Tool erzeugt): Parameter "SpriteNormal"
        nrm = expr(mat, unreal.MaterialExpressionTextureSampleParameter2D, -500, 450, parameter_name="SpriteNormal", sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        MEL.connect_material_property(nrm, "RGB", unreal.MaterialProperty.MP_NORMAL)
    MEL.recompile_material(mat)
    return mat


def make_debris_material():
    mat = create_asset("M_Debris", MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
    col = expr(mat, unreal.MaterialExpressionVectorParameter, -300, 0, parameter_name="Color", default_value=unreal.LinearColor(1, 0.5, 0.1, 1))
    MEL.connect_material_property(col, "", unreal.MaterialProperty.MP_BASE_COLOR)
    emis = expr(mat, unreal.MaterialExpressionMultiply, -100, 200, const_b=0.35)
    MEL.connect_material_expressions(col, "", emis, "A")
    MEL.connect_material_property(emis, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    rough = expr(mat, unreal.MaterialExpressionConstant, -300, 300, r=0.7)
    MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.recompile_material(mat)
    return mat


def run(lit=True):
    mats = dict(
        tile=make_tile_material(lit),
        sprite_lit=make_sprite_material("M_SpriteLit", True),
        sprite_unlit=make_sprite_material("M_SpriteUnlit", False),
        debris=make_debris_material(),
    )
    save_all()
    log("Materialien erzeugt: " + ", ".join(m.get_name() for m in mats.values()))
    return mats


if __name__ == "__main__":
    run()
