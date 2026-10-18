# Blueprint-Bauplan (Blueprint-first)

Berlin Expendables ist ein **reines Blueprint-Projekt**. Dieses Dokument ist der ausführbare Bauplan für die
lokale Claude-Code-Session mit Unreal-MCP (BlueprintTools, MaterialTools, NiagaraTools, Python) – und zugleich die
Referenz für alle, die im Editor von Hand arbeiten. Jede Funktion hat ein 1:1-Gegenstück im C++-Port unter
`Reference/CppPort/Source/BerlinExpendables`; dort stehen die exakten Formeln und Zahlen.

## 0. Grundregeln

- **Levelraum:** Simulation in 2D-Vektoren (`Vector2D`): x nach rechts, y nach **unten**, 1 Einheit = 1 Texel. Umrechnung nach Unreal: `X = x`, `Z = -y`, `Y = Tiefe` (Kamera schaut entlang +Y). Tiefen: Tiles 0, Pawns −2, Projektile −5, Effekte −10, Hintergrund-Tiles +20, Erdreich +30, Parallax +500…+900.
- **Fester Schritt:** Pawns/Projektile simulieren in Substeps von 1/120 s (Akkumulator im Tick, max. 12 Schritte). Geschwindigkeiten in Einheiten/s.
- **Keine UE-Kollision** für Gameplay: alles läuft über `BP_TileWorld` (AABB gegen Grid). Alle Komponenten: Collision = NoCollision.
- **Ordner:** `/Game/BE/Blueprints` (Klassen), `/Game/BE/Data` (DataAsset-Instanzen, von Python erzeugt), `/Game/BE/Sprites|Flipbooks|Textures|Materials` (Python), `/Game/BE/UI` (Widgets), `/Game/BE/FX` (Niagara), `/Game/Maps`.
- **Namen:** Blueprints `BP_*`, Enums `E_*`, Structs `S_*`, Widgets `WBP_*`, Function Library `BPL_*`, Niagara `NS_*`.

## 1. Enums & Structs (zuerst anlegen)

| Asset | Werte / Felder |
| --- | --- |
| `E_Tile` | Air, Dirt, Brick, Steel, Concrete, Platform, Crate |
| `E_Back` | None, Plaster, Window, Brick, Graffiti |
| `E_Aim` | Horizontal, Up, Down |
| `E_InputSource` | Controller, KeyboardTwo |
| `E_MatchState` | Playing, Won, GameOver |
| `E_ItemKind` | Heavy, Rocket, Flame, Laser, Bombs, Life |
| `E_HitKind` | Shot, Melee, Explosion, Touch, Fall |
| `E_MookState` | Patrol, Alert, Attack, Panic |
| `S_Body` | Pos (Vector2D), Size (Vector2D), DropThrough (Float) |
| `S_InputFrame` | Left, Right, Up, Down, Jump, Fire, Special, Melee, Start (Bool) + JumpPressed, FirePressed, SpecialPressed, MeleePressed, StartPressed (Bool) |
| `S_BulletProfile` | Flipbook (PaperFlipbook), Speed, LifeTime, Damage, TileDamage (Float), Pierce (Int), Gravity, Lob, ExplodeRadius (Float), Spin (Bool) |
| `S_WeaponProfile` | WeaponId (Name), Cooldown (Float), Count (Int), Spread, Recoil, Shake (Float), Bullet (S_BulletProfile) |
| `S_EntitySpawn` | Type (Name), TX, TY (Int), Arg (String) |
| `S_Popup` | Text (String), Pos (Vector2D), Color (LinearColor), Life (Float) |
| `S_JoinRecord` | ControllerIndex (Int), Source (E_InputSource) |
| `S_KeyLayout` | Left, Right, Up, Down, Jump, Fire, Special, Melee, Start (Key) |

## 2. Daten-Blueprints (Parent: **PrimaryDataAsset**) – Variablennamen sind Vertrag mit `be_import_sprites.py`

