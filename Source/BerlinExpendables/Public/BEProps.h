// Requisiten: Gasflasche, Käfig + Gefangener, Item, Fluchttrabi.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BETypes.h"
#include "BEProps.generated.h"

class UPaperSpriteComponent;
class UPaperFlipbookComponent;

UCLASS()
class BERLINEXPENDABLES_API ABEBarrel : public AActor
{
	GENERATED_BODY()
public:
	ABEBarrel();
	FBEBody Box;
	UPROPERTY(BlueprintReadOnly, Category = "BE") bool bDead = false;
	void Init(FVector2D TopLeft);
	void Ignite(float Delay);
	virtual void Tick(float DeltaSeconds) override;
private:
	UPROPERTY() TObjectPtr<UPaperSpriteComponent> Sprite;
	float Fuse = -1.f;
};

UCLASS()
class BERLINEXPENDABLES_API ABECage : public AActor
{
	GENERATED_BODY()
public:
	ABECage();
	FBEBody Box;
	UPROPERTY(BlueprintReadOnly, Category = "BE") float Hp = 3.f;
	UPROPERTY(BlueprintReadOnly, Category = "BE") bool bDead = false;
	void Init(FVector2D TopLeft);
	void Hurt(float Damage, int32 RescuerSlot);
	void Open(int32 RescuerSlot);
private:
	UPROPERTY() TObjectPtr<UPaperSpriteComponent> CageSprite;
	UPROPERTY() TObjectPtr<UPaperFlipbookComponent> Captive;
};

UCLASS()
class BERLINEXPENDABLES_API ABECaptive : public AActor
{
	GENERATED_BODY()
public:
	ABECaptive();
	void Init(FVector2D TopLeft, int32 InFacing);
	virtual void Tick(float DeltaSeconds) override;
private:
	UPROPERTY() TObjectPtr<UPaperFlipbookComponent> Body;
	FBEBody Box; float VelY = 0.f; int32 Facing = 1; float Life = 1.8f;
};

UCLASS()
class BERLINEXPENDABLES_API ABEItem : public AActor
{
	GENERATED_BODY()
public:
	ABEItem();
	FBEBody Box;
	UPROPERTY(BlueprintReadOnly, Category = "BE") EBEItemKind Kind = EBEItemKind::Bombs;
	UPROPERTY(BlueprintReadOnly, Category = "BE") bool bDead = false;
	void Init(FVector2D TopLeft, EBEItemKind InKind);
	virtual void Tick(float DeltaSeconds) override;
	static FName SpriteName(EBEItemKind Kind);
	static FString DisplayName(EBEItemKind Kind);
private:
	UPROPERTY() TObjectPtr<UPaperSpriteComponent> Sprite;
	FVector2D Vel; float Life = 20.f; float Bob = 0.f;
	void Apply(class ABEBro* Bro);
};

UCLASS()
class BERLINEXPENDABLES_API ABEExit : public AActor
{
	GENERATED_BODY()
public:
	ABEExit();
	FBEBody Box;
	void Init(FVector2D TileTopLeft);
	virtual void Tick(float DeltaSeconds) override;
	UPROPERTY(BlueprintReadOnly, Category = "BE") float Leaving = -1.f;
private:
	UPROPERTY() TObjectPtr<UPaperSpriteComponent> Sprite;
	float PuffT = 0.f;
};
