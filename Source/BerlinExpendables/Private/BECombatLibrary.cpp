#include "BECombatLibrary.h"
#include "BEGameState.h"
#include "BETileWorld.h"
#include "BEBro.h"
#include "BEMook.h"
#include "BEProps.h"
#include "BECameraActor.h"
#include "BESettings.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

ABEGameState* UBECombatLibrary::GS(const UObject* WorldContextObject)
{
	UWorld* W = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	return W ? W->GetGameState<ABEGameState>() : nullptr;
}

void UBECombatLibrary::Explode(const UObject* WorldContextObject, FVector2D Center, float Radius, bool bByPlayer, int32 Slot, bool bFromBarrel)
{
	ABEGameState* S = GS(WorldContextObject); if (!S) return;
	const UBEGameSettings* Cfg = UBEGameSettings::Get();

	if (S->TileWorld) S->TileWorld->DamageRadius(Center, Radius, 12.f);
	for (ABEMook* M : TArray<TObjectPtr<ABEMook>>(S->Mooks))
		if (IsValid(M) && !M->bDead && FVector2D::Distance(M->Center(), Center) < Radius + 12.f) S->HurtMook(M, 9.f, bByPlayer ? Slot : -1, EBEHitKind::Explosion);
	for (ABEBro* B : TArray<TObjectPtr<ABEBro>>(S->Bros))
		if (IsValid(B) && B->bAlive && FVector2D::Distance(B->Center(), Center) < Radius * 0.85f) S->KillBro(B, EBEHitKind::Explosion);
	for (ABEBarrel* Bl : S->Barrels)
		if (IsValid(Bl) && !Bl->bDead && FVector2D::Distance(Bl->Box.Center(), Center) < Radius + 20.f) Bl->Ignite(0.07f + FMath::FRand() * 0.13f);
	for (ABECage* C : TArray<TObjectPtr<ABECage>>(S->Cages))
		if (IsValid(C) && !C->bDead && FVector2D::Distance(C->Box.Center(), Center) < Radius + 12.f) C->Open(bByPlayer ? Slot : -1);

	const float Scale = Radius / 64.f;
	SpawnFX(WorldContextObject, Cfg->ExplosionFX, Center, FLinearColor(1.f, 0.55f, 0.16f), Scale);
	SpawnDebris(WorldContextObject, Center, 10, FLinearColor(1.f, 0.55f, 0.16f), 520.f, 5.f);
	SpawnDebris(WorldContextObject, Center, 8, FLinearColor(1.f, 0.9f, 0.47f), 420.f, 4.f);
	SpawnDebris(WorldContextObject, Center, 6, FLinearColor(0.43f, 0.42f, 0.53f), 200.f, 8.f);
	Flash(WorldContextObject, Center, FLinearColor(1.f, 0.6f, 0.2f), 40.f * Scale, Radius * 6.f, 0.35f);
	if (S->Camera) S->Camera->Shake(bFromBarrel ? 0.6f : 0.5f);
	S->Freeze(0.05f);
}

void UBECombatLibrary::SpawnFX(const UObject* WorldContextObject, const TSoftObjectPtr<UNiagaraSystem>& System, FVector2D At, FLinearColor Tint, float Scale, float Depth)
{
	if (System.IsNull()) return;
	UNiagaraSystem* Sys = System.LoadSynchronous(); if (!Sys) return;
	UWorld* W = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull); if (!W) return;
	UNiagaraComponent* C = UNiagaraFunctionLibrary::SpawnSystemAtLocation(W, Sys, BE::ToWorld(At, Depth), FRotator::ZeroRotator, FVector(Scale), true, true, ENCPoolMethod::AutoRelease);
	if (C) C->SetVariableLinearColor(FName(TEXT("Tint")), Tint);
}

void UBECombatLibrary::SpawnDebris(const UObject* WorldContextObject, FVector2D At, int32 Count, FLinearColor Color, float Speed, float Size)
{
	UWorld* W = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull); if (!W) return;
	for (int32 i = 0; i < Count; ++i)
	{
		const float A = FMath::FRand() * 2.f * PI, Sp = Speed * (0.3f + FMath::FRand() * 0.7f);
		if (ABEDebris* D = W->SpawnActor<ABEDebris>(ABEDebris::StaticClass(), BE::ToWorld(At, BE::DepthFront), FRotator::ZeroRotator))
			D->Init(At, FVector2D(FMath::Cos(A) * Sp, FMath::Sin(A) * Sp - Speed * 0.3f), Color, Size * (0.6f + FMath::FRand() * 0.8f), 0.8f + FMath::FRand() * 1.2f);
	}
}

void UBECombatLibrary::Flash(const UObject* WorldContextObject, FVector2D At, FLinearColor Color, float Intensity, float Radius, float Duration)
{
	UWorld* W = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull); if (!W) return;
	if (ABEFlash* F = W->SpawnActor<ABEFlash>(ABEFlash::StaticClass(), BE::ToWorld(At, -60.f), FRotator::ZeroRotator)) F->Init(Color, Intensity, Radius, Duration);
}