| Blueprint | Variablen (Name: Typ) |
| --- | --- |
| `BP_CharacterData` | Id: Name · DisplayName: Text · Tagline: Text · Accent: LinearColor · Skin: LinearColor · IsPlayer: Bool · Flipbooks: Map<Name, PaperFlipbook> · Portrait: Texture2D · Gibs: Array<PaperSprite> · BodySize: Vector2D · CrouchSize: Vector2D · WeaponId: Name · Cooldown: Float · Count: Int · Spread: Float · Recoil: Float · Shake: Float · BulletFlipbook: PaperFlipbook · BulletSpeed, BulletLife, BulletDamage, BulletTileDamage: Float · BulletPierce: Int · BulletGravity, BulletLob, BulletExplode: Float · BulletSpin: Bool · Grenades: Int · GrenadeRadius: Float · Hp: Int · Burst: Int · BurstGap: Float · CooldownMin, CooldownMax: Float · SightRange, WalkSpeed: Float · Score: Int |
| `BP_SpriteLibrary` | Sprites: Map<Name, PaperSprite> · Flipbooks: Map<Name, PaperFlipbook> |
| `BP_WeaponLibrary` | Sprites: Map<Name, PaperSprite> (Schlüssel `<id>_h/_u/_d`, Pivot = Griff, Socket `Muzzle`) |
| `BP_TileSet` | Atlas: Texture2D · Rects: Map<Name, Vector4> (u0, v0, u1, v1 normalisiert) |
| `BP_LevelData` | DisplayName: Text · Rows: Array<String> |

Flipbook-Schlüssel in `Flipbooks`: `idle_h`, `idle_u`, `idle_d`, `run_*`, `jump_*`, `fall_*`, `crouch_h`, `crouch_u`, `melee_h`, `panic_h`. Sprites tragen den Socket `Hand` (Körper) bzw. `Muzzle` (Waffe); Pivot der Körper-Sprites liegt auf Fußmitte.

Nach dem Anlegen dieser fünf Klassen: `py be_build_all.py` erneut ausführen → erzeugt `DA_Char_*`, `DA_SpriteLibrary`, `DA_WeaponLibrary`, `DA_TileSet`, `DA_Level_Kreuzberg01`.

## 3. BP_TileWorld (Actor) – zerstörbare Welt

**Komponenten:** `Root` (Scene), `ISM_Tiles`, `ISM_Back`, `ISM_Earth` (Instanced Static Mesh, Mesh `/Engine/BasicShapes/Plane`, Material `M_TileAtlas`, Num Custom Data Floats = 5, Mobility Movable, kein Schatten, keine Kollision).

**Variablen:** Level (BP_LevelData), TileSet (BP_TileSet), TileSize = 32, W, H (Int), Tiles (Array<Byte>), Hp (Array<Float>), Back (Array<Byte>), WasSolid (Array<Bool>), InstanceIndex (Array<Int>), Spawns (Array<S_EntitySpawn>), SpawnTile (IntPoint), FlipU, FlipV (Bool).
**Event Dispatcher:** `OnTileDestroyed(TX, TY, Tile: E_Tile, Center: Vector2D)`.

