# Ablauf, Technik & Asset-Pipeline

Dieses Dokument beschreibt **wie** wir Berlin Expendables bauen: Technologie, Architektur, Asset-Pipeline (eigener Spritegen + AutoSprite), Levelformat, Meilensteine und die Arbeitsweise pro Session.

---

## 1. Technologie-Entscheidung

**Gewählt: TypeScript + Vite + eigener Canvas-2D-Renderer (kein Game-Framework).**

| Kriterium | Eigene Canvas-Engine (gewählt) | Phaser 3 | Godot 4 |
| --- | --- | --- | --- |
| Zerstörbares Tile-Terrain mit eigener Physik | volle Kontrolle, ~300 Zeilen | geht, aber gegen Arcade-Physics gearbeitet | gut, aber Tilemap-Mutation + Kollision selbst bauen |
| Pixel-perfektes Rendering (Integer-Skalierung, kein Smoothing) | trivial | möglich, Fallstricke bei Kamera-Zoom | gut |
| Lokaler 4-Spieler-Input | Gamepad-API direkt | ok | sehr gut |
| In dieser Cloud-Session baubar & testbar (Screenshots per Playwright) | ✅ | ✅ | ❌ (kein Godot-Binary, Export aufwendig) |
| Distribution | Browser sofort; Desktop später per Tauri/Electron; Steam-Deck-Browser | gleich | native Exporte |
| Bundle | ~55 KB JS + ~200 KB Assets | ~1,2 MB | – |

Fazit: Für ein Broforce-artiges Spiel ist das Terrain der Kern, und das will man selbst in der Hand haben. Die Engine ist bewusst klein; falls wir später doch Godot wollen, sind Konzept, Palette, Spritegen-Assets und Levelformat 1:1 übertragbar.

## 2. Architektur

```
src/
  main.ts                 Boot: Assets laden, Welt erzeugen, Loop starten
  engine/                 spielunabhängig
    renderer.ts           480x270-Pixelbuffer, Integer-Skalierung, Sprite/Parallax/Text
    font.ts               5x7-Bitmap-Font (pro Farbe gecacht)
    input.ts              Tastatur x2 + Gamepads x4, Edge-Detection, Join-Logik
    camera.ts             Bounding-Box-Kamera + Shake (Trauma²)
    loop.ts               fester 60-Hz-Step mit Akkumulator
    audio.ts              WebAudio-Retro-Synth (SFX ohne Dateien)
    assets.ts             Atlas-Loader (TexturePacker-JSON-Hash + Zusatzfelder)
  game/
    world.ts              Zustand, Entity-Listen, Explosionen, Rettung, Respawn, Draw-Reihenfolge
    tilemap.ts            Tile-Typen, HP, AABB-Kollision (moveX/moveY, One-Way-Plattformen), Zeichnen mit Auto-Kanten + Rissen
    level.ts              ASCII-Parser
    characters.ts         Bros, Pickups, Gegnerprofile (alle Balancing-Zahlen an einem Ort)
    particles.ts, hud.ts
    entities/             player, mook, bullet, grenade, props (Fass, Käfig, Gefangener, Item, Trabi), fx (Explosion, Gibs, Popups)
  data/levels/            Levels als ASCII
tools/spritegen/          prozeduraler Pixel-Art-Generator (Node, tsx)
public/assets/            generierte PNG + JSON (eingecheckt)
scripts/shot.mjs          Playwright-Smoke-Test, schreibt docs/screenshots/
```

**Prinzipien**
- Simulation läuft mit festem Schritt (1/60 s), alle Geschwindigkeiten sind px/Frame. Rendering interpoliert nicht (Pixel-Art).
- Entities sind einfache Klassen in typisierten Listen (`players`, `mooks`, `bullets` …), keine ECS-Magie. `World` ist der einzige Ort mit Querbeziehungen (Explosion trifft alles).
- Keine Sprite-Rotation außer 90°-Schritte (Waffen hoch/runter, Kugeln vertikal). Spiegeln nur horizontal.
- Zahlen fürs Balancing stehen ausschließlich in `characters.ts` und `constants.ts`.

