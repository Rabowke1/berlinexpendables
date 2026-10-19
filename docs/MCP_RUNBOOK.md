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

1. `BerlinExpendables.uproject` in UE 5.8 öffnen. Es ist ein reines Blueprint-Projekt: **kein Kompilieren, kein Visual Studio nötig.** (Erscheint trotzdem ein „Missing Modules“-Dialog, liegt noch ein alter `Source/`-Ordner, `Binaries/` oder `Intermediate/` aus einem früheren Stand im Projektordner – diese Ordner löschen; der C++-Port liegt jetzt bewusst unter `Reference/CppPort`.)
2. Die Plugins `ModelContextProtocol` und `AllToolsets` sind in der `.uproject` bereits aktiviert. Falls der Editor sie nicht findet, stammen sie aus dem Engine-Ordner `Engine/Plugins/Experimental/ModelContextProtocol` deiner Installation.
3. MCP-Server starten: Editor-Konsole (`~`) → `ModelContextProtocol.StartServer`. Für Autostart in `Saved/Config/<Platform>Editor/EditorPerProjectUserSettings.ini`:
   ```ini
   [/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]
   bAutoStartServer=True
   ```
4. Die `.mcp.json` im Repo zeigt auf `http://127.0.0.1:8000/mcp`. Falls du Port/Pfad geändert hast: Konsole `ModelContextProtocol.GenerateClientConfig ClaudeCode` schreibt sie neu.
5. Im Terminal `claude` starten; beim ersten Mal die Projekt-MCP-Server bestätigen. Prüfen mit `/mcp` (Häkchen bei `unreal-mcp`) oder `claude mcp list`.

## 3. AutoSprite- und ComfyUI-MCP eintragen

### ComfyUI (eingerichtet am 19.10.)

Als Brücke dient `comfy-mcp` (offizieller Comfy-Org-MCP-Server, PyPI) mit `comfy-cli` als Engine. Beide laufen isoliert als `uv`-Tools; ComfyUI selbst ist die Stability-Matrix-Installation unter `E:\temp\Data\Packages\ComfyUI`, die auf `127.0.0.1:8188` läuft.

```powershell
uv tool install "comfy-cli>=1.14.0"          # Befehl: comfy
uv tool install comfy-mcp                     # Befehl: comfy-mcp
comfy --skip-prompt tracking disable
comfy --skip-prompt set-default "E:\temp\Data\Packages\ComfyUI"
comfy --skip-prompt env                       # server.running muss true sein
```

Der Eintrag in `.mcp.json` (committed) startet `comfy-mcp.exe` per stdio; `COMFY_BIN` zeigt auf `comfy.exe`, `COMFYUI_URL` auf den laufenden Server. Pfade sind benutzerbezogen (`C:\Users\<Name>\.local\bin\...`), auf einem anderen Rechner anpassen. Prüfen: `claude mcp list` → `comfyui: … √ Connected`. Wichtige Tools: `server_info`, `search_models`, `nodes`, `validate_workflow`, `run_workflow`, `job`, `fetch_outputs`, `generate_image`, `search_templates`. ComfyUI muss vorher laufen (Stability Matrix → Launch); `launch_comfyui` über den MCP ist für diese Installation nicht nötig.

Vorhandene Modelle (Stand 19.10.): `flux1-dev-fp8` (Komplettcheckpoint), `flux_dev` + `t5xxl` + `clip_l` + `ae`, Qwen Image 2512, HiDream, `dreamshaper_8` (SD 1.5), mehrere Pony/SDXL-Checkpoints. Nachgeladene LoRAs in `E:\temp\Data\Models\Lora`: `flux_retro_pixel` (Trigger `Retro Pixel`), `flux_ume_modern_pixelart` (Trigger `umempart`), `sdxl_pixel-art-xl` (Trigger `pixel art`). Der getestete Hintergrund-Workflow steht in `docs/PROMPTS_COMFYUI.md`; Skripte unter `Tools/comfyui/` (`run_comfy.ps1`, `seamfix.py`, `workflows/`).

### AutoSprite

```bash
# Remote-MCP mit API-Key; URL laut AutoSprite-Dashboard
claude mcp add --scope project --transport http autosprite https://<autosprite-mcp-url> \
  --header "Authorization: Bearer ${AUTOSPRITE_API_KEY}"
```

