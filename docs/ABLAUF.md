# Ablauf, Technik & Asset-Pipeline (Unreal Engine 5.8)

Dieses Dokument beschreibt **wie** wir Berlin Expendables bauen: Engine-Entscheidung, Architektur des UE-Projekts, Asset-Pipeline mit AutoSprite, ComfyUI und eigenem Spritegen, Levelformat, Meilensteine und die Arbeitsweise pro Session. Die MCP-Verbindungen (Unreal, AutoSprite, ComfyUI) laufen auf deinem Rechner; Schritt-für-Schritt-Checkliste in [`MCP_RUNBOOK.md`](MCP_RUNBOOK.md).

---

## 1. Engine-Entscheidung: Unreal Engine 5.8

Zielbild: ein **grafisch opulentes 2D-Spiel mit 3D-VFX**. UE 5.8 liefert dafür Paper2D (Sprites, Flipbooks, Sockets), Niagara (GPU-Partikel, Mesh-Partikel, Light-Renderer), echtes Licht auf Sprites (Normalmaps, Punktlichter bei Explosionen), Bloom/Color-Grading, Chaos für Trümmer und einen Desktop-Build ohne Umwege.

Was **nicht** aus der Engine kommt, sondern aus eigenem C++:
- **Zerstörbares Terrain** (Tile-Grid mit HP, Krater, Auto-Kanten) als Instanced-Static-Mesh-Quads in 16×16-Chunks; UE-Kollision wird dafür nicht benutzt.
- **2D-Physik** (AABB gegen das Grid, One-Way-Plattformen, Coyote-Zeit, Eingabepuffer) in einem festen 1/120-s-Schritt. Das Spielgefühl ist damit identisch mit dem Web-Prototyp, dessen Werte 1:1 übernommen wurden (px/Frame × 120 = uu/s bei 2×-Assets).
- **Lokaler 4-Spieler-Input** ohne Enhanced-Input-Assets: Controller 0 pollt beide Tastatur-Layouts, Controller 1–3 die Gamepads (`bOffsetPlayerGamepadIds`). Drop-in per Sprung/Start.

Der Web-Prototyp (`Prototype/Web`) bleibt als **Gameplay-Referenz und Balancing-Sandbox** im Repo: dort lassen sich Regeln in Sekunden testen, bevor sie nach C++ wandern.

## 2. Projektstruktur

```
BerlinExpendables.uproject          UE 5.8, Plugins: Paper2D, Niagara, PythonScriptPlugin, ModelContextProtocol, AllToolsets
.mcp.json                           Claude-Code-Projektkonfiguration: unreal-mcp (http://127.0.0.1:8000/mcp)
Config/                             DefaultEngine/Game/Input/Editor.ini (Pixel-Art-Renderer-Flags, Balancing, Tastenlayouts)
Source/BerlinExpendables/           C++-Modul (16 Klassen, siehe unten)
Content/Python/                     Editor-Automation: be_build_all.py baut alle Assets aus Content/Import
Content/Import/Placeholder/         Spritegen-Platzhalter in 2x (64-px-Figuren, 32-px-Tiles) + Atlanten
Content/Import/AutoSprite/          Ziel für KI-Charaktere (nach pixelize)
Content/Import/ComfyUI/             Ziel für Parallax-Ebenen (nach pixelize)
Content/BE/, Content/Maps/          im Editor erzeugte .uasset/.umap (Git LFS)
Tools/spritegen/                    prozeduraler Pixel-Art-Generator (Node)
Tools/pixelize/pixelize.py          KI-Output -> Pixelraster, Palette, Outline, Normalmap, Tiling-Check
Tools/comfyui/workflows/            ComfyUI-Workflow-Vorlagen (API-Format)
Prototype/Web/                      Web-Prototyp (Vite/TS), npm run dev
docs/                               Konzept, Styleguide, Prompt-Bücher, Runbook, Screenshots
```

### C++-Klassen (Source/BerlinExpendables)

