"""Erzeugt Level-DataAssets (BP_LevelData) aus den ASCII-Levels des Prototyps und die Map L_Kreuzberg01
mit Licht, BP_TileWorld und BP_Camera (sofern die Blueprints schon existieren, sonst nur Licht + Hinweis)."""
import os
import unreal
from be_common import *  # noqa: F401,F403

LEVELS = {"Kreuzberg01": os.path.join(PROJECT_DIR, "Prototype", "Web", "src", "data", "levels", "kreuzberg01.ts")}


def make_level_assets():
    out = {}
    cls = bp_class("BP_LevelData")
    for key, ts in LEVELS.items():
        name, rows = read_level_rows_from_ts(ts)
        if cls is None:
            warn(f"BP_LevelData fehlt – Level {key} ({len(rows)} Zeilen) nicht als DataAsset angelegt.")
            out[key] = None
            continue
        da = make_data_asset(f"DA_Level_{key}", cls)
        set_props(da, DisplayName=unreal.Text(name), Rows=rows)
        out[key] = da
        log(f"Level {key}: {len(rows)} Zeilen x {max(len(r) for r in rows)} Spalten")
    return out


def make_map(level_asset):
    ensure_dir(MAP_DIR)
    path = f"{MAP_DIR}/L_Kreuzberg01"
    if EAL.does_asset_exist(path):
        log(f"Map existiert bereits, wird nicht überschrieben: {path}")
        return path
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    les.new_level(path)
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    # Nachtlicht: schwaches, kühles Key-Light + Skylight; Explosionen liefern die warmen Akzente
    sun = eas.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 500), unreal.Rotator(-35, 60, 0))
    sun.set_actor_label("BE_MoonLight")
    comp = sun.get_component_by_class(unreal.DirectionalLightComponent)
    comp.set_intensity(2.0)
    comp.set_light_color(unreal.LinearColor(0.62, 0.7, 1.0, 1.0))
    comp.set_cast_shadows(False)
    sky = eas.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 500))
    sky.set_actor_label("BE_SkyLight")
    skyc = sky.get_component_by_class(unreal.SkyLightComponent)
    skyc.set_intensity(0.8)
    skyc.set_light_color(unreal.LinearColor(0.35, 0.4, 0.7, 1.0))
    world_cls, cam_cls = bp_class("BP_TileWorld"), bp_class("BP_Camera")
    if world_cls is not None:
        w = eas.spawn_actor_from_class(world_cls, unreal.Vector(0, 0, 0))
        w.set_actor_label("BE_TileWorld")
        if level_asset is not None:
            set_props(w, Level=level_asset)
    else:
        warn("BP_TileWorld fehlt – bitte später in der Map platzieren.")
    if cam_cls is not None:
        cam = eas.spawn_actor_from_class(cam_cls, unreal.Vector(0, -1000, 0))
        cam.set_actor_label("BE_Camera")
    les.save_current_level()
    log(f"Map gespeichert: {path}")
    return path


def run(assets=None, mats=None):
    level_assets = make_level_assets()
    save_all()
    make_map(level_assets.get("Kreuzberg01"))


if __name__ == "__main__":
    run()
