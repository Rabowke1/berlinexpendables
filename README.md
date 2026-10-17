# Berlin Expendables

4-Spieler-Lokal-Koop-Run'n'Gun in echter 2D-Pixel-Art: **Broforce**-Chaos (zerstörbare Welt, Bro-Wechsel) trifft **Metal-Slug**-Spektakel (Waffen-Pickups, Fahrzeuge, Riesen-Bosse) – in einem überzeichneten Berlin.

![Koop-Screenshot](docs/screenshots/04-coop.png)

| Dokument | Inhalt |
| --- | --- |
| [docs/KONZEPT.md](docs/KONZEPT.md) | Game-Design: Säulen, Bros, Gegner, Welten, Regeln |
| [docs/ABLAUF.md](docs/ABLAUF.md) | Technik, Architektur, Asset-Pipeline (Spritegen + AutoSprite), Levelformat, Meilensteine, offene Fragen |
| [docs/STYLEGUIDE.md](docs/STYLEGUIDE.md) | Pixel-Art-Regeln und Palette „Berlin Nacht" |

## Schnellstart

```bash
npm install
npm run assets     # Sprites & Atlanten generieren (prozedural, deterministisch)
npm run dev        # http://127.0.0.1:5173
```

Weitere Befehle: `npm run check` (Assets + Typecheck + Build), `npm run preview` + `npm run shot` (Playwright-Smoke-Test mit Screenshots).

## Steuerung

| Aktion | Tastatur P1 | Tastatur P2 | Gamepad |
| --- | --- | --- | --- |
| Laufen / Zielen hoch / Ducken | W A S D | Pfeile | Stick / D-Pad |
| Feuer | J | , | X / RT |
| Sprung (und Beitreten) | K | . | A |
| Granate | L | / | Y / RB |
| Nahkampf | U | M | B / LB |

Beitreten jederzeit per Sprung/Start auf einem freien Gerät. `F1` = Debug, `?blut=0` = Konfetti-Modus.

## Stand (Meilenstein 0)

- 4 spielbare Bros mit eigenen Waffen (Ketchup-Pumpgun, Bass-Kanone, Entwerter-MP, Pfandflaschen), Granaten, Ducken, automatischer Nahkampf
- Zerstörbares Tile-Terrain, Explosionen, Kettenreaktionen mit Gasflaschen, Kisten mit Items
- Gegner-KI (Patrouille, Alarm, Salven, Panik), Käfige mit Gefangenen (+1 Leben, Item, Bro-Wechsel)
- Metal-Slug-Pickups H/R/F/L/B, Fluchttrabi als Levelende, HUD, Drop-in-Koop für 4, Game Over / Sieg
- Prozeduraler Pixel-Art-Generator (`tools/spritegen`) für alle Assets, Kontaktbogen unter `docs/art/preview.png`

![Kontaktbogen](docs/art/preview.png)
