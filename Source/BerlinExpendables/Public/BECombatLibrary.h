// Explosionen, Effekte, Trümmer, Lichtblitze. Alles null-sicher: ohne Niagara-Assets gibt es 3D-Würfel-Trümmer + Punktlicht.
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameFramework/Actor.h"
#include "BETypes.h"
#include "BECombatLibrary.generated.h"

class ABEGameState;
class UNiagaraSystem;
class UStaticMeshComponent;
class UPointLightComponent;

UCLASS()
class BERLINEXPENDABLES_API UBECombatLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static ABEGameState* GS(const UObject* WorldContextObject);

	/** Radiale Explosion: Terrain, Gegner, Spieler, Fässer, Käfige, FX, Licht, Shake, Hitstop. */
	UFUNCTION(BlueprintCallable, Category = "BE", meta = (WorldContext = "WorldContextObject"))
	static void Explode(const UObject* WorldContextObject, FVector2D Center, float Radius, bool bByPlayer, int32 Slot, bool bFromBarrel = false);

	static void SpawnFX(const UObject* WorldContextObject, const TSoftObjectPtr<UNiagaraSystem>& System, FVector2D At, FLinearColor Tint = FLinearColor::White, float Scale = 1.f, float Depth = BE::DepthFront);
	static void SpawnDebris(const UObject* WorldContextObject, FVector2D At, int32 Count, FLinearColor Color, float Speed, float Size = 4.f);
	static void Flash(const UObject* WorldContextObject, FVector2D At, FLinearColor Color, float Intensity, float Radius, float Duration);
	static void Blood(const UObject* WorldContextObject, FVector2D At, int32 Count);
	static void Sparks(const UObject* WorldContextObject, FVector2D At, int32 Count);
	static void Dust(const UObject* WorldContextObject, FVector2D At, int32 Count);
	static void PlaySound2D(const UObject* WorldContextObject, USoundBase* Sound, float Volume = 1.f);
};

/** Kleiner 3D-Würfel, der im Levelraum fliegt, von Tiles abprallt und rotiert (Trümmer, Gibs-Ersatz). */
UCLASS()
class BERLINEXPENDABLES_API ABEDebris : public AActor
{
	GENERATED_BODY()

public:
	ABEDebris();
	void Init(FVector2D At, FVector2D Velocity, FLinearColor Color, float Size, float Life);
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Mesh;
	FVector2D Pos, Vel;
	FRotator Spin;
	float Life = 2.f, MaxLife = 2.f;
};

/** Punktlicht, das in Duration auf 0 abklingt und sich entfernt. */
UCLASS()
class BERLINEXPENDABLES_API ABEFlash : public AActor
{
	GENERATED_BODY()

public:
	ABEFlash();
	void Init(FLinearColor Color, float Intensity, float Radius, float Duration);
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY() TObjectPtr<UPointLightComponent> Light;
	float Duration = 0.3f, Elapsed = 0.f, StartIntensity = 0.f;
};
