// Parallax-Ebene: kachelt ein Sprite horizontal und bewegt sich mit Faktor relativ zur Kamera.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BEParallaxLayer.generated.h"

class UPaperSprite;
class UPaperSpriteComponent;

UCLASS()
class BERLINEXPENDABLES_API ABEParallaxLayer : public AActor
{
	GENERATED_BODY()

public:
	ABEParallaxLayer();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BE") TObjectPtr<UPaperSprite> Sprite;
	/** 0 = steht still (Himmel), 1 = bewegt sich wie die Welt. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BE", meta = (ClampMin = 0, ClampMax = 1)) float Factor = 0.2f;
	/** Unterkante im Levelraum (y nach unten). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BE") float BottomY = 720.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BE") float Depth = 400.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BE") int32 Copies = 4;

	UFUNCTION(BlueprintCallable, Category = "BE") void Rebuild();
	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY() TArray<TObjectPtr<UPaperSpriteComponent>> Tiles;
	float SpriteWidth = 960.f, SpriteHeight = 280.f;
};
