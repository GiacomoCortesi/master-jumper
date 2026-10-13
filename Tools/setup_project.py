"""Master Jumper - one-shot Content setup. Run INSIDE the Unreal Editor (after the C++ module has compiled):

    Output Log -> switch the input box to "Python" ->  py "<ProjectDir>/Tools/setup_project.py"
    (or File -> Execute Python Script... and pick this file)

Creates, under /Game/MasterJumper:
    Textures/T_CorridorWall           (imported from SourceArt/Textures/T_CorridorWall.png)
    Materials/M_CorridorWall, M_OfficeChair, M_OfficePC, M_POV_Arms   (+ MI_* colour instances from the bake)
    Meshes/SM_OfficeChair, SM_OfficeComputer, SM_POV_Arms, SM_CorridorWallPanel  (baked by C++ UMJArtLibrary)
    Input/IA_JumpCharge
    Blueprints/BP_MJ_Player, BP_MJ_LevelController, BP_MJ_ObstacleStack
    Maps/LVL_OfficeCorridor_01        (corridor, obstacle, gate and level controller placed)

Idempotent: anything that already exists is kept. Delete Content/MasterJumper/* (keep the folders) to regenerate.
Every step is independent; the game still runs if one fails, because C++ falls back to code-built art/input.
"""
import os
import unreal

ROOT = "/Game/MasterJumper"
TEX, MAT, MESH, INP, BP, MAPS = (f"{ROOT}/{d}" for d in ("Textures", "Materials", "Meshes", "Input", "Blueprints", "Maps"))
MAP_PATH = f"{MAPS}/LVL_OfficeCorridor_01"

# Must match Source/MasterJumper/Public/MJLayout.h
OBSTACLE_CENTER_X = 2400.0 + 220.0 * 0.5
GATE_CENTER = (0.5 * (2640.0 + 3150.0), 0.0, 150.0 * 0.5)

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
results = []


def step(name):
    def wrap(fn):
        def run():
            try:
                msg = fn() or "ok"
                results.append((name, True, msg))
            except Exception as exc:  # keep going: every step has a runtime fallback in C++
                results.append((name, False, repr(exc)))
                unreal.log_error(f"[MJ setup] {name} failed: {exc!r}")
        return run
    return wrap


def project_file(*parts):
    return os.path.normpath(os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), *parts))


def exists(path):
    return eal.does_asset_exist(path)


@step("Texture T_CorridorWall")
def import_texture():
    path = f"{TEX}/T_CorridorWall"
    if exists(path):
        return "exists"
    png = project_file("SourceArt", "Textures", "T_CorridorWall.png")
    if not os.path.isfile(png):
        raise RuntimeError(f"missing {png} (run python3 Tools/gen_wall_texture.py)")
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", png)
    task.set_editor_property("destination_path", TEX)
    task.set_editor_property("destination_name", "T_CorridorWall")
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    asset_tools.import_asset_tasks([task])
    if not exists(path):
        raise RuntimeError("import produced no asset")


def new_material(name):
    return asset_tools.create_asset(name, MAT, unreal.Material, unreal.MaterialFactoryNew())


def scalar(material, prop, value, x, y):
    node = mel.create_material_expression(material, unreal.MaterialExpressionConstant, x, y)
    node.set_editor_property("r", value)
    mel.connect_material_property(node, "", prop)


@step("Material M_CorridorWall")
def make_wall_material():
    path = f"{MAT}/M_CorridorWall"
    if exists(path):
        return "exists"
    texture = eal.load_asset(f"{TEX}/T_CorridorWall")
    if texture is None:
        raise RuntimeError("T_CorridorWall missing")
    material = new_material("M_CorridorWall")
    sample = mel.create_material_expression(material, unreal.MaterialExpressionTextureSample, -500, 0)
    sample.set_editor_property("texture", texture)
    mel.connect_material_property(sample, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    scalar(material, unreal.MaterialProperty.MP_ROUGHNESS, 0.85, -300, 250)
    mel.recompile_material(material)
    eal.save_loaded_asset(material)


def make_color_material(name, color, roughness, metallic=0.0):
    path = f"{MAT}/{name}"
    if exists(path):
        return "exists"
    material = new_material(name)
    tint = mel.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -500, 0)
    tint.set_editor_property("parameter_name", "Color")
    tint.set_editor_property("default_value", unreal.LinearColor(color[0], color[1], color[2], 1.0))
    mel.connect_material_property(tint, "", unreal.MaterialProperty.MP_BASE_COLOR)
    rough = mel.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -500, 250)
    rough.set_editor_property("parameter_name", "Roughness")
    rough.set_editor_property("default_value", roughness)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    if metallic:
        scalar(material, unreal.MaterialProperty.MP_METALLIC, metallic, -300, 400)
    mel.recompile_material(material)
    eal.save_loaded_asset(material)


