// Gemeinsame Basis für Bros und Gegner: AABB-Körper im Levelraum, Flipbook-Körper, Waffe am Hand-Socket.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "BETypes.h"
#include "BEPawnBase.generated.h"

class UPaperFlipbookComponent;
class UPaperSpriteComponent;
class UBECharacterData;
class ABETileWorld;
class ABEGameState;

UCLASS(Abstract)
class BERLINEXPENDABLES_API ABEPawnBase : public APawn
{
	GENERATED_BODY()

public:
	ABEPawnBase();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BE") TObjectPtr<UPaperFlipbookComponent> Body;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BE") TObjectPtr<UPaperSpriteComponent> WeaponSprite;
	UPROPERTY(BlueprintReadOnly, Category = "BE") TObjectPtr<UBECharacterData> Data;

	FBEBody Box;
	FVector2D Vel = FVector2D::ZeroVector;
	bool bOnGround = false;
	int32 Facing = 1;
	EBEAim Aim = EBEAim::Horizontal;
	bool bCrouch = false;

	UFUNCTION(BlueprintCallable, Category = "BE") virtual void SetCharacter(UBECharacterData* InData);
	UFUNCTION(BlueprintPure, Category = "BE") FVector2D Center() const { return Box.Center(); }
	UFUNCTION(BlueprintPure, Category = "BE") FVector2D Feet() const { return Box.Feet(); }
	/** Setzt die Position über den Fußpunkt (Mitte unten). */
	UFUNCTION(BlueprintCallable, Category = "BE") void SetFeet(FVector2D FeetPos);
	/** Mündung der aktuellen Waffe im Levelraum (Socket "Muzzle" des Waffensprites). */
	FVector2D MuzzlePos() const;
	void SetWeaponId(FName InWeaponId);

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;
	/** Fester Simulationsschritt (1/120 s). */
	virtual void SimStep(float Dt) {}
	virtual FName AnimName() const;
	virtual bool ShowWeapon() const { return true; }
	virtual void UpdateVisuals();
	void SetAnim(FName Anim);

	ABETileWorld* World() const;
	ABEGameState* GS() const;

	FName CurrentAnim = NAME_None;
	EBEAim CurrentAnimAim = EBEAim::Horizontal;
	FName WeaponId;
	float Accum = 0.f;
	static constexpr float Step = 1.f / 120.f;
	float BlinkTime = 0.f;   // > 0: Sichtbarkeit blinkt (Unverwundbarkeit)

private:
	void ApplyWeaponSprite();
};
