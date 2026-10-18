// Projektile (Kugeln, Wellen, Flaschen, Raketen) und Granaten.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BETypes.h"
#include "BEProjectile.generated.h"

class UPaperFlipbookComponent;
class UPaperSpriteComponent;

UCLASS()
class BERLINEXPENDABLES_API ABEProjectile : public AActor
{
	GENERATED_BODY()

public:
	ABEProjectile();
	void Init(FVector2D At, FVector2D Velocity, const FBEBulletProfile& InProfile, bool bInByPlayer, int32 InSlot);
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(BlueprintReadOnly, Category = "BE") FBEBulletProfile Profile;
	UPROPERTY(BlueprintReadOnly, Category = "BE") bool bByPlayer = true;
	UPROPERTY(BlueprintReadOnly, Category = "BE") int32 Slot = -1;

private:
	UPROPERTY() TObjectPtr<UPaperFlipbookComponent> Sprite;
	FVector2D Pos, Vel;
	float Life = 1.f;
	int32 Pierce = 0;
	float SpinAngle = 0.f;
	TSet<FObjectKey> Hit;
	void Impact();
	void UpdateVisual();
};

UCLASS()
class BERLINEXPENDABLES_API ABEGrenade : public AActor
{
	GENERATED_BODY()

public:
	ABEGrenade();
	void Init(FVector2D At, FVector2D Velocity, float InRadius, bool bInByPlayer, int32 InSlot);
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY() TObjectPtr<UPaperSpriteComponent> Sprite;
	FBEBody Box;
	FVector2D Vel;
	float Radius = 64.f, Fuse = 1.17f, SmokeT = 0.f;
	bool bByPlayer = true; int32 Slot = -1;
};
