# MCP-Runbook: So geht es auf deinem Rechner weiter

Dieses Repo wurde in einer Cloud-Session vorbereitet, die **keinen** Zugriff auf Unreal, AutoSprite oder ComfyUI hat. Alles, was einen laufenden Editor oder die beiden Generatoren braucht, passiert in einer Claude-Code-Session **auf deinem PC**. Dieses Dokument ist die Checkliste dafür.

## 1. Session in die lokale CLI holen

Voraussetzungen: Claude Code CLI installiert und mit demselben claude.ai-Konto angemeldet (`claude auth login`), Git LFS installiert (`git lfs install`), UE 5.8 installiert.

```bash
git clone https://github.com/Rabowke1/berlinexpendables.git
cd berlinexpendables
claude --teleport session_01AGcML3kED5H3cSWB9eUL84
```

`--teleport` holt diese Cloud-Session samt Verlauf in das Terminal und checkt den Branch `claude/affectionate-knuth-yd502g` aus. Alternativ ohne Session-ID: `claude --teleport` öffnet eine Auswahl; oder in der Web-Oberfläche **Open in → Terminal**. Der Arbeitsbaum muss sauber sein. Die lokale Kopie ist danach eigenständig; die Cloud-Session sieht lokale Änderungen nicht.

Umgekehrt (Cloud-App steuert deinen PC): `claude remote-control` im Projektordner starten, dann erscheint die Session in der Claude-Code-App.

## 2. Unreal-MCP verbinden

1. `BerlinExpendables.uproject` in UE 5.8 öffnen. Beim ersten Start fragt der Editor, ob das C++-Modul kompiliert werden soll: **Ja**. (Der Code wurde ohne Compiler geschrieben; Fehler werden in dieser ersten Runde per Live Coding behoben.)
2. Die Plugins `ModelContextProtocol` und `AllToolsets` sind in der `.uproject` bereits aktiviert. Falls der Editor sie nicht findet, stammen sie aus dem Engine-Ordner `Engine/Plugins/Experimental/ModelContextProtocol` deiner Installation.
3. MCP-Server starten: Editor-Konsole (`~`) → `ModelContextProtocol.StartServer`. Für Autostart in `Saved/Config/<Platform>Editor/EditorPerProjectUserSettings.ini`:
   ```ini
   [/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]
   bAutoStartServer=True
   ```
4. Die `.mcp.json` im Repo zeigt auf `http://127.0.0.1:8000/mcp`. Falls du Port/Pfad geändert hast: Konsole `ModelContextProtocol.GenerateClientConfig ClaudeCode` schreibt sie neu.
5. Im Terminal `claude` starten; beim ersten Mal die Projekt-MCP-Server bestätigen. Prüfen mit `/mcp` (Häkchen bei `unreal-mcp`) oder `claude mcp list`.

## 3. AutoSprite- und ComfyUI-MCP eintragen

Die genauen Server-URLs/Kommandos stehen in deinen Konten bzw. ComfyUI-Setups; so werden sie projektweit (committed in `.mcp.json`) registriert:

```bash
# AutoSprite (Remote-MCP mit API-Key; URL laut AutoSprite-Dashboard)
claude mcp add --scope project --transport http autosprite https://<autosprite-mcp-url> \
  --header "Authorization: Bearer ${AUTOSPRITE_API_KEY}"

# ComfyUI (lokaler MCP-Server, der an http://127.0.0.1:8188 spricht; Paketname laut deinem ComfyUI-MCP)
claude mcp add --scope project --transport stdio --env COMFYUI_URL=http://127.0.0.1:8188 comfyui \
  -- npx -y <comfyui-mcp-paket>
```

`${AUTOSPRITE_API_KEY}` bleibt als Platzhalter in der Datei und wird aus deiner Umgebung expandiert, so landet kein Schlüssel im Repo. Nach Änderungen an `.mcp.json` die Session neu starten.

## 4. Erste Editor-Session (Reihenfolge)

