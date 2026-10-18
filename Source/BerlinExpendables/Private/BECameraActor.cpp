#include "BECameraActor.h"
#include "BEGameState.h"
#include "BEBro.h"
#include "BEProps.h"
#include "BETileWorld.h"
#include "BESettings.h"
#include "Camera/CameraComponent.h"

ABECameraActor::ABECameraActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics;   // nach den Pawns
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(RootComponent);
	Camera->ProjectionMode = ECameraProjectionMode::Orthographic;
	Camera->OrthoWidth = 640.f;
	Camera->bConstrainAspectRatio = true;
	Camera->AspectRatio = 16.f / 9.f;
	Camera->OrthoNearClipPlane = 10.f;
	Camera->OrthoFarClipPlane = 5000.f;
	Camera->SetRelativeRotation(FRotator(0.f, 90.f, 0.f));   // Blick entlang +Y
	Camera->PostProcessSettings.bOverride_BloomIntensity = true;
	Camera->PostProcessSettings.BloomIntensity = 0.55f;
	Camera->PostProcessSettings.bOverride_AutoExposureMinBrightness = true;
	Camera->PostProcessSettings.bOverride_AutoExposureMaxBrightness = true;
	Camera->PostProcessSettings.AutoExposureMinBrightness = 1.f;
	Camera->PostProcessSettings.AutoExposureMaxBrightness = 1.f;
	Camera->PostProcessSettings.bOverride_VignetteIntensity = true;
	Camera->PostProcessSettings.VignetteIntensity = 0.25f;
	Rng.Initialize(1337);
}

void ABECameraActor::BeginPlay()
{
	Super::BeginPlay();
	const UBEGameSettings* S = UBEGameSettings::Get();
	ViewSize = FVector2D(S->ViewWidth, S->ViewHeight);
	Camera->OrthoWidth = S->ViewWidth;
	Camera->AspectRatio = S->ViewWidth / S->ViewHeight;
}

void ABECameraActor::ClampToLevel()
{
	ABEGameState* GS = GetWorld() ? GetWorld()->GetGameState<ABEGameState>() : nullptr;
	if (!GS || !GS->TileWorld) return;
	View.X = FMath::Clamp(View.X, 0.f, FMath::Max(0.f, GS->TileWorld->PixelWidth() - ViewSize.X));
	View.Y = FMath::Clamp(View.Y, 0.f, FMath::Max(0.f, GS->TileWorld->PixelHeight() - ViewSize.Y));
}

void ABECameraActor::SnapTo(FVector2D Center)
{
	View = Center - ViewSize * 0.5f;
	ClampToLevel();
	SetActorLocation(BE::ToWorld(View + ViewSize * 0.5f, -1000.f));
}

void ABECameraActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ABEGameState* GS = GetWorld()->GetGameState<ABEGameState>();
	if (!GS) return;

	TArray<FVector2D> Points;
	if (GS->MatchState != EBEMatchState::Won)
		for (ABEBro* B : GS->Bros) if (IsValid(B) && B->bAlive) Points.Add(B->Center());
	if (Points.Num() == 0 && GS->MatchState == EBEMatchState::Won && GS->Exits.Num() && IsValid(GS->Exits[0])) Points.Add(GS->Exits[0]->Box.Center());
	if (Points.Num() == 0 && GS->Bros.Num() == 0 && GS->TileWorld) Points.Add(GS->TileWorld->SpawnPoint());

	if (Points.Num())
	{
		FVector2D Min(TNumericLimits<float>::Max(), TNumericLimits<float>::Max()), Max(-TNumericLimits<float>::Max(), -TNumericLimits<float>::Max());
		for (const FVector2D& P : Points) { Min = FVector2D::Min(Min, P); Max = FVector2D::Max(Max, P); }
		const FVector2D Target = (Min + Max) * 0.5f - ViewSize * 0.5f - FVector2D(0.f, 40.f);
		const float KX = 1.f - FMath::Pow(0.88f, DeltaSeconds * 60.f), KY = 1.f - FMath::Pow(0.9f, DeltaSeconds * 60.f);
		View.X += (Target.X - View.X) * KX;
		View.Y += (Target.Y - View.Y) * KY;
	}
	ClampToLevel();

	Trauma = FMath::Max(0.f, Trauma - 2.1f * DeltaSeconds);
	const float T = Trauma * Trauma;
	ShakeOffset = FVector2D(FMath::RoundToFloat(Rng.FRandRange(-1.f, 1.f) * 12.f * T), FMath::RoundToFloat(Rng.FRandRange(-1.f, 1.f) * 8.f * T));

	const FVector2D C(FMath::RoundToFloat(View.X + ViewSize.X * 0.5f + ShakeOffset.X), FMath::RoundToFloat(View.Y + ViewSize.Y * 0.5f + ShakeOffset.Y));
	SetActorLocation(BE::ToWorld(C, -1000.f));
}