void UBECombatLibrary::Blood(const UObject* WorldContextObject, FVector2D At, int32 Count)
{
	const UBEGameSettings* Cfg = UBEGameSettings::Get();
	if (Cfg->bGore)
	{
		SpawnDebris(WorldContextObject, At, Count / 2, FLinearColor(0.89f, 0.23f, 0.23f), 360.f, 3.f);
		SpawnDebris(WorldContextObject, At, Count / 2, FLinearColor(0.62f, 0.1f, 0.1f), 300.f, 2.5f);
	}
	else
	{
		static const FLinearColor Confetti[] = { FLinearColor(1.f, 0.25f, 0.64f), FLinearColor(0.22f, 0.94f, 0.9f), FLinearColor(1.f, 0.9f, 0.47f), FLinearColor(0.5f, 0.82f, 0.31f) };
		for (int32 i = 0; i < 4; ++i) SpawnDebris(WorldContextObject, At, Count / 4, Confetti[i], 340.f, 3.f);
	}
	SpawnFX(WorldContextObject, Cfg->BloodFX, At);
}

void UBECombatLibrary::Sparks(const UObject* WorldContextObject, FVector2D At, int32 Count)
{
	SpawnDebris(WorldContextObject, At, Count, FLinearColor(1.f, 0.9f, 0.47f), 240.f, 2.f);
	SpawnFX(WorldContextObject, UBEGameSettings::Get()->SparksFX, At);
}

void UBECombatLibrary::Dust(const UObject* WorldContextObject, FVector2D At, int32 Count)
{
	SpawnDebris(WorldContextObject, At, Count, FLinearColor(0.62f, 0.61f, 0.69f), 70.f, 2.f);
	SpawnFX(WorldContextObject, UBEGameSettings::Get()->DustFX, At);
}

void UBECombatLibrary::PlaySound2D(const UObject* WorldContextObject, USoundBase* Sound, float Volume)
{
	if (Sound) UGameplayStatics::PlaySound2D(WorldContextObject, Sound, Volume);
}

// --- Trümmer -----------------------------------------------------------------------
ABEDebris::ABEDebris()
{
	PrimaryActorTick.bCanEverTick = true;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded()) Mesh->SetStaticMesh(Cube.Object);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Mat(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (Mat.Succeeded()) Mesh->SetMaterial(0, Mat.Object);
}

void ABEDebris::Init(FVector2D At, FVector2D Velocity, FLinearColor Color, float Size, float InLife)
{
	Pos = At; Vel = Velocity; Life = MaxLife = InLife;
	const UBEGameSettings* Cfg = UBEGameSettings::Get();
	if (UStaticMesh* M = Cfg->DebrisMesh.LoadSynchronous()) Mesh->SetStaticMesh(M);
	if (UMaterialInterface* MI = Cfg->DebrisMaterial.LoadSynchronous()) Mesh->SetMaterial(0, MI);
	if (UMaterialInstanceDynamic* MID = Mesh->CreateAndSetMaterialInstanceDynamic(0)) MID->SetVectorParameterValue(TEXT("Color"), Color);
	SetActorScale3D(FVector(Size / 100.f));
	Spin = FRotator(FMath::FRandRange(-540.f, 540.f), FMath::FRandRange(-540.f, 540.f), FMath::FRandRange(-540.f, 540.f));
	SetActorLocation(BE::ToWorld(Pos, BE::DepthFront));
}

void ABEDebris::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float Dt = FMath::Min(DeltaSeconds, 0.05f);
	ABEGameState* S = UBECombatLibrary::GS(this);
	ABETileWorld* W = S ? S->TileWorld.Get() : nullptr;
	Vel.Y += 1800.f * Dt;
	Pos.X += Vel.X * Dt;
	if (W && W->IsSolidAt(Pos)) { Pos.X -= Vel.X * Dt; Vel.X *= -0.4f; }
	Pos.Y += Vel.Y * Dt;
	if (W && W->IsSolidAt(Pos)) { Pos.Y -= Vel.Y * Dt; Vel.Y *= -0.3f; Vel.X *= 0.7f; Spin *= 0.5f; if (FMath::Abs(Vel.Y) < 40.f) Vel.Y = 0.f; }
	Life -= Dt;
	if (Life <= 0.f || (W && Pos.Y > W->PixelHeight() + 100.f)) { Destroy(); return; }
	AddActorLocalRotation(Spin * Dt);
	SetActorLocation(BE::ToWorld(Pos, BE::DepthFront));
	if (Life < 0.4f) SetActorScale3D(GetActorScale3D() * FMath::Max(0.f, 1.f - Dt * 2.f));
}

// --- Lichtblitz ----------------------------------------------------------------------
ABEFlash::ABEFlash()
{
	PrimaryActorTick.bCanEverTick = true;
	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	RootComponent = Light;
	Light->SetCastShadows(false);
	Light->bUseInverseSquaredFalloff = false;
	Light->LightFalloffExponent = 2.f;
	Light->SetMobility(EComponentMobility::Movable);
}

void ABEFlash::Init(FLinearColor Color, float Intensity, float Radius, float InDuration)
{
	Duration = FMath::Max(0.05f, InDuration); StartIntensity = Intensity;
	Light->SetLightColor(Color);
	Light->SetIntensity(Intensity);
	Light->SetAttenuationRadius(Radius);
}

void ABEFlash::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Elapsed += DeltaSeconds;
	const float T = FMath::Clamp(Elapsed / Duration, 0.f, 1.f);
	Light->SetIntensity(StartIntensity * FMath::Square(1.f - T));
	if (T >= 1.f) Destroy();
}
