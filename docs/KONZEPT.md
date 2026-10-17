# BERLIN EXPENDABLES – Spielkonzept (GDD v0.1)

> 4-Spieler-Lokal-Koop-Run'n'Gun in echter 2D-Pixel-Art.
> Broforce liefert das Chaos (zerstörbare Welt, Bro-Wechsel, Koop für vier),
> Metal Slug liefert das Spektakel (Waffen-Pickups, Fahrzeuge, riesige Maschinen-Bosse, Gegner-Comedy).
> Schauplatz: ein überzeichnetes Berlin, Bezirk für Bezirk.

---

## 1. Elevator Pitch

Ein fiktives Konsortium („Beton & Rendite AG", angeführt von **Baron von Beton**) hat Berlin übernommen:
Spätis geschlossen, Clubs zu Co-Working-Spaces umgebaut, die BVG privatisiert, Currywurst verboten.
Die **Berlin Expendables** – ein Haufen Berliner Originale – schlagen zurück.
Vier Freunde auf einer Couch, vier Controller, eine komplett zerlegbare Stadt.

## 2. Design-Säulen

| Säule | Bedeutung für jede Entscheidung |
| --- | --- |
| **Alles ist zerstörbar** | Erde, Ziegel, Kisten, Autos, Fassaden. Jede Waffe gräbt, jede Explosion hinterlässt Krater. Nur Stahl hält. |
| **Koop-Chaos für vier** | Gemeinsamer Lebenspool, geteilte Kamera, Freundbeschuss durch Explosionen. Lachen am Sofa ist das Ziel, nicht Perfektion. |
| **Bros mit Berliner Schnauze** | Jede Figur ist ein Berlin-Klischee mit eigener Waffe, Special, Spruch und Spielgefühl. Beim Tod oder bei Rettung wechselt man den Bro. |
| **Metal-Slug-Spektakel** | Power-Waffen aus Kisten (H, R, F, L), Fahrzeuge zum Einsteigen, Bosse so groß wie der Bildschirm, Gegner, die essen, pennen und in Panik rennen. |
| **Echte Pixel-Art** | Feste Auflösung 480×270, 16-px-Tiles, eine Palette, 1-px-Outlines, keine Rotation/Skalierung von Sprites außer in 90°-Schritten. Siehe `STYLEGUIDE.md`. |

## 3. Was kommt woher?

| Element | Broforce | Metal Slug | Berlin Expendables |
| --- | --- | --- | --- |
| Zerstörbares Terrain | ✅ | – | ✅ Tile-HP, Krater, Graben per Nahkampf |
| Spieleranzahl | 4 | 2 | **4** lokal (Tastatur ×2 + Gamepads ×4, frei kombinierbar) |
| Figurenwechsel | bei Rettung/Tod | – | ✅ bei Rettung und Respawn |
| Ein Treffer = Tod | ✅ | ✅ | ✅ (Option „Arcade": 3 HP) |
| Waffen-Pickups | – | ✅ H/R/S/F/L | ✅ H/R/F/L + Granaten-Nachschub, zeitlich/munitionsbegrenzt über der Signaturwaffe |
| Fahrzeuge | selten | ✅ Slug | ✅ Trabi-Slug, BVG-Bus, Lastenrad-Mech, Schwalbe (ab M4) |
| Ducken / Nahkampf | – / Messer | ✅ / automatisches Messer | ✅ Ducken, automatischer Nahkampf bei Kontakt + Taste |
| Riesen-Bosse | ✅ | ✅✅ | ✅ Ein Maschinen-Boss pro Bezirk |
| Gegner-Comedy | Panik, brennen | essen, sonnen, fliehen | ✅ Panik, Döner-Pause, Handy-Zocken, „Ick hab nüscht gesehen" |
| Score | – | ✅ | ✅ Punkte, Combo-Multiplikator, MVP am Levelende |

## 4. Story & Setting

- **Gegner:** fiktives Konsortium „Beton & Rendite AG". Fußvolk sind *Anzug-Schergen* (Sonnenbrille, Krawatte), *Security* (Warnweste, Gewehr), *Drohnenpiloten*, *Koffer-Bomber* (rennen mit Aktenkoffer auf dich zu), *Wachhunde*, *Scharfschützen auf Kränen*, *Bagger*, *Immobilien-Helis*.
- **Keine realen Personen, Firmen oder Marken.** Humor über Klischees und Berliner Alltag, nicht über echte Leute.
- **Ton:** Overacting, Actionfilm-Parodie, Berliner Schnauze. Einblendungen wie „KIEZ BEFREIT!", „MIT ALLES UND SCHARF!".
- **Stationen (Welten):** Kreuzberg → U-Bahn/Alexanderplatz → Club-Bunker (Friedrichshain) → Tempelhofer Feld → Regierungsviertel & Fernsehturm-Finale.

## 5. Core Loop & Regeln

1. **Nach rechts kämpfen** (gelegentlich runter/hoch), Terrain zerlegen, Gegner ausschalten.
2. **Gefangene befreien** (Käfige kaputtschießen): +1 Leben, Item fällt raus, der Retter wechselt zum nächsten Bro (Broforce-Regel).
3. **Checkpoints** (Späti-Schilder) setzen den Respawn-Punkt.
4. **Levelende:** alle lebenden Spieler zum **Fluchttrabi** – er fährt hupend davon, Punkte-Tafel, MVP.

**Leben & Tod**
- Gemeinsamer Lebenspool (Start 5). Tote Spieler respawnen nach kurzer Zeit beim Team, sofern Leben übrig sind, als **neuer Bro** mit 2 s Unverwundbarkeit.
- Ein Treffer tötet (Default). Option „Arcade-Modus": 3 Treffer, Herzen im HUD.
- Eigene Explosionen sind tödlich. Kugeln der Mitspieler nicht (Option „Freundbeschuss").
- Game Over, wenn niemand mehr lebt und der Pool leer ist. Neustart am letzten Checkpoint mit frischem Pool (Casual) oder Levelanfang (Normal).

**Kamera**
- Folgt der Bounding-Box aller lebenden Spieler. Niemand kann aus dem Bild laufen: der Bildrand blockt. Wer hängen bleibt, zwingt das Team zu warten – ein bewusster Koop-Reibungspunkt wie in Broforce.
- Screen-Shake und Hitstop bei Explosionen und Kills.

**Skalierung**
- Pro zusätzlichem Spieler +25 % Gegner in markierten Spawn-Gruppen; Bosse +30 % HP pro Spieler.

## 6. Die Bros

Alle Bros teilen ein Rig (32×32 Frame, 10×22 Kollisionsbox). Unterschiede: Palette, Kopfbedeckung, Props, **Signaturwaffe**, **Special** (Granaten-Slot, begrenzt, Nachschub über Kisten/Käfige) und **Nahkampf**.

| Bro | Herkunft | Signaturwaffe (Primär) | Special (L / Y) | Nahkampf | Spielgefühl |
| --- | --- | --- | --- | --- | --- |
| **Currywurst Willi** | Imbissbudenbesitzer, Kreuzberg | *Ketchup-Pumpgun*: 5 Schrot-Pellets, kurze Reichweite, Rückstoß. In der Luft nach unten schießen = „Ketchup-Jump" | *Pommes-Granate*: Splittergranate | Bratwurstzange | Nahkampf-Brecher, Terrain-Bagger |
| **Technodore (Doro)** | Türsteherin, Club-Bunker | *Bass-Kanone*: langsame Schallwelle, durchschlägt 3 Gegner, bricht Ziegel | *Strobo-Flash*: betäubt alle Gegner im Bild 3 s | Türsteher-Schubs (Knockback) | Crowd-Control, Raumkontrolle |
| **BVG-Berti** | Busfahrer | *Entwerter-MP*: schnelles Dauerfeuer, leichte Streuung | *Linienbus*: ein Bus pflügt durchs Bild, zerstört Terrain und Gegner | Haltestangen-Schlag | Allrounder, Dauerdruck |
| **Späti-Sven** | Späti-Besitzer | *Pfandflaschen*: Bogenwurf, explodiert klein beim Aufprall | *Bierkasten*: zerspringt in 6 Flaschen-Explosionen | Kronkorken-Schnipser | Indirekter Beschuss, Flächenschaden |
| *Taxi-Tanja* (Unlock W2) | Taxifahrerin | *Hupen-Schockwelle* | *Taxi-Rammbock* | Türknall | Mobilität |
| *Dr. Döner* (Unlock W3) | Dönermeister | *Messerwirbel* (reiner Nahkampf, reflektiert Kugeln) | *Knoblauchsoßen-Teppich* (Flächen-DoT) | – | Risiko/Belohnung |
| *Pfand-Paul* (Unlock W4) | Flaschensammler | *Einkaufswagen-Granatwerfer* | *Pfandautomat* (saugt Gegner an) | Wagen-Ramme | Artillerie |
| *Schwalben-Sascha* (Unlock W5) | Mopedkurier | *Zwille mit Kugellagern* | *Schwalbe-Dash* (unverwundbar, zerstört Terrain) | Helmstoß | Speed |

**Nahkampf:** Bei Gegnerkontakt schlägt der Bro automatisch zu (Metal-Slug-Messer), zusätzlich per Taste. Nahkampf gräbt auch Terrain (Broforce).

## 7. Metal-Slug-Schicht

**Waffen-Pickups** (leuchtende Boxen mit Buchstabe, aus Kisten und Käfigen):

| Box | Waffe | Munition | Charakter |
| --- | --- | --- | --- |
| **H** | Heavy Machine Gun | 200 | Dauerfeuer, 2 Schaden, frisst Ziegel |
| **R** | Rocket Launcher | 15 | Große Explosionen |
| **F** | Flame Shot | 150 | Kurze Reichweite, durchdringt, setzt Gegner in Panik |
| **L** | Laser Shot | 200 | Durchgehender Strahl, schnell |
| **B** | Bomben | +5 Granaten | – |
| 🌭 | Currywurst | +1 Leben | – |

Pickup überschreibt die Signaturwaffe, bis die Munition leer ist („LEER!") oder der Bro stirbt.

**Fahrzeuge (ab Meilenstein 4):** *Trabi-Slug* (Panzer-Trabi mit Hupe-Kanone, 3 Treffer, Ausstieg per Sprung), *BVG-Doppeldecker* (zwei Spieler: Fahrer + Dachschütze), *Lastenrad-Mech* (Sprunggewaltig), *Schwalbe* (schnell, zerbrechlich). Fahrzeuge haben eigene HP und explodieren spektakulär.

**Ducken & Kriechen:** Ducken (unten) halbiert die Trefferfläche, langsames Kriechen; durch Plattformen fallen mit unten + Sprung.

**Score & Combos:** Kills in schneller Folge erhöhen einen Multiplikator (×2 bis ×8). Levelende zeigt Punkte, Rettungen, MVP (meiste Kills) und „Kollateralschaden" (zerstörte Tiles).

**Gegner-Comedy:** Schergen machen Döner-Pause, zocken am Handy, schlafen im Stehen; wer einen Kollegen sterben sieht, rennt mit erhobenen Armen in Panik (schon drin). Brennende Gegner rennen und stecken andere an.

## 8. Gegner

| Gegner | Verhalten | Tod |
| --- | --- | --- |
| **Anzug-Scherge** | Patrouilliert, dreht an Kanten, Pistolenschuss bei Sichtkontakt (150 px) | 1 Treffer |
| **Security** | Wie Scherge, 3-Schuss-Salven, 2 HP, Warnweste | 2 Treffer |
| **Koffer-Bomber** | Rennt auf Spieler zu, Aktenkoffer explodiert | 1 Treffer (explodiert!) |
| **Drohnenpilot** | Steuert Lieferdrohne mit Bomben, selbst wehrlos | Pilot töten → Drohne stürzt ab |
| **Wachhund** | Schnell, springt, kein Fernkampf | 1 Treffer |
| **Scharfschütze** | Auf Kränen/Dächern, Laserpointer-Warnung 1 s | 1 Treffer |
| **Bagger** (Mini-Boss) | Schaufel-Schläge, überrollt Terrain | 20 Treffer / Fahrerkabine |
| **Immobilien-Heli** | Setzt Schergen ab, Raketen | 30 Treffer |

## 9. Welten & Bosse

| Welt | Bezirk | Optik | Level | Boss |
| --- | --- | --- | --- | --- |
| 1 | **Kreuzberg** | Hinterhöfe, Spätis, Graffiti, Landwehrkanal | 1-1 Hinterhof, 1-2 Dachlandschaft, 1-3 Kanal | **Abriss-Bernd**: Riesenbagger mit Abrissbirne |
| 2 | **U-Bahn / Alexanderplatz** | Tunnel, Bahnsteige, Fernsehturm im Hintergrund | 2-1 Bahnsteig, 2-2 Tunnel (fahrende Züge), 2-3 Alex | **Zug der Zukunft**: gepanzerte U-Bahn, Kampf auf dem Dach |
| 3 | **Club-Bunker (Friedrichshain)** | Industrieruine, Nebel, Strobo, Neon | 3-1 Schlange vor dem Club, 3-2 Dancefloor, 3-3 Keller | **Soundsystem 3000**: Lautsprecher-Mech, Bass-Wellen zerstören Boden |
| 4 | **Tempelhofer Feld** | Offenes Feld, Flak, Drachensteiger, Hangars | 4-1 Feld, 4-2 Rollbahn, 4-3 Hangar | **Zeppelin „Rendite Eins"** |
| 5 | **Regierungsviertel & Fernsehturm** | Glasfassaden, Spree, Kugel des Turms | 5-1 Spreeufer, 5-2 Baustelle, 5-3 Turmaufstieg | **Baron von Beton** im Kran-Mech, Finale auf der Turmkugel |

**Levelbausteine:** Grube mit Plattform, Gebäude mit Innenraum (Hintergrund-Tiles), Stahlbunker (unzerstörbar), Käfig-Nische, Fass-Kette, Kistenstapel mit Item, Scharfschützen-Posten, Checkpoint-Späti, Fluchttrabi. Levels sind ASCII-Dateien (siehe `ABLAUF.md`), später mit In-Game-Editor.

## 10. Steuerung

| Aktion | Tastatur P1 | Tastatur P2 | Gamepad |
| --- | --- | --- | --- |
| Laufen / Zielen hoch / Ducken | W A S D | Pfeile | Stick / D-Pad |
| Feuer | J | , (Numpad 1) | X / RT |
| Sprung (+ Beitreten) | K / Leertaste | . (Numpad 2) | A |
| Special / Granate | L | / (Numpad 3) | Y / RB |
| Nahkampf | U | M (Numpad 0) | B / LB |
| Beitreten / Pause | Enter | Shift rechts | Start |

Beitreten jederzeit per Sprung/Start auf einem freien Gerät (Drop-in). Bis zu 4 Spieler in jeder Kombination.

## 11. Audio

- **SFX:** prozeduraler Retro-Synth (WebAudio), kein Sample nötig; pro Waffe eigener Klang, Explosionen mit Sub-Bass.
- **Musik (ab M3):** Techno-Chiptune per kleinem Tracker (Pattern pro Welt), Boss-Themes schneller; Club-Welt mit Sidechain-Pumpen.
- **Stimmen:** kurze, pitch-geshiftete Rufe („Zurückbleiben!"), optional.

## 12. HUD & UI

- Pro Spieler Karte oben: Porträt, Farbe, Bro-Name, Granaten, Pickup-Munition, Respawn-Countdown.
- Mitte: Lebenspool. Rechts: Punkte. Spawn-Banner mit Name und Spruch.
- Join-Hinweise unten, Titel-Overlay solange niemand spielt.
- Menüs (ab M3): Titel, Join-Screen mit Bro-Vorschau, Weltkarte (Berlin-Stadtplan), Optionen, Pause.

## 13. Optionen & Zugänglichkeit

- Blut ↔ **Konfetti-Modus** (URL `?blut=0`, später Option).
- Hardcore (1 HP) ↔ Arcade (3 HP).
- Freundbeschuss an/aus, Screen-Shake-Stärke, Bildschirm-Blitze reduzieren (Strobo!).
- Komplett neu belegbare Tasten, Controller-Deadzone.

## 14. Technik (Kurzfassung)

TypeScript + Vite, eigener Canvas-2D-Renderer mit 480×270-Pixelbuffer und Integer-Skalierung, fester 60-Hz-Step, Gamepad-API. Assets kommen aus einem **eigenen prozeduralen Pixel-Art-Generator** (`tools/spritegen`) und optional aus **AutoSprite** über einen Import-Schritt, der echte Pixelraster erzwingt. Details, Meilensteine und Pipeline: `docs/ABLAUF.md`.
