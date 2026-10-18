#include "BEProjectile.h"
#include "BEGameState.h"
#include "BETileWorld.h"
#include "BEBro.h"
#include "BEMook.h"
#include "BEProps.h"
#include "BESettings.h"
#include "BEData.h"
#include "BECombatLibrary.h"
#include "NiagaraSystem.h"
#include "PaperFlipbookComponent.h"
#include "PaperSpriteComponent.h"
#include "PaperFlipbook.h"

ABEProjectile::ABEProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Sprite = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("Sprite"));
	Sprite->SetupAttachment(RootComponent);
	Sprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Sprite->SetCastShadow(false);
}

void ABEProjectile::Init(FVector2D At, FVector2D Velocity, const FBEBulletProfile& InProfile, bool bInByPlayer, int32 InSlot)
{
	Profile = InProfile; bByPlayer = bInByPlayer; Slot = InSlot;
	Pos = At; Vel = Velocity; Vel.Y -= Profile.Lob;
	Life = Profile.LifeTime; Pierce = Profile.Pierce;
	if (Profile.Flipbook) Sprite->SetFlipbook(Profile.Flipbook);
	UpdateVisual();
}

void ABEProjectile::Impact()
{
	if (Profile.ExplodeRadius > 0.f) UBECombatLibrary::Explode(this, Pos, Profile.ExplodeRadius, bByPlayer, Slot);
	else if (Profile.bSpin) UBECombatLibrary::Sparks(this, Pos, 6);
	UBECombatLibrary::SpawnFX(this, TSoftObjectPtr<UNiagaraSystem>(Profile.ImpactFX), Pos);
	Destroy();
}

void ABEProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float Dt = FMath::Min(DeltaSeconds, 0.05f);
	ABEGameState* S = UBECombatLibrary::GS(this);
	ABETileWorld* W = S ? S->TileWorld.Get() : nullptr;
	if (!S || !W) { Destroy(); return; }

	Vel.Y += Profile.Gravity * Dt;
	const int32 Steps = FMath::Max(1, FMath::CeilToInt(FMath::Max(FMath::Abs(Vel.X), FMath::Abs(Vel.Y)) * Dt / 8.f));
	const FVector2D StepV = Vel * Dt / Steps;
	for (int32 i = 0; i < Steps; ++i)
	{
		Pos += StepV;
		const int32 TX = FMath::FloorToInt(Pos.X / W->GetTileSize()), TY = FMath::FloorToInt(Pos.Y / W->GetTileSize());
		if (W->IsSolidTile(W->GetTile(TX, TY)))
		{
			if (Profile.TileDamage > 0.f) W->DamageTile(TX, TY, Profile.TileDamage);
			UBECombatLibrary::Sparks(this, Pos - StepV, 3);
			Impact(); return;
		}
		if (bByPlayer)
		{
			for (ABEMook* M : TArray<TObjectPtr<ABEMook>>(S->Mooks))
			{
				if (!IsValid(M) || M->bDead || Hit.Contains(M) || !M->Box.Contains(Pos, 4.f)) continue;
				Hit.Add(M);
				S->HurtMook(M, Profile.Damage, Slot, EBEHitKind::Shot);
				if (Pierce-- <= 0) { Impact(); return; }
			}
			for (ABEBarrel* B : S->Barrels)
			{
				if (!IsValid(B) || B->bDead || Hit.Contains(B) || !B->Box.Contains(Pos)) continue;
				Hit.Add(B); B->Ignite(0.03f);
				if (Pierce-- <= 0) { Impact(); return; }
			}
			for (ABECage* C : TArray<TObjectPtr<ABECage>>(S->Cages))
			{
				if (!IsValid(C) || C->bDead || Hit.Contains(C) || !C->Box.Contains(Pos)) continue;
				Hit.Add(C); C->Hurt(Profile.Damage, Slot);
				if (Pierce-- <= 0) { Impact(); return; }
			}
		}
		else
		{
			for (ABEBro* B : TArray<TObjectPtr<ABEBro>>(S->Bros))
			{
				if (!IsValid(B) || !B->bAlive || B->Invulnerable > 0.f || !B->Box.Contains(Pos)) continue;
				S->KillBro(B, EBEHitKind::Shot);
				Destroy(); return;
			}
		}
	}
	Life -= Dt;
	if (Life <= 0.f) { if (Profile.ExplodeRadius > 0.f) Impact(); else Destroy(); return; }
	if (Pos.Y > W->PixelHeight() + 100.f || Pos.X < -100.f || Pos.X > W->PixelWidth() + 100.f) { Destroy(); return; }
	SpinAngle += 720.f * Dt;
	UpdateVisual();
}

