"""
build_muzzleflash_vfx.py
------------------------------------------------------------------------------
BattlegroundClone - 머즐플래시 VFX 셋업 스크립트 (UE 5.8, Unreal Editor Python)

무엇을 만드는가
  /Game/VFX/MuzzleFlash/
    M_MuzzleFlash_Core   : Additive/Unlit, T_Explosion_SubUV (6x6) 섬광 플룸
    M_MuzzleFlash_Spark  : Additive/Unlit, T_Spark_Core 스파크
    M_MuzzleFlash_Smoke  : Translucent/Unlit, T_Smoke_SubUV (8x8) 잔연
    NS_MuzzleFlash        : 빈 Niagara System (에미터는 아래 레시피대로 손으로 구성)

전제
  - Content/StarterContent/Textures 에 T_Explosion_SubUV / T_Spark_Core / T_Smoke_SubUV 존재
    (이미 복사됨. 에디터에서 안 보이면 Content 우클릭 > Fix Up Redirectors / 재시작)

실행
  Unreal Editor > Tools > Execute Python Script... 로 이 파일 선택
  또는 콘솔:  py "C:/Users/user/Documents/Unreal Projects/BattlegroundClone/Tools/VFX/build_muzzleflash_vfx.py"
------------------------------------------------------------------------------
"""

import unreal

MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
ATH = unreal.AssetToolsHelpers.get_asset_tools()

DEST = "/Game/VFX/MuzzleFlash"
SC_TEX = "/Game/StarterContent/Textures"

TEX = {
    "core":  SC_TEX + "/T_Explosion_SubUV",   # 6x6 = 36 subimages
    "spark": SC_TEX + "/T_Spark_Core",        # 단일 스프라이트
    "smoke": SC_TEX + "/T_Smoke_SubUV",       # 8x8 = 64 subimages
}

BLEND_ADDITIVE    = unreal.BlendMode.BLEND_ADDITIVE
BLEND_TRANSLUCENT = unreal.BlendMode.BLEND_TRANSLUCENT
MSM_UNLIT         = unreal.MaterialShadingModel.MSM_UNLIT
MP_EMISSIVE       = unreal.MaterialProperty.MP_EMISSIVE_COLOR
MP_OPACITY        = unreal.MaterialProperty.MP_OPACITY


def log(msg):
    unreal.log("[MuzzleFlashVFX] " + msg)


def ensure_dir(path):
    if not EAL.does_directory_exist(path):
        EAL.make_directory(path)


def load_tex(path):
    tex = EAL.load_asset(path)
    if tex is None:
        unreal.log_error("[MuzzleFlashVFX] 텍스처 없음: %s  (StarterContent 복사/리다이렉터 확인)" % path)
    return tex


def fresh_asset(name, factory, asset_class):
    full = "%s/%s" % (DEST, name)
    if EAL.does_asset_exist(full):
        EAL.delete_asset(full)
    return ATH.create_asset(name, DEST, asset_class, factory)


def new_expr(mat, cls, x, y):
    return MEL.create_material_expression(mat, cls, x, y)


def make_material(name, blend, build_fn):
    mat = fresh_asset(name, unreal.MaterialFactoryNew(), unreal.Material)
    mat.set_editor_property("blend_mode", blend)
    mat.set_editor_property("shading_model", MSM_UNLIT)
    mat.set_editor_property("two_sided", True)
    # 파티클/Niagara 스프라이트에서 쓰이도록 usage 플래그
    for flag in ("used_with_niagara_sprites", "used_with_particle_sprites",
                 "used_with_niagara_ribbons"):
        try:
            mat.set_editor_property(flag, True)
        except Exception:
            pass
    build_fn(mat)
    MEL.recompile_material(mat)
    EAL.save_asset("%s/%s" % (DEST, name))
    log("material OK: %s/%s" % (DEST, name))
    return mat


# ---------------------------------------------------------------- M_MuzzleFlash_Core
def build_core(mat):
    sub = new_expr(mat, unreal.MaterialExpressionTextureSampleParameterSubUV, -760, -40)
    sub.set_editor_property("parameter_name", "SubUV")
    sub.set_editor_property("texture", load_tex(TEX["core"]))

    pcol = new_expr(mat, unreal.MaterialExpressionParticleColor, -760, 220)

    inten = new_expr(mat, unreal.MaterialExpressionScalarParameter, -760, 420)
    inten.set_editor_property("parameter_name", "EmissiveBoost")
    inten.set_editor_property("default_value", 45.0)

    rgb = new_expr(mat, unreal.MaterialExpressionMultiply, -470, 0)
    MEL.connect_material_expressions(sub, "RGB", rgb, "A")
    MEL.connect_material_expressions(pcol, "RGB", rgb, "B")

    emissive = new_expr(mat, unreal.MaterialExpressionMultiply, -240, 0)
    MEL.connect_material_expressions(rgb, "", emissive, "A")
    MEL.connect_material_expressions(inten, "", emissive, "B")
    MEL.connect_material_property(emissive, "", MP_EMISSIVE)

    opac = new_expr(mat, unreal.MaterialExpressionMultiply, -470, 300)
    MEL.connect_material_expressions(sub, "A", opac, "A")
    MEL.connect_material_expressions(pcol, "A", opac, "B")
    MEL.connect_material_property(opac, "", MP_OPACITY)


