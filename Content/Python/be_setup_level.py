"""Erzeugt Level-Asset(s) aus den ASCII-Levels des Prototyps, die Map L_Kreuzberg01 mit Welt, Kamera,
Licht und Parallax-Ebenen, und trägt alle Assets in die Projekteinstellungen (DefaultGame.ini + CDO) ein."""
import os
import unreal
from be_common import *  # noqa: F401,F403

LEVELS = {"Kreuzberg01": os.path.join(PROJECT_DIR, "Prototype", "Web", "src", "data", "levels", "kreuzberg01.ts")}


def make_level_assets():
    out = {}
    for key, ts in LEVELS.items():
        name, rows = read_level_rows_from_ts(ts)
        da = make_data_asset(f"DA_Level_{key}", unreal.BELevelAsset)
        da.set_editor_property("display_name", unreal.Text(name))
        da.set_editor_property("rows", rows)
        out[key] = da
        log(f"Level {key}: {len(rows)} Zeilen x {max(len(r) for r in rows)} Spalten")
    return out


def write_settings(assets, mats, level_assets):
    """Soft-Referenzen in die Settings schreiben (für die laufende Sitzung + DefaultGame.ini)."""
    cdo = unreal.get_default_object(unreal.BEGameSettings)
    chars = assets["characters"]
    players = [chars[c] for c in ("willi", "doro", "berti", "sven") if c in chars]
    values = {
        "StartLevel": level_assets["Kreuzberg01"],
        "PlayerCharacters": players,
        "MookData": chars.get("mook"),
        "SecurityData": chars.get("security"),
        "CaptiveData": chars.get("captive"),
        "WeaponLibrary": assets["weapon_library"],
        "SpriteLibrary": assets["sprite_library"],
        "TileSet": assets["tileset"],
        "TileMaterial": mats["tile"],
        "DebrisMaterial": mats["debris"],
    }
    # Laufende Sitzung
    cdo.set_editor_property("start_level", values["StartLevel"])
    cdo.set_editor_property("player_characters", players)
    cdo.set_editor_property("mook_data", values["MookData"])
    cdo.set_editor_property("security_data", values["SecurityData"])
    cdo.set_editor_property("captive_data", values["CaptiveData"])
    cdo.set_editor_property("weapon_library", values["WeaponLibrary"])
    cdo.set_editor_property("sprite_library", values["SpriteLibrary"])
    cdo.set_editor_property("tile_set", values["TileSet"])
    cdo.set_editor_property("tile_material", values["TileMaterial"])
    cdo.set_editor_property("debris_material", values["DebrisMaterial"])
    # Persistenz: DefaultGame.ini ergänzen
    ini = os.path.join(PROJECT_DIR, "Config", "DefaultGame.ini")
    with open(ini, "r", encoding="utf-8") as f:
        lines = f.read().splitlines()
    section = "[/Script/BerlinExpendables.BEGameSettings]"
    keys = set(values.keys())
    out, in_section = [], False
    for ln in lines:
        if ln.strip().startswith("["):
            if in_section:
                out.extend(_setting_lines(values)); in_section = False
            in_section = ln.strip() == section
            out.append(ln); continue
        if in_section and (ln.split("=")[0].strip().lstrip("+") in keys):
            continue
        out.append(ln)
    if in_section:
        out.extend(_setting_lines(values))
    if section not in lines:
        out += ["", section] + _setting_lines(values)
    with open(ini, "w", encoding="utf-8") as f:
        f.write("\n".join(out) + "\n")
    log(f"Settings geschrieben: {ini}")


def _path(a):
    return a.get_path_name() if a is not None else ""


def _setting_lines(values):
    lines = []
    for k, v in values.items():
        if isinstance(v, list):
            lines.append(f"!{k}=ClearArray")
            lines += [f"+{k}={_path(a)}" for a in v]
        else:
            lines.append(f"{k}={_path(v)}")
    return lines


def make_map(level_asset):
    ensure_dir(MAP_DIR)
    path = f"{MAP_DIR}/L_Kreuzberg01"
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    les.new_level(path)
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    world = eas.spawn_actor_from_class(unreal.BETileWorld, unreal.Vector(0, 0, 0))
    world.set_editor_property("level", level_asset)
    world.set_actor_label("BE_TileWorld")
    cam = eas.spawn_actor_from_class(unreal.BECameraActor, unreal.Vector(0, -1000, 0))
    cam.set_actor_label("BE_Camera")
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
    les.save_current_level()
    log(f"Map gespeichert: {path}")
    return path


def run(assets, mats):
    level_assets = make_level_assets()
    write_settings(assets, mats, level_assets)
    save_all()
    make_map(level_assets["Kreuzberg01"])


if __name__ == "__main__":
    raise SystemExit("Bitte be_build_all.py ausführen (benötigt Import- und Materialergebnisse).")
