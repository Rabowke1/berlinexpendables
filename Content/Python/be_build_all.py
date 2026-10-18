"""Kompletter Editor-Aufbau in einem Rutsch (im Editor: Python-Konsole `py be_build_all.py` oder per MCP):
 1. Materialien  2. Sprites/Flipbooks/DataAssets importieren  3. Level-Assets, Map, Settings.
Läuft auch ohne die Daten-Blueprints (dann nur Sprites/Flipbooks/Materialien); nach dem Anlegen der
Blueprints einfach erneut ausführen. Danach: Play-in-Editor in L_Kreuzberg01, Sprung/Start drücken."""
import importlib
import unreal

for m in ("be_common", "be_make_materials", "be_import_sprites", "be_setup_level"):
    if m in globals():
        importlib.reload(globals()[m])

import be_common
import be_make_materials
import be_import_sprites
import be_setup_level


def main():
    with unreal.ScopedSlowTask(3, "Berlin Expendables: Assets aufbauen") as task:
        task.make_dialog(True)
        task.enter_progress_frame(1, "Materialien")
        mats = be_make_materials.run(lit=True)
        task.enter_progress_frame(1, "Sprites & Daten")
        assets = be_import_sprites.run(sprite_material=mats["sprite_lit"])
        task.enter_progress_frame(1, "Level & Settings")
        be_setup_level.run(assets, mats)
    be_common.log("Fertig. Map /Game/Maps/L_Kreuzberg01 öffnen und PIE starten.")


if __name__ == "__main__":
    main()
else:
    main()