Jeder Punkt ist eine Anweisung an die lokale Claude-Session; die Toolsets werden mit `list_toolsets` / `describe_toolset` entdeckt und per `call_tool` aufgerufen.

1. **Kompilieren**: `LiveCodingToolset.CompileLiveCoding` (oder der Editor-Dialog beim Öffnen). Fehler lesen, beheben, erneut kompilieren, bis grün.
2. **Assets bauen**: Python-Toolset → `py be_build_all.py` ausführen (oder Editor-Menü *Tools → Execute Python Script*). Erzeugt unter `/Game/BE` Texturen (Nearest, keine Mips), Sprites mit Pivots und Hand-/Muzzle-Sockets, Flipbooks, Charakter-DataAssets, Waffen- und Sprite-Bibliothek, Tileset, Materialien, das Level-Asset und die Map `/Game/Maps/L_Kreuzberg01`; trägt alles in `Config/DefaultGame.ini` ein.
3. **Projekteinstellungen prüfen**: *Project Settings → Game → Berlin Expendables – Gameplay*: StartLevel, PlayerCharacters (4), MookData, SecurityData, CaptiveData, Libraries, TileSet, TileMaterial gesetzt.
4. **PIE**: Map öffnen, Play. Erwartung: Himmel/Skyline/Häuser-Parallax, Tile-Welt, Titel-Overlay. `K` (Tastatur 1) oder `.` (Tastatur 2) oder Gamepad-A → Bro erscheint mit Banner. Schießen `J`, Granate `L`. Explosionen reißen Krater, Trümmerwürfel fliegen, Lichtblitze.
   - Tiles spiegelverkehrt? → Settings `bFlipTileU/bFlipTileV` umschalten.
   - Waffe sitzt daneben? → Hand-Socket-Offset in `be_import_sprites.py` (Pivot/Socket) prüfen.
5. **Niagara (3D-VFX)**: mit dem Niagara-Toolset Systeme anlegen und in den Settings (*VFX*) eintragen:
   - `NS_Explosion`: Sprite-Renderer mit `FB_fx_explosion`-Textur, GPU-Funken (Mesh-Renderer Würfel), Rauchring, Light-Renderer (orange, 0,3 s), User-Parameter `Tint` (LinearColor).
   - `NS_Debris`: Mesh-Partikel mit Kollision gegen Szene aus, Lebenszeit 1–2 s, `Tint`.
   - `NS_Blood`, `NS_Sparks`, `NS_Dust`, `NS_Smoke`: Sprite-Partikel, kleine Mengen, Pixel-Look (Nearest-Texturen).
   Alle Fallbacks (3D-Würfel, Punktlicht) bleiben aktiv, bis die Systeme eingetragen sind.
6. **AutoSprite**: ersten Bro bestellen (siehe `docs/PROMPTS_AUTOSPRITE.md`), Export nach `Content/Import/AutoSprite/<id>/`, `pixelize.py --grid 64 --outline --normals`, dann `BE_IMPORT_DIR` auf diesen Ordner setzen und `be_import_sprites.py` erneut laufen lassen.
7. **ComfyUI**: Welt-1-Ebenen mit `Tools/comfyui/workflows/parallax_layer.json` rendern, `pixelize.py --width 960 --dither --tile-check`, als Sprites importieren und den `ABEParallaxLayer`-Aktoren in der Map zuweisen.
8. **Commit**: `Content/**/*.uasset` und `*.umap` laufen über Git LFS (`.gitattributes`). Vor großen MCP-Änderungen und danach speichern (Strg+Shift+S).

## 5. Sicherheitsregeln bei MCP-Änderungen

- Vor Bulk-Änderungen (Import, Materialbau) alles speichern; viele MCP-Edits sind nicht rückgängig zu machen.
- Nicht während einer laufenden C++-/Shader-Kompilierung Tools aufrufen.
- Rückgabewerte lesen: Blueprint-/Material-/Niagara-Operationen melden Fehler oft nur im Ergebnis, nicht als Exception.
- Keine Asset-Erzeugung während PIE.
