"""Importiert die Sprite-Atlanten (PNG + JSON) nach /Game/BE: Texturen, PaperSprites mit Pivots & Sockets,
Flipbooks und – sobald die Daten-Blueprints BP_CharacterData, BP_SpriteLibrary, BP_WeaponLibrary, BP_TileSet
existieren (docs/BLUEPRINTS.md) – die DataAsset-Instanzen. Quelle: Content/Import/Placeholder
(oder BE_IMPORT_DIR, z. B. ein AutoSprite-Export nach dem Pixelize-Schritt).
Ohne die Blueprint-Klassen werden nur Sprites/Flipbooks erzeugt (Namensschema FB_<id>_<anim>_<aim>)."""
import unreal
from be_common import *  # noqa: F401,F403

# Animationsgeschwindigkeiten (Frames pro Sekunde)
FPS = {"idle": 5, "run": 12, "jump": 1, "fall": 1, "crouch": 1, "melee": 15, "panic": 12,
       "explosion": 20, "muzzle": 30, "smoke": 15, "wave": 15, "bottle": 15, "flame": 15}

# Balancing (Prototyp-Werte: px/Frame x120 = uu/s bei 2x-Assets; Frames/60 = s)
CHAR_TABLE = {
    "willi":    dict(name="CURRYWURST WILLI", tagline="MIT ALLES UND SCHARF!", player=True, weapon="shotgun", grenade_radius=68.0,
                     w=dict(cooldown=0.433, count=5, spread=0.26, recoil=168.0, shake=0.25), b=dict(sprite="pellet", speed=660.0, life=0.37)),
    "doro":     dict(name="TECHNODORE", tagline="DU KOMMST HIER NICHT REIN.", player=True, weapon="bass", grenade_radius=60.0,
                     w=dict(cooldown=0.3, count=1, spread=0.0, recoil=48.0, shake=0.12), b=dict(sprite="wave", speed=540.0, life=1.0, pierce=3, tile=2.0)),
    "berti":    dict(name="BVG-BERTI", tagline="ZURUECKBLEIBEN BITTE!", player=True, weapon="smg", grenade_radius=60.0,
                     w=dict(cooldown=0.083, count=1, spread=0.08, recoil=18.0, shake=0.05), b=dict(sprite="smgbullet", speed=960.0, life=0.83)),
    "sven":     dict(name="SPAETI-SVEN", tagline="PFAND IST NICHT DRIN.", player=True, weapon="bottle", grenade_radius=60.0,
                     w=dict(cooldown=0.4, count=1, spread=0.04, recoil=24.0, shake=0.08), b=dict(sprite="bottle", speed=576.0, life=2.33, gravity=1224.0, lob=312.0, explode=44.0, spin=True)),
    "mook":     dict(name="ANZUG-SCHERGE", tagline="", player=False, weapon="pistol", hp=1, burst=1, gap=0.0, cd=(1.0, 1.83), sight=300.0, walk=60.0, score=100,
                     w=dict(cooldown=1.0, count=1, spread=0.0, recoil=0.0, shake=0.0), b=dict(sprite="mookbullet", speed=384.0, life=1.83, tile=0.5)),
    "security": dict(name="SECURITY", tagline="", player=False, weapon="rifle", hp=2, burst=3, gap=0.12, cd=(1.33, 2.17), sight=380.0, walk=48.0, score=150,
                     w=dict(cooldown=1.0, count=1, spread=0.0, recoil=0.0, shake=0.0), b=dict(sprite="mookbullet", speed=480.0, life=1.83, tile=0.5)),
    "captive":  dict(name="GEFANGENER", tagline="", player=False, weapon="pistol", hp=1, burst=0, gap=0.0, cd=(9.0, 9.0), sight=0.0, walk=60.0, score=0,
                     w=dict(cooldown=1.0, count=0, spread=0.0, recoil=0.0, shake=0.0), b=dict(sprite="mookbullet", speed=1.0, life=0.1)),
}


def group_frames(atlas):
    """{anim_name: [frame_name, ...]} aus dem Atlas (Suffix _n)."""
    return atlas.get("animations", {})


