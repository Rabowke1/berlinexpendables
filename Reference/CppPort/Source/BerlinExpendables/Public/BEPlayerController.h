// Liest Tasten/Gamepad pro lokalem Spieler, baut Eingabeframes und meldet Beitritte. Controller 0 bedient beide Tastatur-Layouts.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BETypes.h"
#include "BEPlayerController.generated.h"

class ABEBro;

UCLASS()
class BERLINEXPENDABLES_API ABEPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ABEPlayerController();
	virtual void PlayerTick(float DeltaTime) override;
	virtual void BeginPlay() override;

	UPROPERTY(BlueprintReadOnly, Category = "BE") TObjectPtr<ABEBro> PrimaryBro;
	UPROPERTY(BlueprintReadOnly, Category = "BE") TObjectPtr<ABEBro> KeyboardTwoBro;

	int32 LocalIndex() const;

private:
	FBEInputFrame PrevPrimary, PrevKeyboardTwo;
	FBEInputFrame ReadLayout(const FBEKeyLayout& L, bool bGamepadSticks) const;
	void Dispatch(FBEInputFrame& Frame, FBEInputFrame& Prev, TObjectPtr<ABEBro>& Bro, EBEInputSource Source);
};