@step("Material M_OfficeChair")
def make_chair_material():
    return make_color_material("M_OfficeChair", (0.02, 0.026, 0.055), 0.75)


@step("Material M_OfficePC")
def make_pc_material():
    return make_color_material("M_OfficePC", (0.5, 0.44, 0.33), 0.45)


@step("Material M_POV_Arms")
def make_arms_material():
    return make_color_material("M_POV_Arms", (0.7, 0.42, 0.3), 0.65)


@step("Bake meshes SM_* (C++ UMJArtLibrary)")
def bake_meshes():
    if not unreal.MJArtLibrary.bake_art_assets(MESH, MAT):
        raise RuntimeError("bake reported failure (see Output Log, LogMasterJumper)")
    eal.save_directory(MESH, only_if_is_dirty=False, recursive=True)
    eal.save_directory(MAT, only_if_is_dirty=False, recursive=True)


@step("Input IA_JumpCharge")
def make_input_action():
    path = f"{INP}/IA_JumpCharge"
    if exists(path):
        return "exists"
    action = asset_tools.create_asset("IA_JumpCharge", INP, unreal.InputAction, unreal.InputAction_Factory())
    action.set_editor_property("value_type", unreal.InputActionValueType.BOOLEAN)
    eal.save_loaded_asset(action)
    return "created (keys Space / LMB / Gamepad A are mapped at runtime by AMJPlayer)"


def make_blueprint(name, parent):
    path = f"{BP}/{name}"
    if exists(path):
        return "exists"
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", parent)
    blueprint = asset_tools.create_asset(name, BP, None, factory)
    if blueprint is None:
        raise RuntimeError("create_asset returned None")
    compiler = getattr(unreal, "BlueprintEditorLibrary", None)
    if compiler:
        compiler.compile_blueprint(blueprint)
    eal.save_loaded_asset(blueprint)


@step("Blueprint BP_MJ_Player")
def bp_player():
    return make_blueprint("BP_MJ_Player", unreal.MJPlayer.static_class())


@step("Blueprint BP_MJ_LevelController")
def bp_controller():
    return make_blueprint("BP_MJ_LevelController", unreal.MJLevelController.static_class())


@step("Blueprint BP_MJ_ObstacleStack")
def bp_obstacle():
    return make_blueprint("BP_MJ_ObstacleStack", unreal.MJObstacleStack.static_class())


def bp_class(name, fallback):
    path = f"{BP}/{name}"
    if exists(path):
        cls = eal.load_blueprint_class(path)
        if cls:
            return cls
    return fallback


@step("Map LVL_OfficeCorridor_01")
def make_map():
    if exists(MAP_PATH):
        return "exists"
    level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if not level_editor.new_level(MAP_PATH):
        raise RuntimeError("new_level failed")
    rot = unreal.Rotator(0.0, 0.0, 0.0)
    actors.spawn_actor_from_class(unreal.MJCorridor.static_class(), unreal.Vector(0.0, 0.0, 0.0), rot)
    actors.spawn_actor_from_class(bp_class("BP_MJ_ObstacleStack", unreal.MJObstacleStack.static_class()), unreal.Vector(OBSTACLE_CENTER_X, 0.0, 0.0), rot)
    actors.spawn_actor_from_class(unreal.MJLevelGate.static_class(), unreal.Vector(*GATE_CENTER), rot)
    actors.spawn_actor_from_class(bp_class("BP_MJ_LevelController", unreal.MJLevelController.static_class()), unreal.Vector(0.0, 0.0, 0.0), rot)
    if not level_editor.save_current_level():
        raise RuntimeError("save_current_level failed")


def main():
    for d in (TEX, MAT, MESH, INP, BP, MAPS):
        eal.make_directory(d)
    for fn in (import_texture, make_wall_material, make_chair_material, make_pc_material, make_arms_material,
               bake_meshes, make_input_action, bp_player, bp_controller, bp_obstacle, make_map):
        fn()
    unreal.log("[MJ setup] ---------------- summary ----------------")
    for name, ok, msg in results:
        (unreal.log if ok else unreal.log_error)(f"[MJ setup] {'OK  ' if ok else 'FAIL'} {name}: {msg}")
    failed = [r for r in results if not r[1]]
    unreal.log(f"[MJ setup] done: {len(results) - len(failed)}/{len(results)} steps ok. Press Play (PIE) in LVL_OfficeCorridor_01.")


main()