def import_characters(sprite_material, fx_flipbooks):
    palette = read_json("palette.json")
    result = {}
    for cid, spec in CHAR_TABLE.items():
        atlas = read_json(f"{cid}.json")
        tex = import_texture(f"{cid}.png", f"T_{cid}")
        portrait_tex = import_texture(f"{cid}_portrait.png", f"T_{cid}_portrait")
        meta = atlas["meta"]
        fs = meta.get("frameSize", 64)
        feet_y = fs - 2  # Füße auf y = 30 (1x) -> 60 (2x) + 1 Outline-Reihe
        sprites = {}
        for fname, fdef in atlas["frames"].items():
            fr = fdef["frame"]
            if fname.startswith(("gib_", "portrait")):
                pivot = (fr["w"] / 2.0, fr["h"] / 2.0)
                sockets = None
            else:
                pivot = (fs / 2.0, feet_y)
                sockets = {"Hand": tuple(fdef["hand"])} if "hand" in fdef else None
            sprites[fname] = make_sprite(f"S_{cid}_{fname}", tex, fr, pivot, sockets, sprite_material)
        flipbooks = {}
        for anim, names in group_frames(atlas).items():
            base = anim.split("_")[0]
            flipbooks[anim] = make_flipbook(f"FB_{cid}_{anim}", [sprites[n] for n in names], FPS.get(base, 10))

        cls = bp_class("BP_CharacterData")
        if cls is None:
            warn(f"BP_CharacterData fehlt – {cid}: nur Sprites/Flipbooks erzeugt.")
            result[cid] = None
            log(f"Charakter {cid}: {len(sprites)} Sprites, {len(flipbooks)} Flipbooks")
            continue
        da = make_data_asset(f"DA_Char_{cid}", cls)
        box, cbox = meta["box"], meta["crouchBox"]
        w, b = spec["w"], spec["b"]
        cd = spec.get("cd", (1.0, 1.8))
        set_props(da,
            Id=cid, DisplayName=unreal.Text(spec["name"]), Tagline=unreal.Text(spec["tagline"]),
            Accent=hex_to_linear(meta.get("accent", "#ffffff")), Skin=hex_to_linear(meta.get("skin", "#f1c27d")),
            IsPlayer=spec["player"], Flipbooks={unreal.Name(k): v for k, v in flipbooks.items()}, Portrait=portrait_tex,
            Gibs=[sprites[n] for n in ("gib_head", "gib_torso", "gib_leg") if n in sprites],
            BodySize=unreal.Vector2D(box["w"], box["h"]), CrouchSize=unreal.Vector2D(cbox["w"], cbox["h"]),
            WeaponId=spec["weapon"], Cooldown=w["cooldown"], Count=w["count"], Spread=w["spread"], Recoil=w["recoil"], Shake=w["shake"],
            BulletFlipbook=fx_flipbooks.get(b["sprite"]), BulletSpeed=b["speed"], BulletLife=b["life"],
            BulletDamage=float(b.get("dmg", 1.0)), BulletTileDamage=float(b.get("tile", 1.0)), BulletPierce=int(b.get("pierce", 0)),
            BulletGravity=float(b.get("gravity", 0.0)), BulletLob=float(b.get("lob", 0.0)), BulletExplode=float(b.get("explode", 0.0)),
            BulletSpin=bool(b.get("spin", False)), Grenades=3, GrenadeRadius=spec.get("grenade_radius", 60.0),
            Hp=int(spec.get("hp", 1)), Burst=int(spec.get("burst", 1)), BurstGap=float(spec.get("gap", 0.12)),
            CooldownMin=float(cd[0]), CooldownMax=float(cd[1]), SightRange=float(spec.get("sight", 300.0)),
            WalkSpeed=float(spec.get("walk", 60.0)), Score=int(spec.get("score", 100)))
        result[cid] = da
        log(f"Charakter {cid}: {len(sprites)} Sprites, {len(flipbooks)} Flipbooks")
    return result


def import_weapons(sprite_material):
    atlas = read_json("weapons.json")
    tex = import_texture("weapons.png", "T_weapons")
    sprites = {}
    for fname, fdef in atlas["frames"].items():
        grip, tip = fdef["grip"], fdef["tip"]
        sprites[fname] = make_sprite(f"S_weapon_{fname}", tex, fdef["frame"], (grip[0], grip[1]), {"Muzzle": (tip[0], tip[1])}, sprite_material)
    cls = bp_class("BP_WeaponLibrary")
    lib = None
    if cls is None:
        warn("BP_WeaponLibrary fehlt – Waffensprites erzeugt, kein DataAsset.")
    else:
        lib = make_data_asset("DA_WeaponLibrary", cls)
        set_props(lib, Sprites={unreal.Name(k): v for k, v in sprites.items()})
    log(f"Waffen: {len(sprites)} Sprites")
    return lib


