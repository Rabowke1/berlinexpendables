// DataAssets: Level (ASCII), Charaktere, Waffen- und Sprite-Bibliotheken, Tileset. Werden vom Python-Import erzeugt.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BETypes.h"
#include "BEData.generated.h"

class UPaperFlipbook;
class UPaperSprite;
class UTexture2D;

UCLASS(BlueprintType)
class BERLINEXPENDABLES_API UBELevelAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BE") FText DisplayName;
	/** Eine Zeile pro Tilereihe. Legende: docs/ABLAUF.md */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BE") TArray<FString> Rows;
};

UCLASS(BlueprintType)
class BERLINEXPENDABLES_API UBECharacterData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identität") FName Id;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identität") FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identität") FText Tagline;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identität") FLinearColor Accent = FLinearColor::White;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identität") FLinearColor Skin = FLinearColor(0.95f, 0.76f, 0.49f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identität") bool bPlayer = true;

	/** Schlüssel: "<anim>_<aim>", z. B. run_h, idle_u, crouch_h, melee_h, panic_h. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grafik") TMap<FName, TObjectPtr<UPaperFlipbook>> Flipbooks;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grafik") TObjectPtr<UTexture2D> Portrait;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grafik") TArray<TObjectPtr<UPaperSprite>> Gibs;
	/** Kollisionsbox im Levelraum (Breite/Höhe stehend und geduckt). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grafik") FVector2D BodySize = FVector2D(20.f, 44.f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grafik") FVector2D CrouchSize = FVector2D(20.f, 34.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kampf") FBEWeaponProfile Weapon;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kampf") int32 Grenades = 3;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kampf") float GrenadeRadius = 64.f;

	// Gegnerwerte
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gegner") int32 Hp = 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gegner") int32 Burst = 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gegner") float BurstGap = 0.12f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gegner") FVector2D CooldownRange = FVector2D(1.0f, 1.8f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gegner") float SightRange = 300.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gegner") float WalkSpeed = 60.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gegner") int32 Score = 100;

	UFUNCTION(BlueprintPure, Category = "BE")
	UPaperFlipbook* GetFlipbook(FName Anim, EBEAim Aim) const;
};

UCLASS(BlueprintType)
class BERLINEXPENDABLES_API UBEWeaponLibrary : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Schlüssel "<id>_h" / "_u" / "_d". Pivot der Sprites = Griff, Socket "Muzzle" = Mündung. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BE") TMap<FName, TObjectPtr<UPaperSprite>> Sprites;

	UFUNCTION(BlueprintPure, Category = "BE")
	UPaperSprite* Find(FName WeaponId, EBEAim Aim) const;
};

UCLASS(BlueprintType)
class BERLINEXPENDABLES_API UBESpriteLibrary : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Einzelsprites: item_H, item_life, crate, barrel, cage, trabi, marker, alert ... */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BE") TMap<FName, TObjectPtr<UPaperSprite>> Sprites;
	/** Animationen: explosion, muzzle, smoke, wave, bottle, flame, pellet(1 Frame) ... */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BE") TMap<FName, TObjectPtr<UPaperFlipbook>> Flipbooks;

	UFUNCTION(BlueprintPure, Category = "BE") UPaperSprite* Sprite(FName Name) const { const TObjectPtr<UPaperSprite>* S = Sprites.Find(Name); return S ? S->Get() : nullptr; }
	UFUNCTION(BlueprintPure, Category = "BE") UPaperFlipbook* Flipbook(FName Name) const { const TObjectPtr<UPaperFlipbook>* F = Flipbooks.Find(Name); return F ? F->Get() : nullptr; }
};

USTRUCT(BlueprintType)
struct FBETileRect
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BE") FVector2D UV0 = FVector2D::ZeroVector;   // normalisiert
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BE") FVector2D UV1 = FVector2D::UnitVector;
};

UCLASS(BlueprintType)
class BERLINEXPENDABLES_API UBETileSet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BE") TObjectPtr<UTexture2D> Atlas;
	/** dirt, dirt_top, brick, brick_top, steel, concrete, platform, crate, back_plaster, back_window, back_brick, back_graffiti, crack_1..3 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BE") TMap<FName, FBETileRect> Rects;

	bool GetRect(FName Name, FBETileRect& Out) const { if (const FBETileRect* R = Rects.Find(Name)) { Out = *R; return true; } return false; }
};
