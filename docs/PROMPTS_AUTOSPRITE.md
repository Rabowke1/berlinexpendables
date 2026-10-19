# AutoSprite-Prompt-Buch (Charaktere & Gegner)

AutoSprite liefert aus einem Referenzbild oder Preset die Animationen als Sprite-Sheet + Atlas-JSON. Diese Datei hält fest, **was** wir pro Figur bestellen, damit alle Exporte zur Engine passen. Der MCP-Server von AutoSprite ist auf deinem Rechner konfiguriert (siehe `docs/MCP_RUNBOOK.md`), hier ausgeführt von der Claude Code CLI.

## Technische Vorgaben für jeden Export

| Parameter | Wert | Grund |
| --- | --- | --- |
| Frame | **96×96 px**, Figur ≈ 66–72 px hoch, Füße auf y = 93 | 3× der Platzhalter, 48-px-Tiles, mehr Detail als Metal Slug |
| Blickrichtung | **rechts** | links wird gespiegelt |
| Animationen (Pflicht) | idle (4), run (8), jump (1), fall (1), crouch (1), melee (3), death (3–4) | Namensschema `<anim>_h_<n>` |
| Zielvarianten | idle/run/jump/fall zusätzlich **aim up** (`_u`) und **aim down** (`_d`) | Schießen nach oben/unten |
| Waffe | **nicht** in der Hand zeichnen (leere Hand, Arm ausgestreckt) | Waffen sind eigene Sprites am Hand-Socket |
| Hintergrund | transparent | – |
| Stil | Pixel-Art, 1-px-Outline, harte Kanten, max. 16 Farben | danach `pixelize.py` (Palette, Outline) |
| Export | PNG-Sheet + JSON (TexturePacker/Phaser-Format) | `tools/import-atlas` (M1) → `Content/Import/AutoSprite/<id>/` |

Nach dem Export immer:
```bash
python Tools/pixelize/pixelize.py <sheet>.png Content/Import/AutoSprite/<id>.png --grid 96 --outline --normals Content/Import/AutoSprite/<id>_n.png
```

## Stilbasis (an jeden Prompt anhängen)
> 2D pixel art game sprite, side-scrolling run and gun (Metal Slug meets Broforce), 96x96 frame, chunky readable silhouette, 1 pixel dark outline, limited palette, night-time Berlin colors (navy, concrete grey, neon pink, neon cyan, sodium orange, BVG yellow), no anti-aliasing, transparent background, facing right, empty extended right hand ready to hold a weapon.

## Die Bros

| Id | Referenzbeschreibung für AutoSprite | Akzentfarbe |
| --- | --- | --- |
| `willi` | Stämmiger Imbissbudenbesitzer, rote Basecap mit Logo, braunes Schnauzbart, weißes Unterhemd unter cremefarbener Schürze mit Ketchupspritzern, dunkelblaue Hose, braune Arbeitsschuhe | #e23b3b |
| `doro` | Große Türsteherin, pinker Irokese, schwarze Tanktop und schwarze Jeans, Sonnenbrille, Tattoo am Arm (Cyan), schwere Boots, Kaugummi | #ff3fa4 |
| `berti` | Busfahrer mittleren Alters, gelbe BVG-Schirmmütze, dunkelblaue Uniformjacke mit gelben Streifen, gelbe Krawatte, Schnauzbart, dunkle Haut, schwarze Schuhe | #ffcf5c |
| `sven` | Schlaksiger Späti-Besitzer, lila Hoodie mit Kapuze auf, blonde Strähne, graue Jogginghose, weiße Sneaker, müder Blick, Flaschenöffner am Band | #38f0e6 |
| `tanja` (W2) | Taxifahrerin, beige Taxi-Jacke, Headset, rote Locken, Jeans | #d9b48f |
| `doener` (W3) | Dönermeister, weiße Kochjacke, Schnauzbart, zwei Messer, Papierhut | #ffffff |
| `paul` (W4) | Pfandsammler, Parka mit Flaschensack, Mütze, Einkaufswagen (separates Prop) | #3f8a3a |
| `sascha` (W5) | Mopedkurier, orangefarbene Kurierjacke, Jethelm mit Visier, Umhängetasche | #ff8c2a |

## Gegner

| Id | Beschreibung | Besonderheit |
| --- | --- | --- |
| `mook` | Anzug-Scherge: schwarzer Anzug, weißes Hemd, rote Krawatte, Sonnenbrille, gegeltes Haar | zusätzlich `panic` (4 Frames, Arme hoch, rennend) |
| `security` | Security: schwarze Kleidung, gelbe Warnweste, Glatze, Sonnenbrille, Funkgerät | 2 HP, `panic` |
| `koffer` | Koffer-Bomber: gehetzter Anzugträger mit Aktenkoffer, Schweißperlen | nur `run`, `explode` (4 Frames) |
| `hund` | Wachhund, dunkelgrau, Stachelhalsband | 72×48 Frame, `run`, `jump`, `bite` |
| `sniper` | Scharfschütze in Baustellenweste mit Gewehr auf Kran | `idle`, `aim`, `shoot` |
| `captive` | Gefangener: zerzaust, cremefarbenes Hemd, braune Hose, dankbar winkend | `idle`, `run`, `cheer` |

## Bosse (M3+)

| Boss | Größe | Beschreibung |
| --- | --- | --- |
| Abriss-Bernd | 384×288 | Riesiger gelber Bagger mit Abrissbirne, Fahrerkabine mit Anzugträger, Schlagseite, rostige Ketten |
| Zug der Zukunft | 768×240 | Gepanzerte U-Bahn, Werbebanner „Rendite", Geschütztürme auf dem Dach |
| Soundsystem 3000 | 336×384 | Lautsprecherturm auf Mech-Beinen, Strobo-Auge, Subwoofer-Mund |
| Zeppelin „Rendite Eins" | 960×384 | Werbe-Zeppelin mit Gondel-Geschützen |
| Baron von Beton | 480×480 | Kran-Mech mit Betonmischer-Torso, Monokel, Zylinder |

Bosse werden in Teilen bestellt (Körper, bewegliche Glieder, Geschütze) und in UE als Sprite-Hierarchie animiert.