| Klasse | Aufgabe |
| --- | --- |
| `BETypes.h` | Enums (Tiles, Aim, Items), `FBEInputFrame`, `FBEBody` (AABB), Waffen-/Projektilprofile, Levelraum↔UE-Umrechnung (`X=x, Z=-y, Y=Tiefe`) |
| `UBEGameSettings`, `UBEInputSettings` | Project Settings: Balancing, Auflösung 640×360, Asset-Referenzen, VFX-Systeme, Tastenlayouts (alles in `DefaultGame.ini`) |
| `UBELevelAsset`, `UBECharacterData`, `UBEWeaponLibrary`, `UBESpriteLibrary`, `UBETileSet` | DataAssets, vom Python-Import befüllt |
| `ABETileWorld` | Grid, HP, Chunk-ISMs mit Custom-Data (UV-Rechteck + Schaden), Hintergrund- und Erdreich-Ebene, `MoveX/MoveY`, `DamageRadius`, Delegate `OnTileDestroyed` |
| `ABEPawnBase` | Flipbook-Körper, Waffensprite am Socket `Hand`, Mündung am Socket `Muzzle`, fester Simulationsschritt |
| `ABEBro` | Spieler: Laufen, Springen, Ducken, Zielen (h/u/d), Feuern, Granaten, Auto-Nahkampf, Pickups H/R/F/L, Kamerarand, Respawn |
| `ABEMook` | Gegner-KI: Patrouille, Alarm (`!`), Salven, Panik, Betäubung |
| `ABEProjectile`, `ABEGrenade` | Substep-Kollision, Durchschlag, Schwerkraft/Lob, Explosionen, Abpraller |
| `ABEBarrel`, `ABECage`, `ABECaptive`, `ABEItem`, `ABEExit` | Gasflasche (Kettenreaktion), Käfig + Rettung, Items, Fluchttrabi |
| `UBECombatLibrary` | `Explode()` (Terrain, Gegner, Spieler, Fässer, Käfige, FX, Licht, Shake, Hitstop), Trümmer (`ABEDebris`, 3D-Würfel), Lichtblitz (`ABEFlash`) |
| `ABEGameState` | Leben, Punkte, Entity-Listen, Tod/Respawn/Rettung/Bro-Wechsel, Popups, Hitstop per Zeitdilatation |
| `ABEGameMode`, `UBEGameInstance` | Weltaufbau aus Level-Asset, Spawns, 4 lokale Spieler, Beitritte, Neustart mit Wiederbeitritt |
| `ABEPlayerController` | Tasten/Gamepad lesen, Eingabeframes, Join, F1-Debug |
| `ABECameraActor`, `ABEParallaxLayer` | Ortho-Koop-Kamera (Bounding-Box, Shake, Pixel-Snapping, Bloom), manuelle Parallax-Ebenen |
| `ABEHUD` | Canvas-HUD: Spielerkarten, Leben, Punkte, Banner, Popups, Titel, Endbildschirm, Debug-Boxen |

**Status:** Der Code ist vollständig, aber **noch nicht kompiliert** (in der Cloud-Session gibt es kein UE). Die erste lokale Editor-Session kompiliert per Live Coding und behebt Abweichungen der 5.8-API.

## 3. Rendering-Konzept „opulentes 2D“

- **Auflösung:** 640×360 Welteinheiten im Bild (OrthoWidth 640). 1 uu = 1 Texel. Bei 1080p ist jeder Spielpixel exakt 3×3 Bildschirmpixel, bei 4K 6×6; die Kamera snappt auf ganze Einheiten, Sprites bleiben knackig.
- **Sprites:** Paper2D, Nearest-Filter, keine Mips, lit-Material mit Vertexfarbe und optionaler Normalmap (`SpriteNormal`, erzeugt von `pixelize.py --normals`).
- **Licht:** kühles Mondlicht + Skylight als Basis, warme Punktlichter aus Mündungsfeuer und Explosionen (`ABEFlash`), Neon aus Emissive. Bloom an, TAA/Motion Blur/Auto-Exposure aus.
- **3D-VFX:** Niagara-Systeme für Explosion (Sprite-Sheet + GPU-Funken + Rauchring + Light-Renderer), Trümmer als Mesh-Partikel, Blut/Konfetti, Staub; physikalische Trümmerwürfel (`ABEDebris`) als sofort funktionierender Fallback. Später: Chaos-Fracture für Autos/Props, Volumetric Fog in der Club-Welt, Regen.
- **Parallax:** Himmel statisch, Skyline 0,15, Häuserzeile 0,4, Nahebene 0,7; gemalte Ebenen aus ComfyUI, durch Pixelize in die Palette gebracht.

