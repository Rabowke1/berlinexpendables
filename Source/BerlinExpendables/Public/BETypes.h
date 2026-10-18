// Gemeinsame Typen. Simulation läuft im "Levelraum": x nach rechts, y nach unten, 1 Einheit = 1 Texel.
// Umrechnung nach Unreal: X = x, Z = -y, Y = Tiefe (Kamera schaut entlang +Y).
#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "BETypes.generated.h"

class UPaperFlipbook;
class UPaperSprite;
class UNiagaraSystem;
class USoundBase;

UENUM(BlueprintType)
enum class EBETile : uint8 { Air = 0, Dirt, Brick, Steel, Concrete, Platform, Crate };

UENUM(BlueprintType)
enum class EBEBack : uint8 { None = 0, Plaster, Window, Brick, Graffiti };

UENUM(BlueprintType)
enum class EBEAim : uint8 { Horizontal, Up, Down };

UENUM(BlueprintType)
enum class EBEInputSource : uint8 { Controller, KeyboardTwo };

UENUM(BlueprintType)
enum class EBEMatchState : uint8 { Playing, Won, GameOver };

UENUM(BlueprintType)
enum class EBEItemKind : uint8 { Heavy, Rocket, Flame, Laser, Bombs, Life };

UENUM(BlueprintType)
enum class EBEHitKind : uint8 { Shot, Melee, Explosion, Touch, Fall };

namespace BE
{
	/** Levelraum -> Unreal. Depth > 0 liegt weiter hinten. */
	FORCEINLINE FVector ToWorld(const FVector2D& P, float Depth = 0.f) { return FVector(P.X, Depth, -P.Y); }
	FORCEINLINE FVector2D ToLevel(const FVector& V) { return FVector2D(V.X, -V.Z); }
	constexpr float DepthGameplay = -2.f;
	constexpr float DepthFront = -10.f;
	constexpr float DepthProjectile = -5.f;
	constexpr float DepthBackTiles = 20.f;
	constexpr float DepthEarth = 30.f;
}

/** Ein Simulationsschritt an Eingabe für einen Bro (von Tastatur-Layout oder Gamepad). */
USTRUCT(BlueprintType)
struct FBEInputFrame
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "BE") bool bLeft = false;
	UPROPERTY(BlueprintReadOnly, Category = "BE") bool bRight = false;
	UPROPERTY(BlueprintReadOnly, Category = "BE") bool bUp = false;
	UPROPERTY(BlueprintReadOnly, Category = "BE") bool bDown = false;
	UPROPERTY(BlueprintReadOnly, Category = "BE") bool bJump = false;
	UPROPERTY(BlueprintReadOnly, Category = "BE") bool bFire = false;
	UPROPERTY(BlueprintReadOnly, Category = "BE") bool bSpecial = false;
	UPROPERTY(BlueprintReadOnly, Category = "BE") bool bMelee = false;
	UPROPERTY(BlueprintReadOnly, Category = "BE") bool bStart = false;
	UPROPERTY(BlueprintReadOnly, Category = "BE") bool bJumpPressed = false;
	UPROPERTY(BlueprintReadOnly, Category = "BE") bool bFirePressed = false;
	UPROPERTY(BlueprintReadOnly, Category = "BE") bool bSpecialPressed = false;
	UPROPERTY(BlueprintReadOnly, Category = "BE") bool bMeleePressed = false;
	UPROPERTY(BlueprintReadOnly, Category = "BE") bool bStartPressed = false;

	/** Flankenerkennung gegenüber dem vorherigen Frame. */
	void ComputeEdges(const FBEInputFrame& Prev)
	{
		bJumpPressed = bJump && !Prev.bJump;
		bFirePressed = bFire && !Prev.bFire;
		bSpecialPressed = bSpecial && !Prev.bSpecial;
		bMeleePressed = bMelee && !Prev.bMelee;
		bStartPressed = bStart && !Prev.bStart;
	}
};