# ---------------------------------------------------------------- M_MuzzleFlash_Spark
def build_spark(mat):
    tex = new_expr(mat, unreal.MaterialExpressionTextureSampleParameter2D, -760, -40)
    tex.set_editor_property("parameter_name", "Spark")
    tex.set_editor_property("texture", load_tex(TEX["spark"]))

    pcol = new_expr(mat, unreal.MaterialExpressionParticleColor, -760, 220)

    inten = new_expr(mat, unreal.MaterialExpressionScalarParameter, -760, 420)
    inten.set_editor_property("parameter_name", "EmissiveBoost")
    inten.set_editor_property("default_value", 60.0)

    rgb = new_expr(mat, unreal.MaterialExpressionMultiply, -470, 0)
    MEL.connect_material_expressions(tex, "RGB", rgb, "A")
    MEL.connect_material_expressions(pcol, "RGB", rgb, "B")

    emissive = new_expr(mat, unreal.MaterialExpressionMultiply, -240, 0)
    MEL.connect_material_expressions(rgb, "", emissive, "A")
    MEL.connect_material_expressions(inten, "", emissive, "B")
    MEL.connect_material_property(emissive, "", MP_EMISSIVE)

    opac = new_expr(mat, unreal.MaterialExpressionMultiply, -470, 300)
    MEL.connect_material_expressions(tex, "R", opac, "A")
    MEL.connect_material_expressions(pcol, "A", opac, "B")
    MEL.connect_material_property(opac, "", MP_OPACITY)


# ---------------------------------------------------------------- M_MuzzleFlash_Smoke
def build_smoke(mat):
    sub = new_expr(mat, unreal.MaterialExpressionTextureSampleParameterSubUV, -760, -40)
    sub.set_editor_property("parameter_name", "SubUV")
    sub.set_editor_property("texture", load_tex(TEX["smoke"]))

    pcol = new_expr(mat, unreal.MaterialExpressionParticleColor, -760, 220)

    tint = new_expr(mat, unreal.MaterialExpressionVectorParameter, -760, 420)
    tint.set_editor_property("parameter_name", "SmokeTint")
    tint.set_editor_property("default_value", unreal.LinearColor(0.05, 0.05, 0.06, 1.0))

    rgb = new_expr(mat, unreal.MaterialExpressionMultiply, -470, 0)
    MEL.connect_material_expressions(sub, "RGB", rgb, "A")
    MEL.connect_material_expressions(tint, "RGB", rgb, "B")

    emissive = new_expr(mat, unreal.MaterialExpressionMultiply, -240, 0)
    MEL.connect_material_expressions(rgb, "", emissive, "A")
    MEL.connect_material_expressions(pcol, "RGB", emissive, "B")
    MEL.connect_material_property(emissive, "", MP_EMISSIVE)

    opac = new_expr(mat, unreal.MaterialExpressionMultiply, -470, 300)
    MEL.connect_material_expressions(sub, "A", opac, "A")
    MEL.connect_material_expressions(pcol, "A", opac, "B")
    MEL.connect_material_property(opac, "", MP_OPACITY)


# ---------------------------------------------------------------- Niagara System (빈 껍데기)
def make_niagara_system(name):
    full = "%s/%s" % (DEST, name)
    if EAL.does_asset_exist(full):
        log("니아가라 시스템 이미 존재, 건너뜀: %s" % full)
        return
    try:
        fac = unreal.NiagaraSystemFactoryNew()
        ATH.create_asset(name, DEST, unreal.NiagaraSystem, fac)
        EAL.save_asset(full)
        log("빈 Niagara System 생성: %s  (에미터는 레시피대로 구성)" % full)
    except Exception as e:
        unreal.log_warning("[MuzzleFlashVFX] Niagara System 자동 생성 실패(%s). "
                           "에디터에서 수동 생성하세요." % e)


def main():
    ensure_dir(DEST)
    make_material("M_MuzzleFlash_Core",  BLEND_ADDITIVE,    build_core)
    make_material("M_MuzzleFlash_Spark", BLEND_ADDITIVE,    build_spark)
    make_material("M_MuzzleFlash_Smoke", BLEND_TRANSLUCENT, build_smoke)
    make_niagara_system("NS_MuzzleFlash")
    log("완료. 다음 단계: NS_MuzzleFlash 에미터 구성 -> BP_WeaponBase 의 MuzzleFlashFX 에 지정.")


if __name__ == "__main__":
    main()
