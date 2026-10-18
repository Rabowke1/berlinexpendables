// Canvas-HUD ohne UMG-Assets: Spielerkarten, Leben, Punkte, Banner, Popups, Titel, Endbildschirm, Debug.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BEHUD.generated.h"

UCLASS()
class BERLINEXPENDABLES_API ABEHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	float Scale = 1.f;   // Bildschirmpixel pro Levelpixel
	void Text(const FString& S, float X, float Y, FLinearColor Color, float Size = 1.f, bool bCenter = false, bool bRight = false);
	FVector2D ToScreen(FVector2D LevelPos) const;
	void DrawRectFill(float X, float Y, float W, float H, FLinearColor Color);
};