void ABEProjectile::UpdateVisual()
{
	SetActorLocation(BE::ToWorld(Pos, BE::DepthProjectile));
	if (Profile.bSpin) { SetActorRotation(FRotator(SpinAngle, 0.f, 0.f)); return; }
	const bool bVertical = FMath::Abs(Vel.Y) > FMath::Abs(Vel.X) * 1.5f;
	if (bVertical) { SetActorRotation(FRotator(Vel.Y < 0.f ? 90.f : -90.f, 0.f, 0.f)); Sprite->SetRelativeScale3D(FVector(1.f)); }
	else { SetActorRotation(FRotator::ZeroRotator); Sprite->SetRelativeScale3D(FVector(Vel.X < 0.f ? -1.f : 1.f, 1.f, 1.f)); }
}

// --- Granate --------------------------------------------------------------------
ABEGrenade::ABEGrenade()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Sprite"));
	Sprite->SetupAttachment(RootComponent);
	Sprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Sprite->SetCastShadow(false);
}

void ABEGrenade::Init(FVector2D At, FVector2D Velocity, float InRadius, bool bInByPlayer, int32 InSlot)
{
	Box.Size = FVector2D(10.f, 10.f); Box.Pos = At - Box.Size * 0.5f;
	Vel = Velocity; Radius = InRadius; bByPlayer = bInByPlayer; Slot = InSlot;
	if (UBESpriteLibrary* Lib = UBEGameSettings::Get()->SpriteLibrary.LoadSynchronous()) Sprite->SetSprite(Lib->Sprite(TEXT("grenade")));
	SetActorLocation(BE::ToWorld(Box.Center(), BE::DepthProjectile));
}

void ABEGrenade::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float Dt = FMath::Min(DeltaSeconds, 0.05f);
	ABEGameState* S = UBECombatLibrary::GS(this);
	ABETileWorld* W = S ? S->TileWorld.Get() : nullptr;
	if (!S || !W) { Destroy(); return; }

	Vel.Y = FMath::Min(720.f, Vel.Y + 1800.f * Dt);
	if (W->MoveX(Box, Vel.X * Dt)) Vel.X *= -0.5f;
	if (W->MoveY(Box, Vel.Y * Dt))
	{
		if (Vel.Y > 0.f) { Vel.Y = -Vel.Y * 0.45f; Vel.X *= 0.75f; if (FMath::Abs(Vel.Y) < 108.f) Vel.Y = 0.f; }
		else Vel.Y = 0.f;
	}
	Fuse -= Dt; SmokeT -= Dt;
	if (SmokeT <= 0.f) { SmokeT = 0.17f; UBECombatLibrary::Dust(this, Box.Center() - FVector2D(0.f, 6.f), 1); }
	if (Fuse <= 0.f || Box.Pos.Y > W->PixelHeight() + 80.f)
	{
		UBECombatLibrary::Explode(this, Box.Center(), Radius, bByPlayer, Slot);
		Destroy(); return;
	}
	SetActorLocation(BE::ToWorld(Box.Center(), BE::DepthProjectile));
	AddActorLocalRotation(FRotator(-Vel.X * Dt * 4.f, 0.f, 0.f));
	const bool bHot = Fuse < 0.4f && FMath::Fmod(Fuse * 10.f, 2.f) < 1.f;
	Sprite->SetSpriteColor(bHot ? FLinearColor(3.f, 3.f, 3.f) : FLinearColor::White);
}
