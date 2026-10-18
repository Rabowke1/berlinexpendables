// Projekteinstellungen (Project Settings > Game > Berlin Expendables). Werte stehen in Config/DefaultGame.ini.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "BETypes.h"
#include "BESettings.generated.h"

class UBELevelAsset;
class UBECharacterData;
class UBEWeaponLibrary;
class UBESpriteLibrary;
class UBETileSet;
class UNiagaraSystem;
class UStaticMesh;
class UMaterialInterface;

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Berlin Expendables - Gameplay"))
class BERLINEXPENDABLES_API UBEGameSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const UBEGameSettings* Get() { return GetDefault<UBEGameSettings>(); }

	UPROPERTY(Config, EditAnywhere, Category = "Welt") float TileSize = 32.f;
	UPROPERTY(Config, EditAnywhere, Category = "Welt") float ViewWidth = 640.f;
	UPROPERTY(Config, EditAnywhere, Category = "Welt") float ViewHeight = 360.f;
	/** Falls das Quad-Mesh die UVs gespiegelt abbildet: hier korrigieren statt im Material. */
	UPROPERTY(Config, EditAnywhere, Category = "Welt") bool bFlipTileU = false;
	UPROPERTY(Config, EditAnywhere, Category = "Welt") bool bFlipTileV = false;

	UPROPERTY(Config, EditAnywhere, Category = "Bewegung") float Gravity = 2304.f;
	UPROPERTY(Config, EditAnywhere, Category = "Bewegung") float MaxFallSpeed = 780.f;
	UPROPERTY(Config, EditAnywhere, Category = "Bewegung") float RunSpeed = 204.f;
	UPROPERTY(Config, EditAnywhere, Category = "Bewegung") float CrouchSpeed = 66.f;
	UPROPERTY(Config, EditAnywhere, Category = "Bewegung") float GroundAcceleration = 2520.f;
	UPROPERTY(Config, EditAnywhere, Category = "Bewegung") float AirAcceleration = 1440.f;
	UPROPERTY(Config, EditAnywhere, Category = "Bewegung") float JumpVelocity = 708.f;
	UPROPERTY(Config, EditAnywhere, Category = "Bewegung") float JumpCutVelocity = 288.f;
	UPROPERTY(Config, EditAnywhere, Category = "Bewegung") float CoyoteTime = 0.1f;
	UPROPERTY(Config, EditAnywhere, Category = "Bewegung") float JumpBufferTime = 0.1f;

	UPROPERTY(Config, EditAnywhere, Category = "Regeln") int32 StartLives = 5;
	UPROPERTY(Config, EditAnywhere, Category = "Regeln") float RespawnDelay = 1.4f;
	UPROPERTY(Config, EditAnywhere, Category = "Regeln") float InvulnerableTime = 2.f;
	UPROPERTY(Config, EditAnywhere, Category = "Regeln") int32 MaxPlayers = 4;
	UPROPERTY(Config, EditAnywhere, Category = "Regeln") bool bGore = true;

	/** Vom Python-Setup befüllt (Content/Python/be_build_all.py). */
	UPROPERTY(Config, EditAnywhere, Category = "Assets") TSoftObjectPtr<UBELevelAsset> StartLevel;
	UPROPERTY(Config, EditAnywhere, Category = "Assets") TArray<TSoftObjectPtr<UBECharacterData>> PlayerCharacters;
	UPROPERTY(Config, EditAnywhere, Category = "Assets") TSoftObjectPtr<UBECharacterData> MookData;
	UPROPERTY(Config, EditAnywhere, Category = "Assets") TSoftObjectPtr<UBECharacterData> SecurityData;
	UPROPERTY(Config, EditAnywhere, Category = "Assets") TSoftObjectPtr<UBECharacterData> CaptiveData;
	UPROPERTY(Config, EditAnywhere, Category = "Assets") TSoftObjectPtr<UBEWeaponLibrary> WeaponLibrary;
	UPROPERTY(Config, EditAnywhere, Category = "Assets") TSoftObjectPtr<UBESpriteLibrary> SpriteLibrary;
	UPROPERTY(Config, EditAnywhere, Category = "Assets") TSoftObjectPtr<UBETileSet> TileSet;
	UPROPERTY(Config, EditAnywhere, Category = "Assets") TSoftObjectPtr<UStaticMesh> TileQuadMesh;
	UPROPERTY(Config, EditAnywhere, Category = "Assets") TSoftObjectPtr<UMaterialInterface> TileMaterial;
	UPROPERTY(Config, EditAnywhere, Category = "Assets") TSoftObjectPtr<UStaticMesh> DebrisMesh;
	UPROPERTY(Config, EditAnywhere, Category = "Assets") TSoftObjectPtr<UMaterialInterface> DebrisMaterial;

	/** Niagara-Systeme (optional; ohne sie gibt es 3D-Trümmer + Lichtblitz als Fallback). */
	UPROPERTY(Config, EditAnywhere, Category = "VFX") TSoftObjectPtr<UNiagaraSystem> ExplosionFX;
	UPROPERTY(Config, EditAnywhere, Category = "VFX") TSoftObjectPtr<UNiagaraSystem> DebrisFX;
	UPROPERTY(Config, EditAnywhere, Category = "VFX") TSoftObjectPtr<UNiagaraSystem> BloodFX;
	UPROPERTY(Config, EditAnywhere, Category = "VFX") TSoftObjectPtr<UNiagaraSystem> SmokeFX;
	UPROPERTY(Config, EditAnywhere, Category = "VFX") TSoftObjectPtr<UNiagaraSystem> SparksFX;
	UPROPERTY(Config, EditAnywhere, Category = "VFX") TSoftObjectPtr<UNiagaraSystem> DustFX;
};

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Berlin Expendables - Eingabe"))
class BERLINEXPENDABLES_API UBEInputSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const UBEInputSettings* Get() { return GetDefault<UBEInputSettings>(); }

	UPROPERTY(Config, EditAnywhere, Category = "Tastatur") FBEKeyLayout KeyboardOne;
	UPROPERTY(Config, EditAnywhere, Category = "Tastatur") FBEKeyLayout KeyboardTwo;
	UPROPERTY(Config, EditAnywhere, Category = "Gamepad") FBEKeyLayout Gamepad;
	UPROPERTY(Config, EditAnywhere, Category = "Gamepad") float StickDeadzone = 0.5f;
};
