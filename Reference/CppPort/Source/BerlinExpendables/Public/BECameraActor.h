// Orthografische Koop-Kamera: folgt der Bounding-Box aller lebenden Bros, klemmt ans Level, Shake per Trauma².
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BETypes.h"
#include "BECameraActor.generated.h"

class UCameraComponent;

UCLASS()
class BERLINEXPENDABLES_API ABECameraActor : public AActor
{
	GENERATED_BODY()

public:
	ABECameraActor();
	UPROPERTY(VisibleAnywhere, Category = "BE") TObjectPtr<UCameraComponent> Camera;

	/** Sichtfenster im Levelraum (obere linke Ecke, ohne Shake). */
	UPROPERTY(BlueprintReadOnly, Category = "BE") FVector2D View = FVector2D::ZeroVector;
	UPROPERTY(BlueprintReadOnly, Category = "BE") FVector2D ViewSize = FVector2D(640.f, 360.f);

	UFUNCTION(BlueprintCallable, Category = "BE") void Shake(float Amount) { Trauma = FMath::Min(1.f, Trauma + Amount); }
	UFUNCTION(BlueprintCallable, Category = "BE") void SnapTo(FVector2D Center);
	float Left() const { return View.X; }
	float Right() const { return View.X + ViewSize.X; }

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

private:
	float Trauma = 0.f;
	FVector2D ShakeOffset = FVector2D::ZeroVector;
	FRandomStream Rng;
	void ClampToLevel();
};
