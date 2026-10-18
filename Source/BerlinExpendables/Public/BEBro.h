// Spielbarer Bro: Laufen, Springen (Coyote/Buffer/variable Höhe), Ducken, Zielen, Feuern, Granaten, Nahkampf, Pickups.
#pragma once

#include "CoreMinimal.h"
#include "BEPawnBase.h"
#include "BEBro.generated.h"

USTRUCT(BlueprintType)
struct FBEPickupState
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category = "BE") bool bActive = false;
	UPROPERTY(BlueprintReadOnly, Category = "BE") EBEItemKind Kind = EBEItemKind::Heavy;
	UPROPERTY(BlueprintReadOnly, Category = "BE") int32 Ammo = 0;
};

UCLASS()
class BERLINEXPENDABLES_API ABEBro : public ABEPawnBase
{
	GENERATED_BODY()

public:
	ABEBro();

	UPROPERTY(BlueprintReadOnly, Category = "BE") int32 Slot = 0;
	UPROPERTY(BlueprintReadOnly, Category = "BE") int32 ControllerIndex = 0;
	UPROPERTY(BlueprintReadOnly, Category = "BE") EBEInputSource Source = EBEInputSource::Controller;
	UPROPERTY(BlueprintReadOnly, Category = "BE") bool bAlive = true;
	UPROPERTY(BlueprintReadOnly, Category = "BE") float DeadTimer = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "BE") float Invulnerable = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "BE") int32 Grenades = 3;
	UPROPERTY(BlueprintReadOnly, Category = "BE") FBEPickupState Pickup;
	UPROPERTY(BlueprintReadOnly, Category = "BE") int32 Kills = 0;
	UPROPERTY(BlueprintReadOnly, Category = "BE") float Banner = 0.f;   // Namens-Banner nach Spawn (s)

	void SetInput(const FBEInputFrame& Frame);
	const FBEWeaponProfile& Weapon() const;
	FLinearColor SlotColor() const;
	void GivePickup(EBEItemKind Kind, int32 Ammo);
	void Die();
	void RespawnAt(FVector2D FeetPos);
	void SwitchCharacter(UBECharacterData* NewData);
	virtual void SetCharacter(UBECharacterData* InData) override;

protected:
	virtual void SimStep(float Dt) override;
	virtual FName AnimName() const override;
	virtual bool ShowWeapon() const override { return MeleeT <= 0.f; }
	virtual void UpdateVisuals() override;

private:
	FBEInputFrame Input;
	float Coyote = 0.f, JumpBuffer = 0.f, FireCd = 0.f, MuzzleT = 0.f, SpecialCd = 0.f, MeleeT = 0.f;
	float AnimClock = 0.f;
	FBEWeaponProfile PickupProfile;

	void SetCrouch(bool bOn);
	bool EnemyInReach() const;
	void Fire();
	void ThrowGrenade();
	void MeleeHit();
	void BuildPickupProfile();
};