## 3. Asset-Pipeline

### 3.1 Spritegen (unser Generator) – der garantierte Weg

`npm run assets` → `tools/spritegen/index.ts` erzeugt deterministisch alle Assets:

- **Humanoid-Rig** (`humanoid.ts`): ein 32×32-Rig zeichnet Kopf, Torso, Arme, Beine pixelweise aus einem **Rezept** (Hautton, Haar/Hut-Stil, Shirt, Hose, Schuhe, Akzentfarbe, Props wie Schürze, Krawatte, Weste, Sonnenbrille, Bart). Posen: `idle` ×4, `run` ×8, `jump`, `fall`, `crouch`, `melee` ×3, `panic` ×4 (Gegner) – jeweils für Zielrichtung `h`/`u`/`d`. Dazu Porträt und Gibs.
- **Waffen getrennt** (`weapons.ts`): jede Waffe als eigenes Sprite in 3 Ausrichtungen mit **Griff-** und **Mündungsanker**; der Körper exportiert pro Frame den **Hand-Anker**. Das Spiel setzt die Waffe zur Laufzeit zusammen → Waffen-Pickups ohne Frame-Explosion (Metal-Slug-Anforderung).
- **Tiles/Props/Items** (`tiles.ts`), **Effekte/Projektile** (`fx.ts`), **Parallax-Hintergründe** (`backgrounds.ts`: Himmel mit Dither-Gradient, Skyline mit Fernsehturm und Kran, Altbau-Zeile mit Spätis).
- **Atlas-Format** (`atlas.ts`): Shelf-Packing, TexturePacker-kompatibles JSON-Hash (`frames[name].frame`), plus `animations` (per Suffix `_n` gruppiert) und Zusatzfelder `hand`, `grip`, `tip`, `meta.box`.
- **Kontaktbogen**: `docs/art/preview.png` (4× skaliert) und `docs/art/palette.png` zum schnellen Review.

Neue Figur = neues Rezept in `characters.ts` (≈8 Zeilen). Neue Pose = neue `poseFor`-Variante.

### 3.2 AutoSprite (autosprite.io) – KI-generierte Sprites

Status: Aus dieser Cloud-Session ist `autosprite.io` durch die Netzwerk-Policy der Umgebung **nicht erreichbar** (siehe Hinweis in der Session). Die Pipeline ist so gebaut, dass AutoSprite-Output **ohne Codeänderung** einsetzbar ist:

1. In AutoSprite (Web-App, MCP oder REST) aus einem Referenzbild/Preset die Animationen `idle`, `run`, `jump`, `fall`, `crouch`, `melee`, `death` erzeugen und als **PNG-Sheet + Atlas-JSON** exportieren (TexturePacker/Phaser-Format).
2. `tools/import-atlas` (Meilenstein 1) normalisiert das JSON auf unser Format, benennt Animationen nach unserem Schema (`run_h_0 …`) und ergänzt Hand-Anker (halbautomatisch: Hautfarbe-Blob am rechten Rand des Torsos, sonst manuell im JSON).
3. **Pixelize-Schritt (Pflicht):** KI-Output ist oft nur „pixel-art-ähnlich" (Anti-Aliasing, kein sauberes Raster, Farbrauschen). Der Schritt macht Nearest-Neighbor-Downscale auf das 32-px-Raster, quantisiert auf die **Berlin-Nacht-Palette**, entfernt Einzelpixel-Rauschen und zieht die 1-px-Outline. Erst danach gilt ein Sprite als „echte Pixel-Art" im Sinne des Styleguides.
4. Review auf dem Kontaktbogen neben den Spritegen-Figuren: Proportionen (Kopf ≈ 8 px, Figur ≈ 23 px), Fußlinie, Lesbarkeit bei 1×.

