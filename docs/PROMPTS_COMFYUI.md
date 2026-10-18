# ComfyUI-Prompt-Buch (Parallax-Hintergründe & Stimmung)

ComfyUI erzeugt die gemalten, weiten Hintergrundebenen; der Pixelize-Schritt bringt sie in die Projektpalette. Workflow-Vorlage: `Tools/comfyui/workflows/parallax_layer.json` (API-Format, Platzhalter `<...>` ersetzen; läuft über den ComfyUI-MCP von deinem Rechner).

## Ebenenschema pro Welt

| Ebene | Faktor | Größe (vor Pixelize → nach) | Inhalt |
| --- | --- | --- | --- |
| Sky | 0.0 | 1920×1080 → 960×540 | Himmel, Mond, Sterne, Wolkenschleier |
| Far | 0.15 | 1920×560 → 960×280 | Skyline-Silhouette mit Wahrzeichen |
| Mid | 0.4 | 1920×480 → 960×240 | Häuserzeile mit Fenstern, Neon, Kränen |
| Near (optional) | 0.7 | 1920×320 → 960×160 | Zäune, Laternen, Büsche, Graffitiwand (mit Alpha) |

Alle Ebenen außer Sky müssen **horizontal kachelbar** sein (`--tile-check` < 20).

## Stilbasis (an jeden Prompt anhängen)
> painted 2D side-scroller background layer, flat orthographic side view, night in Berlin, limited palette (navy, slate, sodium orange, neon pink, neon cyan, BVG yellow), strong readable silhouettes, no characters, no text, seamless horizontally tileable, crisp edges, pixel art friendly.

Negativ:
> blurry, photo, perspective, fisheye, text, watermark, people, vehicles in motion, gradient banding, frame, border.

## Welt 1 – Kreuzberg

- **Sky:** deep navy night sky, thin clouds lit from below by orange city glow, crescent moon upper right, sparse stars.
- **Far:** silhouette skyline with Fernsehturm (TV tower with sphere) center-right, church spire left, construction cranes with red warning lights, scattered lit windows.
- **Mid:** row of Altbau facades 4–5 floors, warm lit windows, ground floor Spätis with neon signs (pink/cyan), satellite dishes, graffiti patches, overhead tram wires.
- **Near:** chain-link fence with posters, street lamp with sodium light cone, bushes, trash bins, bicycle leaning on wall.

## Welt 2 – U-Bahn / Alexanderplatz
- **Far:** tunnel vanishing in darkness, tiled walls with yellow line (U-Bahn style), cable ducts, emergency lights.
- **Mid:** platform pillars, benches, vending machines, route maps as colored rectangles, flickering fluorescent tubes.
- Alex-Level: Weltzeituhr silhouette, Fernsehturm huge in background, 60s tiled facades.

## Welt 3 – Club-Bunker
- **Far:** industrial ruin, broken windows, chimney stacks, fog, pink/cyan laser beams.
- **Mid:** concrete walls with speaker stacks, cables, strobe flashes (export two variants: on/off), queue barriers.
- **Near:** fog layer with alpha, bottles, confetti on floor.

## Welt 4 – Tempelhofer Feld
- **Sky:** dusk gradient purple to orange, kites high up.
- **Far:** hangar silhouette curve, radar tower, distant skyline.
- **Mid:** runway lights, grass, zeppelin mooring mast, flak searchlights.

## Welt 5 – Regierungsviertel & Fernsehturm
- **Far:** glass government buildings, dome silhouette, Spree river reflections.
- **Mid:** construction site with cranes, concrete mixers, „Rendite"-billboards (abstract shapes, no real text).
- Finale: inside the TV-tower sphere – panorama windows with the whole city below (Sky layer as rotating panorama).

## Nachbearbeitung
```bash
python Tools/pixelize/pixelize.py far_raw.png Content/Import/ComfyUI/w1_far.png --width 960 --dither --tile-check
python Tools/pixelize/pixelize.py mid_raw.png Content/Import/ComfyUI/w1_mid.png --width 960 --dither --tile-check
```
Danach im Editor `be_import_sprites.py` (Hintergrund-Abschnitt) oder per MCP als Sprite importieren und einer `ABEParallaxLayer` zuweisen.
