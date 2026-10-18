"""Gemeinsame Helfer für die Berlin-Expendables-Editor-Skripte (im Unreal-Editor ausführen)."""
import json
import os
import re
import unreal

PROJECT_DIR = unreal.Paths.project_dir()
IMPORT_DIR = os.environ.get("BE_IMPORT_DIR", os.path.join(PROJECT_DIR, "Content", "Import", "Placeholder"))
GEN_ROOT = "/Game/BE"
TEX_DIR = f"{GEN_ROOT}/Textures"
SPRITE_DIR = f"{GEN_ROOT}/Sprites"
FLIPBOOK_DIR = f"{GEN_ROOT}/Flipbooks"
DATA_DIR = f"{GEN_ROOT}/Data"
MAT_DIR = f"{GEN_ROOT}/Materials"
MAP_DIR = "/Game/Maps"
BP_DIR = f"{GEN_ROOT}/Blueprints"

ASSET_TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
EAL = unreal.EditorAssetLibrary


def log(msg):
    unreal.log(f"[BE] {msg}")


def warn(msg):
    unreal.log_warning(f"[BE] {msg}")


def read_json(name):
    with open(os.path.join(IMPORT_DIR, name), "r", encoding="utf-8") as f:
        return json.load(f)


def ensure_dir(path):
    if not EAL.does_directory_exist(path):
        EAL.make_directory(path)


def load_or_none(path):
    return unreal.load_asset(path) if EAL.does_asset_exist(path) else None


def create_asset(name, path, cls, factory):
    """Erzeugt ein Asset neu (bestehendes wird gelöscht, damit der Import idempotent bleibt)."""
    ensure_dir(path)
    full = f"{path}/{name}"
    if EAL.does_asset_exist(full):
        EAL.delete_asset(full)
    asset = ASSET_TOOLS.create_asset(name, path, cls, factory)
    if asset is None:
        raise RuntimeError(f"Asset konnte nicht erzeugt werden: {full}")
    return asset


def import_texture(png_name, asset_name, srgb=True):
    """PNG importieren: Nearest-Filter, keine Mipmaps, verlustfrei – Pflicht für Pixel-Art."""
    ensure_dir(TEX_DIR)
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", os.path.join(IMPORT_DIR, png_name))
    task.set_editor_property("destination_path", TEX_DIR)
    task.set_editor_property("destination_name", asset_name)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", False)
    ASSET_TOOLS.import_asset_tasks([task])
    tex = unreal.load_asset(f"{TEX_DIR}/{asset_name}")
    if tex is None:
        raise RuntimeError(f"Texturimport fehlgeschlagen: {png_name}")
    tex.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)
    tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    tex.set_editor_property("srgb", srgb)
    tex.set_editor_property("never_stream", True)
    tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_PIXELS2D)
    EAL.save_loaded_asset(tex)
    return tex


def make_sprite(name, texture, frame, pivot_px, sockets=None, material=None):
    """PaperSprite aus einem Atlas-Frame. pivot_px relativ zum Frame; Sockets als {name: (x, y)} relativ zum Frame."""
    sprite = create_asset(name, SPRITE_DIR, unreal.PaperSprite, unreal.PaperSpriteFactory())
    sprite.set_editor_property("source_texture", texture)
    sprite.set_editor_property("source_uv", unreal.Vector2D(frame["x"], frame["y"]))
    sprite.set_editor_property("source_dimension", unreal.Vector2D(frame["w"], frame["h"]))
    sprite.set_editor_property("pivot_mode", unreal.SpritePivotMode.CUSTOM)
    sprite.set_editor_property("custom_pivot_point", unreal.Vector2D(frame["x"] + pivot_px[0], frame["y"] + pivot_px[1]))
    sprite.set_editor_property("pixels_per_unreal_unit", 1.0)
    sprite.set_editor_property("sprite_collision_domain", unreal.SpriteCollisionMode.NONE)
    if material is not None:
        sprite.set_editor_property("default_material", material)
    socks = []
    for sname, (sx, sy) in (sockets or {}).items():
        s = unreal.PaperSpriteSocket()
        s.set_editor_property("socket_name", sname)
        s.set_editor_property("local_transform", unreal.Transform(location=unreal.Vector(sx - pivot_px[0], 0.0, -(sy - pivot_px[1]))))
        socks.append(s)
    sprite.set_editor_property("sockets", socks)
    return sprite


def make_flipbook(name, sprites, fps):
    fb = create_asset(name, FLIPBOOK_DIR, unreal.PaperFlipbook, unreal.PaperFlipbookFactory())
    keys = []
    for sp in sprites:
        k = unreal.PaperFlipbookKeyFrame()
        k.set_editor_property("sprite", sp)
        k.set_editor_property("frame_run", 1)
        keys.append(k)
    fb.set_editor_property("key_frames", keys)
    fb.set_editor_property("frames_per_second", float(fps))
    return fb


def bp_class(name):
    """Generierte Klasse eines Blueprints unter /Game/BE/Blueprints (None, wenn es noch nicht existiert)."""
    return unreal.load_class(None, f"{BP_DIR}/{name}.{name}_C")


def make_data_asset(name, cls):
    """Instanz eines (Blueprint-)PrimaryDataAsset. cls = generierte Klasse, z. B. bp_class("BP_CharacterData")."""
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", cls)
    return create_asset(name, DATA_DIR, cls, factory)


def set_props(obj, **props):
    """set_editor_property für mehrere Blueprint-Variablen; fehlende Variablen werden gemeldet, nicht geworfen."""
    for k, v in props.items():
        try:
            obj.set_editor_property(k, v)
        except Exception as e:  # noqa: BLE001
            warn(f"{obj.get_name()}: Variable '{k}' fehlt oder Typ passt nicht ({e}). Siehe docs/BLUEPRINTS.md, Abschnitt Daten-Blueprints.")


def hex_to_linear(h):
    h = h.lstrip("#")
    r, g, b = (int(h[i:i + 2], 16) / 255.0 for i in (0, 2, 4))
    # sRGB -> linear
    conv = lambda c: c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4
    return unreal.LinearColor(conv(r), conv(g), conv(b), 1.0)


def save_all():
    EAL.save_directory(GEN_ROOT, only_if_is_dirty=False, recursive=True)


def read_level_rows_from_ts(ts_path):
    """Liest die ASCII-Zeilen eines Prototyp-Levels (Prototype/Web/src/data/levels/*.ts)."""
    with open(ts_path, "r", encoding="utf-8") as f:
        src = f.read()
    name = re.search(r"name:\s*'([^']*)'", src)
    rows = re.findall(r"^\s*'([^']*)',\s*$", src, re.M)
    return (name.group(1) if name else os.path.basename(ts_path)), rows