/** Axis-Aligned-Box im Levelraum (Pos = obere linke Ecke). */
struct FBEBody
{
	FVector2D Pos = FVector2D::ZeroVector;
	FVector2D Size = FVector2D(20.f, 44.f);
	float DropThrough = 0.f;   // Sekunden, in denen Plattformen ignoriert werden

	FVector2D Center() const { return Pos + Size * 0.5f; }
	FVector2D Feet() const { return FVector2D(Pos.X + Size.X * 0.5f, Pos.Y + Size.Y); }
	bool Overlaps(const FBEBody& O) const
	{
		return Pos.X < O.Pos.X + O.Size.X && Pos.X + Size.X > O.Pos.X && Pos.Y < O.Pos.Y + O.Size.Y && Pos.Y + Size.Y > O.Pos.Y;
	}
	bool Contains(const FVector2D& P, float Pad = 0.f) const
	{
		return P.X >= Pos.X - Pad && P.X <= Pos.X + Size.X + Pad && P.Y >= Pos.Y - Pad && P.Y <= Pos.Y + Size.Y + Pad;
	}
};

USTRUCT(BlueprintType)
struct FBEBulletProfile
{
	GENERATED_BODY()

	/** Projektilgrafik (1 oder mehr Frames). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BE") TObjectPtr<UPaperFlipbook> Flipbook = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BE") float Speed = 960.f;        // uu/s
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BE") float LifeTime = 1.f;       // s
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BE") float Damage = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BE") float TileDamage = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BE") int32 Pierce = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BE") float Gravity = 0.f;        // uu/s²
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BE") float Lob = 0.f;            // zusätzlicher Aufwärtsimpuls uu/s
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BE") float ExplodeRadius = 0.f;  // uu
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BE") bool bSpin = false;         // Flasche rotiert
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BE") TObjectPtr<UNiagaraSystem> ImpactFX = nullptr;
};

USTRUCT(BlueprintType)
struct FBEWeaponProfile
{
	GENERATED_BODY()

	/** Name der Waffensprites in der Waffenbibliothek (<Id>_h / _u / _d). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BE") FName WeaponId = TEXT("pistol");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BE") float Cooldown = 0.1f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BE") int32 Count = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BE") float Spread = 0.f;      // rad
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BE") float Recoil = 0.f;      // uu/s
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BE") float Shake = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BE") FBEBulletProfile Bullet;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BE") TObjectPtr<UNiagaraSystem> MuzzleFX = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BE") TObjectPtr<USoundBase> Sound = nullptr;
};

USTRUCT(BlueprintType)
struct FBEKeyLayout
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BE") FKey Left;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BE") FKey Right;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BE") FKey Up;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BE") FKey Down;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BE") FKey Jump;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BE") FKey Fire;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BE") FKey Special;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BE") FKey Melee;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BE") FKey Start;
};

/** Spawn-Eintrag aus dem ASCII-Level. */
USTRUCT(BlueprintType)
struct FBEEntitySpawn
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "BE") FName Type;    // mook, security, barrel, cage, exit, item, life
	UPROPERTY(BlueprintReadOnly, Category = "BE") int32 TX = 0;
	UPROPERTY(BlueprintReadOnly, Category = "BE") int32 TY = 0;
	UPROPERTY(BlueprintReadOnly, Category = "BE") FString Arg;   // Itembuchstabe
};

/** Schwebender Text (HUD). */
USTRUCT(BlueprintType)
struct FBEPopup
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "BE") FString Text;
	UPROPERTY(BlueprintReadOnly, Category = "BE") FVector2D Pos = FVector2D::ZeroVector;
	UPROPERTY(BlueprintReadOnly, Category = "BE") FLinearColor Color = FLinearColor::White;
	UPROPERTY(BlueprintReadOnly, Category = "BE") float Life = 0.9f;
};
