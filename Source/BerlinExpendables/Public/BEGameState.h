// Spielzustand: Leben, Punkte, Entity-Listen, Regeln (Tod, Respawn, Rettung) und Hitstop.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "BETypes.h"
#include "BEGameState.generated.h"

class ABETileWorld;
class ABECameraActor;
class ABEBro;
class ABEMook;
class ABEBarrel;
class ABECage;
class ABEItem;
class ABEExit;
class UBECharacterData;

UCLASS()
class BERLINEXPENDABLES_API ABEGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ABEGameState();

	UPROPERTY(BlueprintReadOnly, Category = "BE") int32 Lives = 5;
	UPROPERTY(BlueprintReadOnly, Category = "BE") int32 Score = 0;
	UPROPERTY(BlueprintReadOnly, Category = "BE") EBEMatchState MatchState = EBEMatchState::Playing;
	UPROPERTY(BlueprintReadOnly, Category = "BE") float StateTime = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "BE") float MatchTime = 0.f;
	UPROPERTY(BlueprintReadWrite, Category = "BE") bool bDebug = false;

	UPROPERTY(BlueprintReadOnly, Category = "BE") TObjectPtr<ABETileWorld> TileWorld;
	UPROPERTY(BlueprintReadOnly, Category = "BE") TObjectPtr<ABECameraActor> Camera;
	UPROPERTY(BlueprintReadOnly, Category = "BE") TArray<TObjectPtr<ABEBro>> Bros;
	UPROPERTY(BlueprintReadOnly, Category = "BE") TArray<TObjectPtr<ABEMook>> Mooks;
	UPROPERTY(BlueprintReadOnly, Category = "BE") TArray<TObjectPtr<ABEBarrel>> Barrels;
	UPROPERTY(BlueprintReadOnly, Category = "BE") TArray<TObjectPtr<ABECage>> Cages;
	UPROPERTY(BlueprintReadOnly, Category = "BE") TArray<TObjectPtr<ABEItem>> Items;
	UPROPERTY(BlueprintReadOnly, Category = "BE") TArray<TObjectPtr<ABEExit>> Exits;
	UPROPERTY(BlueprintReadOnly, Category = "BE") TArray<FBEPopup> Popups;

	UFUNCTION(BlueprintCallable, Category = "BE") void AddScore(int32 Amount, FVector2D At, bool bShow = true);
	UFUNCTION(BlueprintCallable, Category = "BE") void AddPopup(const FString& Text, FVector2D At, FLinearColor Color);
	/** Hitstop: Zeitlupe für kurze Echtzeit. */
	UFUNCTION(BlueprintCallable, Category = "BE") void Freeze(float Seconds);

	UFUNCTION() void HandleTileDestroyed(int32 TX, int32 TY, EBETile Tile, FVector2D Center);

	void KillBro(ABEBro* Bro, EBEHitKind Kind);
	void HurtMook(ABEMook* Mook, float Damage, int32 Slot, EBEHitKind Kind);
	void Rescue(ABECage* Cage, int32 RescuerSlot);
	void TryRespawn(ABEBro* Bro);
	void Win();
	void GameOver();

	ABEBro* NearestAliveBro(FVector2D From, float* OutDistance = nullptr) const;
	ABEBro* AnyAliveBro() const;
	int32 AliveBroCount() const;
	UBECharacterData* PickCharacter(const ABEBro* For, bool bRandom) const;
	void CleanupLists();

	virtual void Tick(float DeltaSeconds) override;

private:
	float FreezeUntilRealTime = 0.f;
	bool bFrozen = false;
};