## 4. Asset-Pipeline

| Quelle | Wofür | Weg ins Spiel |
| --- | --- | --- |
| **AutoSprite** (MCP auf deinem PC) | Bros, Gegner, Gefangene, Bosse – Charakterdetail mit Animationen | Export (PNG + Atlas) → `pixelize.py --grid 64 --outline --normals` → `Content/Import/AutoSprite/<id>/` → `be_import_sprites.py` (BE_IMPORT_DIR) |
| **ComfyUI** (MCP auf deinem PC) | Parallax-Ebenen, Titelbild, Weltkarte, Stimmungsbilder | Workflow `Tools/comfyui/workflows/parallax_layer.json` → `pixelize.py --width 960 --dither --tile-check` → Sprite → `ABEParallaxLayer` |
| **Spritegen** (Node, hier) | Platzhalter in korrekter Größe, Tiles, Items, FX-Sheets, Waffen, Prototyp-Assets | `npm run assets` → `Content/Import/Placeholder` (2×) und `Prototype/Web/public/assets` (1×) |
| **Niagara/Materialien** (Editor, MCP) | 3D-VFX, Tile- und Sprite-Materialien | `be_make_materials.py` + Niagara-Toolset per MCP |

Verbindliche Maße: Figur-Frame 64×64 (Füße y = 61), Box 20×44 (geduckt 20×34), Tiles 32×32, Items 20×20, Projektile 6–18 px. Sockets: `Hand` am Körper, `Muzzle` an der Waffe, Pivot der Waffe = Griff. Details und Prompts: `PROMPTS_AUTOSPRITE.md`, `PROMPTS_COMFYUI.md`, `STYLEGUIDE.md`.

## 5. Levelformat (ASCII)

Levels sind Textzeilen in einem `UBELevelAsset` (aus `Prototype/Web/src/data/levels/*.ts` übernommen). Ein Zeichen = ein 32-px-Tile:

| Zeichen | Bedeutung | | Zeichen | Bedeutung |
| --- | --- | --- | --- | --- |
| `.` | Luft | | `P` | Spielerstart |
| `#` | Erde (HP 3, Gras-Kante automatisch) | | `m` / `s` | Anzug-Scherge / Security |
| `=` | Ziegel (HP 6, Betonkrone automatisch) | | `b` | Gasflasche |
| `S` | Stahl (unzerstörbar) | | `k` | Käfig mit Gefangenem |
| `C` | Beton (HP 9) | | `c` | Kiste (HP 2, Item-Chance) |
| `-` | Plattform (einseitig) | | `H R F L B` | Pickups |
| `w W r g` | Hintergrund: Putz, Fenster, Ziegel, Graffiti | | `+` | Extraleben |
| | | | `E` | Fluchttrabi |

Entities stehen in der Luftkachel über dem Boden. Später: In-Editor-Leveleditor als Editor-Utility-Widget (MCP/Python), das dieses Format schreibt.

## 6. Meilensteine