def import_fx_and_props(sprite_material):
    """FX-Atlas (Projektile, Explosion ...) + Tiles-Atlas (Props/Items) + Hintergründe -> SpriteLibrary."""
    sprites, flipbooks = {}, {}
    fx = read_json("fx.json")
    fx_tex = import_texture("fx.png", "T_fx")
    fx_sprites = {}
    for fname, fdef in fx["frames"].items():
        fr = fdef["frame"]
        fx_sprites[fname] = make_sprite(f"S_fx_{fname}", fx_tex, fr, (fr["w"] / 2.0, fr["h"] / 2.0), None, sprite_material)
    for anim, names in fx.get("animations", {}).items():
        flipbooks[anim] = make_flipbook(f"FB_fx_{anim}", [fx_sprites[n] for n in names], FPS.get(anim, 12))
    for single in ("pellet", "smgbullet", "mookbullet", "heavybullet", "rocket", "laserbeam"):
        if single in fx_sprites:
            flipbooks[single] = make_flipbook(f"FB_fx_{single}", [fx_sprites[single]], 1)
    for single in ("grenade", "marker", "alert"):
        if single in fx_sprites:
            sprites[single] = fx_sprites[single]

    tiles = read_json("tiles.json")
    tiles_tex = import_texture("tiles.png", "T_tiles")
    for fname, fdef in tiles["frames"].items():
        if fname.startswith(("item_", "crate", "barrel", "cage", "trabi")):
            fr = fdef["frame"]
            sprites[fname] = make_sprite(f"S_prop_{fname}", tiles_tex, fr, (fr["w"] / 2.0, fr["h"] / 2.0), None, sprite_material)

    for bg in ("bg_sky", "bg_skyline", "bg_mid"):
        tex = import_texture(f"{bg}.png", f"T_{bg}")
        w, h = tex.blueprint_get_size_x(), tex.blueprint_get_size_y()
        sprites[bg] = make_sprite(f"S_{bg}", tex, {"x": 0, "y": 0, "w": w, "h": h}, (w / 2.0, h / 2.0), None, sprite_material)

    cls = bp_class("BP_SpriteLibrary")
    lib = None
    if cls is None:
        warn("BP_SpriteLibrary fehlt – Sprites/Flipbooks erzeugt, kein DataAsset.")
    else:
        lib = make_data_asset("DA_SpriteLibrary", cls)
        set_props(lib, Sprites={unreal.Name(k): v for k, v in sprites.items()}, Flipbooks={unreal.Name(k): v for k, v in flipbooks.items()})
    log(f"SpriteLibrary: {len(sprites)} Sprites, {len(flipbooks)} Flipbooks")
    return lib, flipbooks, tiles_tex, tiles


def import_tileset(tiles_tex, tiles_atlas):
    cls = bp_class("BP_TileSet")
    if cls is None:
        warn("BP_TileSet fehlt – Tile-Atlas importiert, kein DataAsset.")
        return None
    ts = make_data_asset("DA_TileSet", cls)
    size = tiles_atlas["meta"]["size"]
    rects = {}
    for fname, fdef in tiles_atlas["frames"].items():
        fr = fdef["frame"]
        # (u0, v0, u1, v1) normalisiert
        rects[unreal.Name(fname)] = unreal.Vector4(fr["x"] / size["w"], fr["y"] / size["h"], (fr["x"] + fr["w"]) / size["w"], (fr["y"] + fr["h"]) / size["h"])
    set_props(ts, Atlas=tiles_tex, Rects=rects)
    log(f"TileSet: {len(rects)} Rechtecke")
    return ts


def run(sprite_material=None):
    lib, fx_flipbooks, tiles_tex, tiles_atlas = import_fx_and_props(sprite_material)
    weapons = import_weapons(sprite_material)
    chars = import_characters(sprite_material, fx_flipbooks)
    tileset = import_tileset(tiles_tex, tiles_atlas)
    save_all()
    return dict(sprite_library=lib, weapon_library=weapons, characters=chars, tileset=tileset)


if __name__ == "__main__":
    run()