`${AUTOSPRITE_API_KEY}` bleibt als Platzhalter in der Datei und wird aus deiner Umgebung expandiert, so landet kein Schlüssel im Repo. Nach Änderungen an `.mcp.json` die Session neu starten.

## 4. Erste Editor-Session (Reihenfolge)

Jeder Punkt ist eine Anweisung an die lokale Claude-Session; die Toolsets werden mit `list_toolsets` / `describe_toolset` entdeckt und per `call_tool` aufgerufen. Der komplette Bauplan steht in `docs/BLUEPRINTS.md`.

1. **Assets importieren (Teil 1):** Python-Toolset → `py be_build_all.py` (oder *Tools → Execute Python Script*). Erzeugt unter `/Game/BE` Texturen (Nearest, keine Mips), Sprites mit Pivots und Hand-/Muzzle-Sockets, Flipbooks, Materialien (`M_TileAtlas`, `M_SpriteLit`, `M_Debris`) sowie die Map `/Game/Maps/L_Kreuzberg01` mit Licht. Warnungen „BP_… fehlt“ sind in diesem Durchlauf normal.
2. **Enums, Structs, Daten-Blueprints** anlegen (`BLUEPRINTS.md` Abschnitt 1 + 2; Variablennamen exakt), dann `py be_build_all.py` **erneut** → `/Game/BE/Data` enthält `DA_Char_*`, `DA_SpriteLibrary`, `DA_WeaponLibrary`, `DA_TileSet`, `DA_Level_Kreuzberg01`.
3. **Blueprints bauen** in der Reihenfolge aus `BLUEPRINTS.md` Abschnitt 14, nach jedem Schritt kompilieren, speichern, PIE. Project Settings → Maps & Modes: GameMode = `BP_GameMode`, Game Instance = `BP_GameInstance`, Startmap = `L_Kreuzberg01`.
4. **PIE-Erwartung nach Schritt 7:** Himmel/Skyline/Häuser-Parallax, Tile-Welt, Titel-Overlay. `K` (Tastatur 1), `.` (Tastatur 2) oder Gamepad-A → Bro erscheint. Schießen `J`, Granate `L`. Explosionen reißen Krater, Trümmerwürfel fliegen, Lichtblitze.
   - Tiles spiegelverkehrt? → `BP_TileWorld.FlipU/FlipV` umschalten.
   - Waffe sitzt daneben? → Hand-Socket/Pivot in `be_import_sprites.py` prüfen.
5. **Niagara (3D-VFX)** mit dem Niagara-Toolset: `NS_Explosion` (Sprite-Sheet `T_fx`, GPU-Funken, Rauchring, Light-Renderer, User-Param `Tint`), `NS_Debris`, `NS_Blood`, `NS_Sparks`, `NS_Dust`, `NS_Smoke`; in `BPL_Combat` referenzieren. Fallbacks (`BP_Debris`, `BP_Flash`) bleiben aktiv.
6. **AutoSprite:** ersten Bro bestellen (`docs/PROMPTS_AUTOSPRITE.md`), Export nach `Content/Import/AutoSprite/<id>/`, `pixelize.py --grid 96 --outline --normals`, `BE_IMPORT_DIR` auf den Ordner setzen, `be_import_sprites.py` erneut.
7. **ComfyUI:** Welt-1-Ebenen mit `Tools/comfyui/workflows/parallax_layer.json`, `pixelize.py --width 1440 --dither --tile-check`, als Sprites importieren, `BP_ParallaxLayer`-Aktoren zuweisen.
8. **Commit:** `Content/**/*.uasset` und `*.umap` laufen über Git LFS. Vor großen MCP-Änderungen und danach speichern (Strg+Shift+S).

## 5. Sicherheitsregeln bei MCP-Änderungen

- Vor Bulk-Änderungen (Import, Materialbau) alles speichern; viele MCP-Edits sind nicht rückgängig zu machen.
- Nicht während einer laufenden C++-/Shader-Kompilierung Tools aufrufen.
- Rückgabewerte lesen: Blueprint-/Material-/Niagara-Operationen melden Fehler oft nur im Ergebnis, nicht als Exception.
- Keine Asset-Erzeugung während PIE.
