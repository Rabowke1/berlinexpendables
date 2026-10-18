// Gegner-KI: Patrouille (dreht an Kanten), Alarm, Salven, Panik.
#pragma once

#include "CoreMinimal.h"
#include "BEPawnBase.h"
#include "BEMook.generated.h"

UENUM(BlueprintType)
enum class EBEMookState : uint8 { Patrol, Alert, Attack, Panic };

UCLASS()
class BERLINEXPENDABLES_API ABEMook : public ABEPawnBase
{
	GENERATED_BODY()

public:
	ABEMook();

	UPROPERTY(BlueprintReadOnly, Category = "BE") EBEMookState State = EBEMookState::Patrol;
	UPROPERTY(BlueprintReadOnly, Category = "BE") float Hp = 1.f;
	UPROPERTY(BlueprintReadOnly, Category = "BE") float Stun = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "BE") bool bDead = false;

	virtual void SetCharacter(UBECharacterData* InData) override;
	void Panic(float Seconds, int32 AwayFromX);
	void Kill();

protected:
	virtual void SimStep(float Dt) override;
	virtual FName AnimName() const override;
	virtual bool ShowWeapon() const override { return State != EBEMookState::Panic; }
	virtual void UpdateVisuals() override;

private:
	float Timer = 1.f, ShootCd = 1.f, BurstTimer = 0.f;
	int32 BurstLeft = 0;
	UPROPERTY() TObjectPtr<class UPaperSpriteComponent> AlertSprite;

	void FireAt();
	bool LedgeOrWallAhead() const;
};