**Funktionen**
- `Index(TX, TY) → Int`: `TY * W + TX`.
- `GetTile(TX, TY) → E_Tile`: außerhalb links/rechts → Steel (Wände), außerhalb oben/unten → Air; sonst `Tiles[Index]`.
- `IsSolidTile(T) → Bool`: `T ≠ Air und T ≠ Platform`. `IsSolidAt(P: Vector2D)`: `IsSolidTile(GetTile(floor(P.x/TileSize), floor(P.y/TileSize)))`.
- `MaxHp(T)`: Dirt 3, Brick 6, Concrete 9, Platform 4, Crate 2, Steel ∞ (verwende 1e30), Air 0.
- `BuildFromLevel(LevelData)`: `ParseRows(Rows)` → `BuildVisuals()`.
- `ParseRows(Rows)`: W = längste Zeile, H = Anzahl; Arrays mit `Resize` füllen; Zeichen-Mapping: `#` Dirt, `=` Brick, `S` Steel, `C` Concrete, `-` Platform, `c` Crate, `w/W/r/g` Back (Plaster/Window/Brick/Graffiti), `P` SpawnTile, `m/s/b/k/E/+` → Spawns (Type mook/security/barrel/cage/exit/life), `H R F L B` → Spawn Type item mit Arg = Buchstabe. `WasSolid = IsSolidTile(T) und T ≠ Crate`. Danach: für jeden Spawn mit Back links **und** rechts den linken Back-Wert übernehmen (Innenräume).
- `TileTransform(TX, TY, Depth) → Transform`: Location = ToWorld(TileCenter, Depth), Rotation (Roll 90°), Scale (TileSize/100, TileSize/100, 1).
- `ApplyRect(ISM, Idx, RectName, Damage)`: Rect aus TileSet (Fallback 0,0,0.01,0.01), FlipU/FlipV tauschen Komponenten; `Set Custom Data Value` Index 0..3 = u0 v0 u1 v1, Index 4 = Damage (MarkRenderStateDirty nur beim letzten).
- `VisualName(TX, TY) → Name`: Dirt → `dirt_top` wenn darüber Air/Platform, sonst `dirt`; Brick → `brick_top`/`brick`; Steel `steel`; Concrete `concrete`; Platform `platform`; Crate `crate`.
- `BuildVisuals()`: Instanzen löschen; für jedes Tile: Back → `ISM_Back` AddInstance (Depth 20) + ApplyRect(`back_*`); WasSolid → `ISM_Earth` (Depth 30) + ApplyRect(`dirt`, Damage 1); Tile ≠ Air → `ISM_Tiles` AddInstance (Depth 0), `InstanceIndex[i]` merken, ApplyRect(VisualName, 0).
- `RefreshTile(TX, TY)`: Air → Instanz per `Update Instance Transform` auf Scale 0 (Index bleibt stabil); sonst ApplyRect mit Damage = (1 − Hp/MaxHp) · 0.6.
- `DamageTile(TX, TY, Amount) → Bool`: Air/Steel → false; `Hp -= Amount`; > 0 → RefreshTile, false; sonst Tile = Air, RefreshTile(TX,TY), RefreshTile(TX,TY+1), Dispatcher auslösen, true.
- `DamageRadius(Center, Radius, Power) → Int`: Tiles im Quadrat um Center, Abstand D ≤ Radius → `DamageTile(..., Power·(1 − D/Radius) + 2)`.
- `MoveX(Body: S_Body by ref, DX) → Bool` (hit): NX = Pos.x + DX; Reihen Y0 = floor(Pos.y/T), Y1 = floor((Pos.y+Size.y−0.01)/T). DX > 0: TX = floor((NX+Size.x−0.01)/T), wenn eine Reihe solide → Pos.x = TX·T − Size.x, true. DX < 0: TX = floor(NX/T), solide → Pos.x = (TX+1)·T, true. Sonst Pos.x = NX, false.
- `MoveY(Body by ref, DY) → Bool`: analog; nach unten zusätzlich Platform-Regel: blockt nur wenn `(Pos.y + Size.y − 0.01) < TY·T` (war oberhalb) und `DropThrough ≤ 0`.
- `OnPlatformOnly(Body) → Bool`, `FitsAt(Body) → Bool` wie im C++-Port.
- `SpawnPoint() → Vector2D`: `SpawnTile · TileSize`.

## 4. BP_PawnBase (Pawn) – Basis für Bros und Gegner

**Komponenten:** `Root`, `Body` (PaperFlipbook), `Weapon` (PaperSprite). Auto Possess AI = Disabled.
**Variablen:** Data (BP_CharacterData), Box (S_Body), Vel (Vector2D), OnGround, Facing (Int = 1), Aim (E_Aim), Crouch, CurrentAnim (Name), CurrentAnimAim (E_Aim), WeaponId (Name), Accum (Float), Step = 1/120.