| # | Name | Inhalt | Definition of Done |
| --- | --- | --- | --- |
| **M0** | Fundament ✅ | Konzept, Styleguide, Web-Prototyp als Referenz, Spritegen, UE-5.8-Projekt mit vollständigem C++-Port, Editor-Automation, Pixelize-Pipeline, Prompt-Bücher, MCP-Runbook | Repo gepusht, Prototyp läuft, Python syntaxgeprüft |
| **M1** | Erster Editor-Lauf (lokal, MCP) | C++ kompiliert, `be_build_all.py` durch, PIE spielbar mit Platzhaltern, Niagara-Grundsysteme, erste AutoSprite-Bros und ComfyUI-Ebenen für Welt 1 | 4 Spieler in PIE, Krater, Explosionen mit Licht und Trümmern, ein KI-Bro im Spiel |
| **M2** | Vertical Slice 1-1 | Level 1-1 fertig, Checkpoints, Koffer-Bomber, Wachhund, Combo-Score, UMG-HUD statt Canvas, Join-Screen mit Bro-Vorschau, SFX/Musik-Grundlage (MetaSounds) | 4 Leute spielen 1-1 durch |
| **M3** | Die 4 Bros komplett | Echte Specials (Strobo, Bus, Bierkasten, Pommes-Splitter), Arcade-HP-Modus, Konfetti-Option, Normalmap-Licht auf allen Figuren, Chaos-Trümmer für Props | Jeder Bro fühlt sich anders an |
| **M4** | Welt 1 komplett | 1-2 Dächer, 1-3 Kanal, Boss Abriss-Bernd (Sprite-Hierarchie), Weltkarte, Menüs, Speicherstand | Welt 1 in 15 Minuten durchspielbar |
| **M5** | Content-Ausbau | Welt 2 + 3, Fahrzeuge (Trabi-Slug, BVG-Bus), 4 weitere Bros, Drohnen, Scharfschützen | 3 Welten, 8 Bros |
| **M6** | Polish & Release | Welt 4 + 5, Finale, Balancing, Performance, Controller-Remapping, Packaging Windows/Linux, Steam-Deck-Test | Release-Kandidat |

## 7. Arbeitsweise pro Session

**Lokal (mit MCP):** siehe `MCP_RUNBOOK.md`. Kurz: Editor offen, MCP-Server läuft, `claude` im Projektordner, Änderungen an C++ → `LiveCodingToolset.CompileLiveCoding`, Assets → Python/MCP, Test in PIE, speichern, committen (LFS).

**Cloud/CLI ohne Editor:** alles Textbasierte (C++, Python, Levels, Docs, Spritegen, Pixelize). Prüfungen:
```bash
npm install && npm run check        # Spritegen + Typecheck + Prototyp-Build
python3 -m py_compile Content/Python/*.py Tools/pixelize/pixelize.py
npm run preview & CHROMIUM_PATH=/opt/pw-browsers/chromium npm run shot   # Prototyp-Screenshots
```

Balancing-Werte stehen an genau zwei Stellen: `Config/DefaultGame.ini` (Engine) und `Prototype/Web/src/game/characters.ts` / `constants.ts` (Prototyp). Wer eines ändert, zieht das andere nach.

## 8. Offene Entscheidungen

1. **Blut oder Konfetti als Default?** (`bGore` in den Settings, aktuell Blut)
2. **Tastatur-Layout P2:** `, . /` + `M` oder Numpad?
3. **AutoSprite-Stil:** 64-px-Figuren wie geplant, oder 96 px für mehr Detail (dann Tiles 48 px, Auflösung 960×540)?
4. **Normalmap-Licht auf Sprites** von Anfang an (opulenter, mehr Pipeline-Aufwand) oder erst ab M3?
5. **Bro-Roster** und **Bösewicht-Namen** („Beton & Rendite AG", „Baron von Beton") – Änderungswünsche?

## 9. Risiken & Gegenmaßnahmen

| Risiko | Gegenmaßnahme |
| --- | --- |
| C++ wurde ohne Compiler geschrieben | Erste lokale Session ist eine reine Kompilier-Runde; konservative, stabile APIs verwendet (Paper2D, ISM, Niagara-Basis) |
| Paper2D gilt als „wartungsarm" | Nur Sprites/Flipbooks/Sockets genutzt; Terrain, Physik, Kamera sind eigen; Wechsel auf eigene Sprite-Quads wäre lokal in `ABEPawnBase` |
| KI-Assets passen stilistisch nicht | Pixelize erzwingt Raster, Palette, Outline; Kontaktbogen-Review neben Spritegen-Figuren |
| Ortho-Kamera + Lichter/Schatten | Schatten aus, nur Punktlichter und Bloom; falls nötig Perspektivkamera mit schmalem FOV (Pixel-Snapping bleibt) |
| 4-Spieler-Input in UE | Key-Polling statt Enhanced Input, keine Asset-Abhängigkeit; Gamepad-Offset in `DefaultInput.ini` |
