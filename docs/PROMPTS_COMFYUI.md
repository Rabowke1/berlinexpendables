# ComfyUI-Prompt-Buch (Parallax-Hintergründe & Stimmung)

ComfyUI erzeugt die gemalten, weiten Hintergrundebenen; der Pixelize-Schritt bringt sie in die Projektpalette. Workflow-Vorlage: `Tools/comfyui/workflows/parallax_layer.json` (API-Format, Platzhalter `<...>` ersetzen; läuft über den ComfyUI-MCP von deinem Rechner).

## Ebenenschema pro Welt

| Ebene | Faktor | Größe (vor Pixelize → nach) | Inhalt |
| --- | --- | --- | --- |
| Sky | 0.0 | 1920×1080 → 1440×810 | Himmel, Mond, Sterne, Wolkenschleier |
| Far | 0.15 | 1920×560 → 1440×420 | Skyline-Silhouette mit Wahrzeichen |
| Mid | 0.4 | 1920×480 → 1440×360 | Häuserzeile mit Fenstern, Neon, Kränen |
| Near (optional) | 0.7 | 1920×320 → 1440×240 | Zäune, Laternen, Büsche, Graffitiwand (mit Alpha) |

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

## Getesteter Ablauf (19.10., RTX 4090, ComfyUI 0.42)

Ergebnisse für Welt 1 liegen unter `Content/Import/ComfyUI/` (`w1_sky.png` 1440×816, `w1_far.png` 1440×432, `w1_mid.png` 1440×360; Rohbilder in `raw/`). Das Importskript nimmt sie automatisch statt der Platzhalter.

| Schritt | Werkzeug | Erkenntnis |
| --- | --- | --- |
| Rendern | `Tools/comfyui/run_comfy.ps1 -Workflow Tools/comfyui/workflows/w1_mid_flux.json -OutDir <ordner>` | **Flux dev fp8** (`flux1-dev-fp8.safetensors`, 20 Schritte, euler/simple, cfg 1, FluxGuidance 3.5, 1920 breit) liefert in 15–30 s die beste Komposition: Fernsehturm, Kirchturm, Kräne, Altbau-Fassaden mit Balkonen und Neon-Spätis. Negativ-Prompt wirkt bei Flux nicht (ConditioningZeroOut). |
| Stil | LoRA `flux_retro_pixel.safetensors` (Trigger `Retro Pixel`, Stärke 0,85) | Für **Skyline und Himmel** besser: Sterne, Wolkenbänder, grobe Pixelsilhouetten. Für die **Häuserzeile** reicht Flux ohne LoRA, dort ist der Detailgrad wichtiger. `flux_ume_modern_pixelart.safetensors` (Trigger `umempart`) und `sdxl_pixel-art-xl.safetensors` liegen als Alternativen in `E:\temp\Data\Models\Lora`. |
| Naht | `python Tools/comfyui/seamfix.py raw.png seam.png --band 320 [--lora flux_retro_pixel.safetensors]` | Rollt das Bild um die halbe Breite und malt den mittleren Streifen per Flux-Inpainting neu. Nahtdifferenz der Häuserzeile von 45 auf 12,7, Skyline von 11 auf 10,7; visuell unsichtbar. |
| Pixelize | `pixelize.py raw.png out.png --width 1440 --preset bg --dither` | **Preset `bg` ist Pflicht** (ohne helles Blau, Grün, Erde, Haut), sonst kippt der Himmelsverlauf in Grün/Blau. Dither erzeugt den 16-Bit-Himmel. Für den Himmel ohne `red`, sonst wird der Mond rot (`--colors ink,night,navy,slate,concrete,ash,fog,gold,orange,steelDark,steel`). |
| Text | – | Flux malt Pseudo-Schriftzüge auf Neonschilder; nach dem Pixelize sind sie unleserliche Blöcke. Für Nahebenen später per Inpainting durch Symbole ersetzen. |

## Nachbearbeitung
```bash
python Tools/pixelize/pixelize.py far_raw.png Content/Import/ComfyUI/w1_far.png --width 1440 --dither --tile-check
python Tools/pixelize/pixelize.py mid_raw.png Content/Import/ComfyUI/w1_mid.png --width 1440 --dither --tile-check
```
Danach im Editor `be_import_sprites.py` (Hintergrund-Abschnitt) oder per MCP als Sprite importieren und einer `ABEParallaxLayer` zuweisen.