**Funktionen**
- `SetCharacter(Data)`: Box.Size = BodySize (oder CrouchSize), Füße beibehalten, CurrentAnim leeren, `SetWeaponId(Data.WeaponId)`, `UpdateVisuals`.
- `SetFeet(FeetPos)`: Pos = (Feet.x − Size.x/2, Feet.y − Size.y). `Center()`, `Feet()`.
- `SetWeaponId(Id)`: WeaponId setzen, Sprite = `WeaponLibrary.Sprites[Id_<aim>]` (Fallback `_h`).
- `MuzzlePos() → Vector2D`: Weapon hat Socket `Muzzle` → `GetSocketLocation` → ToLevel; sonst Center + (Facing·24, −4).
- `AnimName() → Name` (überschreibbar): nicht am Boden → `jump` (Vel.y < 0) / `fall`; |Vel.x| > 12 → `run`; sonst `idle`.
- `SetAnim(Anim)`: Flipbook = `Data.Flipbooks[<Anim>_<aim>]` (Fallback `_h`, dann `idle_h`); wenn anders: Set Flipbook, Looping = (Anim ≠ melee), Play From Start; dann Waffen-Sprite neu wählen.
- `UpdateVisuals()`: Actor Location = ToWorld(Feet, −2); Body Relative Scale = (Facing, 1, 1); Anim/Aim geändert → SetAnim; Waffe sichtbar wenn `ShowWeapon()` und Body hat Socket `Hand`: World Location = Socket `Hand` (Y = −2.5), World Scale = (Facing, 1, 1).
- `SimStep(Dt)` (überschreibbar, leer), `ShowWeapon() → Bool` (true).
- **Event Tick:** Accum += min(Dt, 0.1); while Accum ≥ Step (max 12): SimStep(Step), Accum −= Step; UpdateVisuals.

Hilfsfunktionen in `BPL_Combat`: `ToWorld(P, Depth) → Vector`, `ToLevel(V) → Vector2D`, `GetGS() → BP_GameState`, `GetTileWorld()`.

## 5. BP_Bro (Child von BP_PawnBase) – Spieler

**Variablen:** Slot, ControllerIndex (Int), Source (E_InputSource), Alive = true, DeadTimer, Invulnerable, Grenades = 3, PickupActive (Bool), PickupKind (E_ItemKind), PickupAmmo (Int), PickupProfile (S_WeaponProfile), Kills, Banner (Float), Input (S_InputFrame), Coyote, JumpBuffer, FireCd, MuzzleT, SpecialCd, MeleeT, AnimClock.
Tunables (aus BP_GameMode lesen oder lokal): Gravity 2304, MaxFall 780, Run 204, Crouch 66, GroundAcc 2520, AirAcc 1440, Jump 708, JumpCut 288, CoyoteTime 0.1, JumpBufferTime 0.1.

