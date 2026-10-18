// Baut die Welt aus dem Level-Asset, erzeugt lokale Spieler, verwaltet Beitritte und Neustart.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Engine/GameInstance.h"
#include "BETypes.h"
#include "BEGameMode.generated.h"

class ABEBro;
class ABEPlayerController;
class UBELevelAsset;
class UBECharacterData;

UCLASS()
class BERLINEXPENDABLES_API ABEGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ABEGameMode();
	virtual void BeginPlay() override;

	/** Spielerbeitritt per Sprung/Start. Gibt den neuen Bro zurück oder nullptr (voll / nicht im Spiel). */
	UFUNCTION(BlueprintCallable, Category = "BE") ABEBro* RequestJoin(ABEPlayerController* PC, EBEInputSource Source);
	UFUNCTION(BlueprintCallable, Category = "BE") void RestartMatch();

	/** Optionales Level-Override (sonst Settings.StartLevel). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BE") TObjectPtr<UBELevelAsset> LevelOverride;

private:
	void SpawnEntities();
	ABEBro* SpawnBro(int32 ControllerIndex, EBEInputSource Source, UBECharacterData* Data);
};

/** Merkt sich Beitritte über einen Neustart hinweg. */
USTRUCT()
struct FBEJoinRecord
{
	GENERATED_BODY()
	UPROPERTY() int32 ControllerIndex = 0;
	UPROPERTY() EBEInputSource Source = EBEInputSource::Controller;
};

UCLASS()
class BERLINEXPENDABLES_API UBEGameInstance : public UGameInstance
{
	GENERATED_BODY()
public:
	UPROPERTY() TArray<FBEJoinRecord> PersistentJoins;
	UPROPERTY() bool bRestarting = false;
};