Empfohlene Arbeitsteilung: **Spritegen** für alles, was konsistent und zahlreich sein muss (Bros, Mooks, Tiles, FX), **AutoSprite** für Einzelstücke mit viel Charakter (Bosse, Fahrzeuge, Porträts, Titelbild) – immer durch den Pixelize-Schritt.

### 3.3 Konventionen

| Asset | Größe | Anker |
| --- | --- | --- |
| Figur | 32×32 Frame, Box 10×22 (geduckt 10×17), Füße y=29 | Frame-Ursprung = (box.x − 11, box.y − 8) |
| Tile | 16×16 | oben links |
| Waffe | variabel, 1 px Outline-Rand | `grip` in der Hand, `tip` = Mündung |
| Explosion | 32×32 ×8 | Mitte |
| Items | 10×10 | oben links |

Animationsnamen: `<anim>_<aim>_<index>`; Aim ∈ {h, u, d}. Blickrichtung links = horizontal gespiegelt (x' = 31 − x).

## 4. Levelformat (ASCII)

Eine Datei pro Level (`src/data/levels/*.ts`), jede Zeile eine Tilereihe:

| Zeichen | Bedeutung | | Zeichen | Bedeutung |
| --- | --- | --- | --- | --- |
| `.` | Luft | | `P` | Spielerstart |
| `#` | Erde (HP 3, Gras-Kante automatisch) | | `m` / `s` | Anzug-Scherge / Security |
| `=` | Ziegel (HP 6, Betonkrone automatisch) | | `b` | Gasflasche (explodiert, Kettenreaktion) |
| `S` | Stahl (unzerstörbar) | | `k` | Käfig mit Gefangenem |
| `C` | Beton (HP 9) | | `c` | Kiste (HP 2, kann Item droppen) |
| `-` | Plattform (einseitig durchlässig) | | `H R F L B` | Waffen-/Bomben-Pickup |
| `w W r g` | Hintergrund: Putz, Fenster, Ziegel, Graffiti | | `+` | Extraleben (Currywurst) |
| | | | `E` | Fluchttrabi (Levelende) |

Entities stehen in der Luftkachel **über** dem Boden. Ein kleines Prüfskript (siehe Session-Notizen) meldet schwebende Objekte; das wird in M1 Teil von `npm run check`.

## 5. Meilensteine

| # | Name | Inhalt | Definition of Done |
| --- | --- | --- | --- |
| **M0** | **Fundament** ✅ | Konzept, Styleguide, Projekt-Setup, Spritegen v1 (4 Bros, 2 Gegner, Gefangener, 10 Waffen, Tileset, FX, Hintergründe), Engine (Renderer, Input ×4, Kamera, Font, Synth), Gameplay-Prototyp: Laufen/Springen/Ducken, 4 Signaturwaffen, Granaten, Explosionen mit Terrainzerstörung, Gegner-KI (Patrouille, Alarm, Salven, Panik), Käfige/Rettung/Bro-Wechsel, Pickups H/R/F/L/B, Fluchttrabi, HUD, Drop-in-Koop, Game Over/Win | `npm run check` grün, Smoke-Test liefert Screenshots, Branch gepusht |
| **M1** | Vertical Slice 1-1 | Level 1-1 fertig designt (3 Minuten, 3 Set-Pieces), Checkpoints, Koffer-Bomber, Wachhund, Combo-Score, Pause-Menü, Join-Screen mit Bro-Vorschau, Level-Validator in `check`, `import-atlas` + Pixelize für AutoSprite | 4 Leute spielen 1-1 durch, ohne auf Code zu schauen |
| **M2** | Die 4 Bros komplett | Echte Specials (Strobo, Bus, Bierkasten, Pommes-Splitter), Nahkampf-Feinschliff, Freundbeschuss-Option, Arcade-HP-Modus, Konfetti-Modus im Menü, Hitstop/Shake-Tuning, mehr SFX | Jeder Bro fühlt sich anders an (Playtest-Fragebogen) |
| **M3** | Welt 1 komplett | 1-2 Dächer, 1-3 Kanal, Boss Abriss-Bernd, Weltkarte, Titel/Optionen, Musik-Tracker, Speicherstand (localStorage) | Welt 1 in 15 Minuten durchspielbar, Boss mit 3 Phasen |
| **M4** | Content-Ausbau | Welt 2 + 3, Fahrzeuge (Trabi-Slug, Bus), 4 weitere Bros, Drohnen, Scharfschützen, Heli | 3 Welten, 8 Bros |
| **M5** | Polish & Release | Welt 4 + 5, Finale, Balancing, Performance-Pass, Controller-Remapping-UI, Desktop-Build (Tauri), itch.io/Steam-Deck-Test, Trailer-GIFs | Release-Kandidat |

Optional/parallel: In-Game-Level-Editor, Online-Koop (bewusst **nicht** vor M5, lokal ist der Kern).

## 6. Arbeitsweise pro Session

```bash
npm install          # einmalig
npm run assets       # Sprites/Atlanten generieren (deterministisch, eingecheckt)
npm run dev          # http://127.0.0.1:5173 – Hot Reload
npm run check        # assets + typecheck + build (das läuft auch in CI)
npm run preview & CHROMIUM_PATH=/opt/pw-browsers/chromium npm run shot   # Smoke-Test, Screenshots nach docs/screenshots
```

- Jede Session: Branch `claude/...` → Feature → `npm run check` → Screenshot(s) → Commit mit klarer Botschaft → Push.
- Balancing-Änderungen nur in `characters.ts`/`constants.ts`, mit Begründung im Commit.
- Neue Grafik immer zuerst auf dem Kontaktbogen (`docs/art/preview.png`) prüfen, dann im Spiel.
- URL-Parameter für Tests: `?auto=1` (P1 automatisch beitreten), `?blut=0` (Konfetti). `F1` zeigt Hitboxen und Debug-Zahlen.

## 7. Offene Entscheidungen (deine Stimme zählt)

1. **Blut oder Konfetti als Default?** (aktuell Blut, Konfetti per `?blut=0`)
2. **Steuerung P2 auf der Tastatur:** `, . /` + `M` oder lieber Numpad-only?
3. **Zielplattform für Release:** Browser/itch.io zuerst, Desktop-Wrapper später – ok?
4. **AutoSprite freischalten:** Soll ich `autosprite.io` (und ggf. `api.autosprite.io`) in den Netzwerk-Einstellungen der Cloud-Umgebung als erlaubte Domain eintragen lassen, damit ich die MCP/REST-Integration direkt bauen kann?
5. **Bro-Roster:** Passen die 4 Start-Bros, oder tauschen wir einen (z. B. Technodore gegen eine Mauerpark-Karaoke-Sängerin mit Mikro-Schockwelle)?
6. **Name der Bösewichte:** „Beton & Rendite AG" / „Baron von Beton" – Vorschläge willkommen.

## 8. Risiken & Gegenmaßnahmen

| Risiko | Gegenmaßnahme |
| --- | --- |
| Koop-Kamera frustriert (Nachzügler) | Bildrand blockt statt zu töten; optionaler Nachzügler-Teleport nach 3 s (M2) |
| Performance bei vielen Partikeln/Tiles | Partikel-Cap 1500, nur sichtbare Tiles zeichnen; bei Bedarf Tile-Chunks cachen |
| KI-Sprites passen stilistisch nicht | Pixelize-Schritt + Palette erzwingen, Spritegen-Figuren als Maßstab |
| Umfang (5 Welten, 8 Bros) | Welt 1 komplett vor Breite; Bausteine wiederverwenden; jedes Level aus ASCII |
| Netzwerk-Policy blockiert Tools | Alles Nötige ist lokal (Node, Chromium); externe Dienste bleiben optional |