**Funktionen**
- `SetInput(Frame)`: Pressed-Flags ODER-verknüpft übernehmen (bleiben bis zum nächsten SimStep erhalten), Rest kopieren.
- `GetWeapon() → S_WeaponProfile`: PickupActive ? PickupProfile : aus Data gebaut (`MakeWeaponFromData`).
- `MakePickupProfile(Kind)`: Heavy (`heavy`, Cooldown 0.067, Spread 0.12, Recoil 30, Shake 0.08; Bullet `heavybullet` 1080 uu/s, Life 1.0, Damage 2, TileDamage 2) · Rocket (`rocket`, 0.5, Recoil 120, Shake 0.2; `rocket` 600, Life 1.5, TileDamage 3, Explode 76) · Flame (`flame`, 0.05, Spread 0.15, Recoil 12, Shake 0.03; `flame` 432, Life 0.4, TileDamage 0.35, Pierce 99, Gravity −216) · Laser (`laser`, 0.033, Shake 0.04; `laserbeam` 1440, Life 0.66, TileDamage 1.5, Pierce 99).
- `GivePickup(Kind, Ammo)`, `Die()`, `RespawnAt(Feet)`, `SwitchCharacter(Data)`, `SetCrouch(On)` (Box.Pos.y um (BodySize.y − CrouchSize.y) verschieben; Aufstehen nur wenn `FitsAt`), `EnemyInReach()`, `MeleeHit()`, `Fire()`, `ThrowGrenade()`, `SlotColor()` (P1 Blau #2f80ed, P2 Rot #e23b3b, P3 Grün #7fd04f, P4 Gelb #ffcf5c).
- `SimStep(Dt)` – Reihenfolge exakt wie `ABEBro::SimStep`: Banner/Tod/Unverwundbarkeit → Ducken & Zielen → Horizontal (Beschleunigung, Reibung 0.68^(Dt·60) am Boden, 0.97 in der Luft, Überschuss 0.9) → Sprung (Coyote, Buffer, Durchfallen bei Unten+Sprung auf Plattform, Jump-Cut) → Schwerkraft → MoveX/MoveY → Kamerarand (`Camera.Left + 2 … Camera.Right − Size.x − 2`) → Sturz unter Level → Nahkampf (automatisch bei `EnemyInReach`, Dauer 0.2 s, Treffer bei 0.12 s) → Feuer (Cooldown, Aim-Richtung, Fächer-Streuung, Rückstoß, Ketchup-Jump bei Aim Down in der Luft: Vel.y −= Recoil·1.6) → Granate (Vel = (Facing·372 + Vel.x·0.5, Up ? −720 : −504)) → Pressed-Flags löschen.
- `AnimName()`: MeleeT > 0 → `melee`; Crouch → `crouch`; sonst Parent. `ShowWeapon()`: MeleeT ≤ 0. `UpdateVisuals()`: Crouch + Aim Down → Horizontal; Blinken bei Invulnerable (15 Hz).

## 6. BP_Mook (Child von BP_PawnBase) – Gegner

**Komponenten zusätzlich:** `Alert` (PaperSprite `alert`, über dem Kopf, standardmäßig unsichtbar).
**Variablen:** State (E_MookState), Hp, Stun, Dead, Timer, ShootCd, BurstTimer, BurstLeft.
**Logik (`SimStep`):** Ziel = nächster lebender Bro; `Sees` = |dx| < SightRange, |dy| < 112, (Blickrichtung passt oder |dx| < 72). Patrol: Laufen/Pausen (Timer), an Kante/Wand drehen (`LedgeOrWallAhead`: vor den Füßen kein Boden oder auf Kniehöhe Wand), bei Sicht → Alert 0.37 s. Alert → Attack (ShootCd 0.33). Attack: zum Ziel drehen (außer im Salvenlauf), Aim Up wenn Ziel > 60 höher und |dx| < 80, Salven (`Burst` Schüsse im Abstand `BurstGap`), Pause `Random(CooldownMin, CooldownMax)`; Ziel verloren > 1 s → Patrol. Panic: 2,6-fache Laufgeschwindigkeit, Arme hoch (Anim `panic`), Timer → Patrol. Dann Schwerkraft, MoveX/MoveY (Wandkontakt dreht in Patrol/Panic).
- `FireAt()`: Projektil aus `MuzzlePos` mit `Data`-Bullet (Streuung ±0.06 rad), kleiner Lichtblitz.
- `Panic(Seconds, AwayFromX)`, `Kill()` (Dead = true, Destroy Actor).

## 7. Projektile, Granaten, Requisiten

- **BP_Projectile** (Actor, `Sprite` PaperFlipbook): `Init(At, Vel, Profile, ByPlayer, Slot)` (Vel.y −= Lob). Tick: Gravity; Substeps so, dass pro Schritt ≤ 8 Einheiten; pro Schritt: Tile solide → DamageTile(TileDamage), Funken, `Impact`; Spieler-Kugel: Mooks (`Box.Contains(Pos, Pad 4)`, je Mook nur einmal → `HurtMook`, Pierce−−), Fässer (`Ignite`), Käfige (`Hurt`); Gegner-Kugel: lebende, verwundbare Bros → `KillBro(Shot)`. Lebenszeit, Levelgrenzen. Darstellung: Spin → Rotation Pitch += 720·Dt; sonst vertikal (|Vel.y| > 1.5·|Vel.x|) → Pitch ±90, horizontal → Scale.x = sign(Vel.x). `Impact`: ExplodeRadius > 0 → `BPL_Combat.Explode`; Spin → Glasfunken; Destroy.
- **BP_Grenade** (Actor, `Sprite` = `grenade`): Box 10×10, Gravity 1800 (max. 720), Abprall X −0.5, Y −0.45 (X ·0.75, Ruhe < 108), Zündschnur 1.17 s, Rauch alle 0.17 s, Blinken < 0.4 s, dann `Explode(Center, Radius)`.
- **BP_Barrel** (`barrel`, Box 16×26): `Ignite(Delay)`; Tick: Blinken, bei 0 → Score 50, `Explode(Center, 80, FromBarrel)`, Destroy.
- **BP_Cage** (`cage` + `Captive` Flipbook `idle_h` von `DA_Char_captive`, Box 32×48, Hp 3): `Hurt(Damage, Slot)` → Funken; ≤ 0 → `Open(Slot)` → `GameState.Rescue(self, Slot)`, Destroy.
- **BP_Captive** (Flipbook `run_h`): läuft 1,8 s in Facing-Richtung (144 uu/s), Schwerkraft, dreht an Wänden, verschwindet mit Staub.
- **BP_Item** (`item_<X>` / `item_life`, Box 20×20): Schwerkraft 900 (max 600), Bobbing, Lebenszeit 20 s (blinkt < 3 s); Überlappung mit lebendem Bro → `Apply`: Bombs +5 Granaten (max 9), Life → Lives+1, sonst `GivePickup` (Heavy 200, Rocket 15, Flame 150, Laser 200); Popup mit Name; Score 100.
- **BP_Exit** (`trabi`, Box 80×40): wenn ≥ 1 lebender Bro berührt und alle lebenden Bros < 180 Einheiten entfernt → `Win`, Bros verstecken, Trabi fährt mit 60 + t·90 uu/s nach rechts, Staubwolken.
- **BP_Debris** (StaticMesh `/Engine/BasicShapes/Cube`, Material `M_Debris` mit Farbparameter `Color`): eigene Flugbahn im Levelraum (Gravity 1800, Abprall an soliden Tiles −0.4/−0.3), Rotation, Lebenszeit 0.8–2 s, schrumpft am Ende. Scale = Size/100.
- **BP_Flash** (PointLight, kein Schatten, kein Inverse-Square, Falloff 2): `Init(Color, Intensity, Radius, Duration)`, Intensität · (1 − t)², dann Destroy.

## 8. BPL_Combat (Blueprint Function Library)

- `Explode(Center, Radius, ByPlayer, Slot, FromBarrel)`: `TileWorld.DamageRadius(Center, Radius, 12)`; Mooks im Radius + 12 → `HurtMook(9, Explosion)`; Bros im Radius · 0.85 (nicht unverwundbar) → `KillBro(Explosion)`; Fässer im Radius + 20 → `Ignite(0.07–0.2)`; Käfige im Radius + 12 → `Open`; Niagara `NS_Explosion` (Scale Radius/64, User-Param `Tint`), Trümmer: 10 Orange (520 uu/s, 5), 8 Gold (420, 4), 6 Grau (200, 8); `Flash` (Orange, 40·Scale, Radius·6, 0.35 s); `Camera.Shake(FromBarrel ? 0.6 : 0.5)`; `GameState.Freeze(0.05)`.
- `SpawnDebris(At, Count, Color, Speed, Size)`, `Flash(...)`, `Blood(At, Count)` (Gore: Rot/Dunkelrot; sonst Konfetti in 4 Farben), `Sparks(At, Count)`, `Dust(At, Count)`, `ToWorld`, `ToLevel`.

## 9. BP_GameState (GameStateBase)

**Variablen:** Lives = 5, Score, MatchState, StateTime, MatchTime, Debug, TileWorld, Camera, Bros, Mooks, Barrels, Cages, Items, Exits (Arrays), Popups (Array<S_Popup>), FreezeUntilRealTime, Frozen.
**Funktionen:** `AddScore(Amount, At, Show)` (Popup `+N` in Gold), `AddPopup(Text, At, Color)`, `Freeze(Seconds)` (Set Global Time Dilation 0.05; Tick stellt bei `Get Real Time Seconds ≥ FreezeUntil` auf 1 zurück), `HandleTileDestroyed(TX, TY, Tile, Center)` (Trümmer in Tile-Farbe, Kiste: Score 10, 35 % Item), `KillBro(Bro, Kind)` (Die, Blut 24, Gibs in Accent/Haut, Shake 0.45, Freeze 0.06, Game Over wenn Pool leer und niemand lebt), `HurtMook(Mook, Damage, Slot, Kind)` (Stun 0.17 bei Überleben; sonst Blut, Trümmer, Score, Kills++, Nachbarn < 140/80 Einheiten mit 50 % in Panik, `Kill`), `Rescue(Cage, Slot)` (Lives+1, Score 500, Popup, Captive, zufälliges Item H/R/F/L/B, Retter wechselt Bro), `TryRespawn(Bro)` (Lives−1, neben lebendem Bro 60 Einheiten höher, neuer zufälliger Bro, `RespawnAt`), `NearestAliveBro`, `AnyAliveBro`, `AliveBroCount`, `PickCharacter(For, Random)` (keine Doppelten unter Lebenden), `Win`, `GameOver`, `CleanupLists`.
**Tick:** Freeze zurücksetzen, Zeiten, Popups (Life −= Dt, y −= 24·Dt), Gegnerkontakt (Bro-Box überlappt Mook-Box, Mook nicht in Panik → `KillBro(Touch)`), Listen bereinigen.

## 10. BP_GameMode, BP_GameInstance, BP_PlayerController

- **BP_GameMode** (GameModeBase; GameState = BP_GameState, PlayerController = BP_PlayerController, Default Pawn = None, HUD = None → UMG). Variablen: LevelOverride (BP_LevelData), StartLevel (BP_LevelData), PlayerCharacters (Array<BP_CharacterData>), MookData, SecurityData, CaptiveData, WeaponLibrary, SpriteLibrary, TileSet, MaxPlayers = 4, StartLives = 5, RespawnDelay = 1.4, InvulnerableTime = 2, Gore = true. **BeginPlay:** TileWorld finden/spawnen (`BuildFromLevel`), `OnTileDestroyed` an GameState binden; Kamera finden/spawnen, `SnapTo(SpawnPoint)`; wenn keine `BP_ParallaxLayer` platziert: Sky (Faktor 0, Bottom 540, Depth 900), Skyline (0.15, 500, 700), Mid (0.4, 570, 500) aus `SpriteLibrary` (`bg_sky`, `bg_skyline`, `bg_mid`); `SpawnEntities` (Positionen: Pawn-Füße = Tile-Mitte unten; Barrel +(8,6); Cage (0, TileSize−48); Exit Tile oben links; Items +(6,6)); `Create Player` für Controller-IDs 1…MaxPlayers−1; alle Controller `Set View Target` = Kamera; Wiederbeitritte aus BP_GameInstance.
  `RequestJoin(PC, Source) → BP_Bro`, `SpawnBro(ControllerIndex, Source, Data)`, `RestartMatch()` (GameInstance.Restarting = true, `Open Level` aktuelle Map).
- **BP_GameInstance:** PersistentJoins (Array<S_JoinRecord>), Restarting (Bool). In Project Settings → Maps & Modes eintragen.
- **BP_PlayerController:** Variablen PrimaryBro, KeyboardTwoBro, PrevPrimary, PrevKeyboardTwo (S_InputFrame), Layouts KeyboardOne/KeyboardTwo/Gamepad (S_KeyLayout; Defaults: WASD + J/K/L/U/Enter; Pfeile + ,/./÷/M/Rechts-Shift; D-Pad + FaceButtons Bottom/Left/Top/Right/Special Right), StickDeadzone 0.5. **Tick:** `Get Player Controller ID` = 0 → zwei Tastatur-Frames (`Is Input Key Down`), sonst Gamepad-Frame (+ `Get Input Analog Key State` Left X/Y, Right Trigger = Feuer). `Dispatch(Frame, Prev, Bro, Source)`: Flanken berechnen; MatchState ≠ Playing → Start/Sprung nach 1,5 s = `RestartMatch`; kein Bro → Sprung/Start = `RequestJoin`; sonst `Bro.SetInput`. F1 → Debug.

## 11. BP_Camera, BP_ParallaxLayer

- **BP_Camera** (Actor, CameraComponent orthografisch, OrthoWidth 640, Seitenverhältnis 16:9 erzwingen, Rotation Yaw 90°, Bloom 0.55, Auto-Exposure fixiert 1/1, Vignette 0.25). Variablen View (Vector2D, obere linke Ecke), ViewSize (640, 360), Trauma. Tick (nach den Pawns): Bounding-Box aller lebenden Bros (nach Sieg: Trabi; ohne Spieler: Spawnpunkt) → Ziel = Mitte − ViewSize/2 − (0, 40); View folgt mit 1 − 0.88^(Dt·60) horizontal, 0.9 vertikal; auf Level klemmen; Shake-Offset = zufällig ·12/8 · Trauma² (Trauma −2.1/s); Actor Location = ToWorld(round(View + ViewSize/2 + Shake), −1000). `SnapTo(Center)`, `Shake(Amount)`, `Left()`, `Right()`.
- **BP_ParallaxLayer** (Actor): Sprite, Factor, BottomY, Depth, Copies = 4 → PaperSprite-Komponenten nebeneinander; Tick: offset = fmod(CamX·Factor, W), StartX = CamX − offset − W; Bottom = BottomY + CamY·(1 − Factor).

## 12. UI (UMG)

- **WBP_HUD:** 4 Spielerkarten (Porträt, Slotfarbe, Name, Granaten, Pickup-Munition, „RESPAWN n“/„TOT“), Leben mittig, Punkte rechts, Join-Hinweis unten (blinkt), Spawn-Banner (Name groß in Slotfarbe + Spruch, 2,5 s, hinein-/hinausgleitend), Titel-Overlay solange niemand spielt (BERLIN / EXPENDABLES + Steuerung), Endbildschirm (KIEZ BEFREIT! / GAME OVER, Punkte, MVP, „START/SPRUNG = NOCHMAL“ nach 1,5 s). Bindings auf BP_GameState.
- **WBP_Popup:** schwebender Text; BP_GameState erzeugt pro `S_Popup` ein Widget und positioniert es per `Project World Location to Screen` (ToWorld(Pos, −2)), Leben 0.9 s.
- Pixel-Font: Bitmap-Font-Asset aus `Tools/spritegen` (später) oder vorerst ein Monospace-Font.

## 13. VFX (Niagara) – per MCP NiagaraTools

`NS_Explosion` (Sprite-Sheet `T_fx` Explosion 8 Frames, GPU-Funken, Rauchring, Light-Renderer orange 0,3 s, User-Param `Tint`), `NS_Debris` (Mesh-Partikel Würfel), `NS_Blood`, `NS_Sparks`, `NS_Dust`, `NS_Smoke`. Alle Texturen Nearest. Bis sie existieren, liefern `BP_Debris` + `BP_Flash` den Look.

## 14. Baureihenfolge (jeweils kompilieren, speichern, kurz in PIE testen)

1. Enums, Structs, die 5 Daten-Blueprints → `py be_build_all.py` → `/Game/BE/Data` prüfen.
2. `BPL_Combat` (nur ToWorld/ToLevel/GetGS), `BP_GameState` (Variablen), `BP_GameInstance`, `BP_GameMode` (nur Variablen + Create Player + Kamera), `BP_Camera`, `BP_PlayerController` → Project Settings (GameMode, GameInstance, Startmap) → PIE: Kamera steht, kein Fehler.
3. `BP_TileWorld` komplett → in `L_Kreuzberg01` platzieren, `Level` = `DA_Level_Kreuzberg01` → PIE: Welt sichtbar, Hintergründe.
4. `BP_PawnBase`, `BP_Bro` (Bewegung, Sprung, Ducken, Kamerarand) → Beitritt per Taste → Laufen & Springen testen.
5. `BP_Projectile`, `BP_Bro.Fire`, `BP_Debris`, `BP_Flash`, `BPL_Combat.Explode`, `BP_Grenade` → Krater, Trümmer, Licht.
6. `BP_Mook` + GameState-Regeln (Kill, Hurt, Panik, Kontakt) → Gegner im Level.
7. `BP_Barrel`, `BP_Cage`, `BP_Captive`, `BP_Item`, `BP_Exit`, Respawn/Bro-Wechsel, Sieg/Game Over.
8. `WBP_HUD`, `WBP_Popup`, Banner.
9. Niagara-Systeme, Materialfeinschliff (Normalmaps), AutoSprite-Bros, ComfyUI-Ebenen.

## 15. Performance-Hinweise für Blueprint

- Tile-Arrays als Byte/Float-Arrays, Zugriff nur per Index-Funktion; keine String-Operationen in Tick (Parsing nur in `BuildFromLevel`).
- `Set Custom Data Value` mit `Mark Render State Dirty` nur beim letzten Wert pro Tile; `Update Instance Transform` ebenso.
- Projektile: Substeps begrenzen (Schrittweite 8 Einheiten), Listen aus BP_GameState lesen statt `Get All Actors Of Class`.
- Trümmer: maximal ~150 gleichzeitig (BPL_Combat zählt; bei Überlauf weniger spawnen).
- Pawns: ein Tick mit Substep-Schleife, keine Timer pro Pawn.
