#include "BEMook.h"
#include "BEData.h"
#include "BESettings.h"
#include "BEGameState.h"
#include "BETileWorld.h"
#include "BEBro.h"
#include "BEProjectile.h"
#include "BECombatLibrary.h"
#include "PaperFlipbookComponent.h"
#include "PaperSpriteComponent.h"

ABEMook::ABEMook()
{
	Facing = -1;
	Timer = 1.f + FMath::FRand() * 2.f;
	AlertSprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Alert"));
	AlertSprite->SetupAttachment(RootComponent);
	AlertSprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AlertSprite->SetVisibility(false);
}

void ABEMook::SetCharacter(UBECharacterData* InData)
{
	Super::SetCharacter(InData);
	if (Data) Hp = static_cast<float>(Data->Hp);
	if (UBESpriteLibrary* Lib = UBEGameSettings::Get()->SpriteLibrary.LoadSynchronous()) AlertSprite->SetSprite(Lib->Sprite(TEXT("alert")));
}

void ABEMook::Panic(float Seconds, int32 AwayFromX)
{
	State = EBEMookState::Panic; Timer = Seconds;
	Facing = Box.Pos.X < AwayFromX ? -1 : 1;
}

void ABEMook::Kill()
{
	if (bDead) return;
	bDead = true;
	Destroy();
}

bool ABEMook::LedgeOrWallAhead() const
{
	ABETileWorld* W = World(); if (!W) return true;
	const float AheadX = Facing > 0 ? Box.Pos.X + Box.Size.X + 4.f : Box.Pos.X - 4.f;
	const float Bottom = Box.Pos.Y + Box.Size.Y;
	return !W->IsSolidAt(FVector2D(AheadX, Bottom + 4.f)) || W->IsSolidAt(FVector2D(AheadX, Bottom - 8.f));
}

void ABEMook::FireAt()
{
	if (!Data) return;
	const FVector2D Mz = MuzzlePos();
	const FVector2D Dir = Aim == EBEAim::Up ? FVector2D(0.f, -1.f) : FVector2D(static_cast<float>(Facing), 0.f);
	const float Ang = FMath::Atan2(Dir.Y, Dir.X) + (FMath::FRand() - 0.5f) * 0.12f;
	const float Sp = Data->Weapon.Bullet.Speed;
	if (ABEProjectile* P = GetWorld()->SpawnActor<ABEProjectile>(ABEProjectile::StaticClass(), BE::ToWorld(Mz, BE::DepthProjectile), FRotator::ZeroRotator))
		P->Init(Mz, FVector2D(FMath::Cos(Ang) * Sp, FMath::Sin(Ang) * Sp), Data->Weapon.Bullet, false, -1);
	UBECombatLibrary::Flash(this, Mz, FLinearColor(1.f, 0.85f, 0.4f), 4.f, 100.f, 0.07f);
	UBECombatLibrary::PlaySound2D(this, Data->Weapon.Sound, 0.6f);
}

void ABEMook::SimStep(float Dt)
{
	const UBEGameSettings* S = UBEGameSettings::Get();
	ABEGameState* G = GS(); ABETileWorld* W = World();
	if (!G || !W || !Data || bDead) return;
	if (Stun > 0.f) { Stun -= Dt; Vel.X = 0.f; }

	float Dist = 0.f;
	ABEBro* Tgt = G->NearestAliveBro(Center(), &Dist);
	const float DX = Tgt ? Tgt->Center().X - Center().X : 0.f;
	const float DY = Tgt ? Tgt->Box.Pos.Y - Box.Pos.Y : 0.f;
	const bool bSees = Tgt && FMath::Abs(DX) < Data->SightRange && FMath::Abs(DY) < 112.f && (FMath::Sign(DX) == Facing || FMath::Abs(DX) < 72.f);

	if (Stun <= 0.f)
	{
		switch (State)
		{
		case EBEMookState::Patrol:
		{
			Timer -= Dt;
			const bool bWalking = Timer > 0.f || Timer < -1.f;
			if (Timer < -1.f) Timer = 1.5f + FMath::FRand() * 2.f;
			Vel.X = bWalking ? Facing * Data->WalkSpeed : 0.f;
			if (bWalking && bOnGround && LedgeOrWallAhead()) Facing *= -1;
			if (bSees) { State = EBEMookState::Alert; Timer = 0.37f; Facing = DX > 0.f ? 1 : -1; Vel.X = 0.f; }
			break;
		}
		case EBEMookState::Alert:
			Vel.X = 0.f;
			Timer -= Dt;
			if (Timer <= 0.f) { State = EBEMookState::Attack; ShootCd = 0.33f; Timer = 0.f; }
			break;
		case EBEMookState::Attack:
		{
			Vel.X = 0.f;
			if (!Tgt || FMath::Abs(DX) > Data->SightRange + 80.f || FMath::Abs(DY) > 180.f) { Timer += Dt; if (Timer > 1.f) State = EBEMookState::Patrol; break; }
			Timer = 0.f;
			if (BurstLeft <= 0) Facing = DX > 0.f ? 1 : -1;
			Aim = (DY < -60.f && FMath::Abs(DX) < 80.f) ? EBEAim::Up : EBEAim::Horizontal;
			if (BurstLeft > 0)
			{
				BurstTimer -= Dt;
				if (BurstTimer <= 0.f) { FireAt(); BurstLeft--; BurstTimer = Data->BurstGap; }
			}
			else
			{
				ShootCd -= Dt;
				if (ShootCd <= 0.f) { BurstLeft = Data->Burst; BurstTimer = 0.f; ShootCd = FMath::FRandRange(Data->CooldownRange.X, Data->CooldownRange.Y); }
			}
			break;
		}
		case EBEMookState::Panic:
			Vel.X = Facing * Data->WalkSpeed * 2.6f;
			if (bOnGround && LedgeOrWallAhead()) Facing *= -1;
			Timer -= Dt;
			if (Timer <= 0.f) State = EBEMookState::Patrol;
			break;
		}
	}

	Vel.Y = FMath::Min<double>(S->MaxFallSpeed, Vel.Y + S->Gravity * Dt);
	if (W->MoveX(Box, Vel.X * Dt)) { Vel.X = 0.f; if (State == EBEMookState::Patrol || State == EBEMookState::Panic) Facing *= -1; }
	if (W->MoveY(Box, Vel.Y * Dt)) { bOnGround = Vel.Y > 0.f; Vel.Y = 0.f; } else bOnGround = false;
	if (Box.Pos.Y > W->PixelHeight() + 80.f) Kill();
}

FName ABEMook::AnimName() const
{
	if (State == EBEMookState::Panic) return TEXT("panic");
	return Super::AnimName();
}

void ABEMook::UpdateVisuals()
{
	Super::UpdateVisuals();
	AlertSprite->SetVisibility(State == EBEMookState::Alert);
	if (State == EBEMookState::Alert) AlertSprite->SetRelativeLocation(FVector(0.f, -1.f, Box.Size.Y + 14.f + (Timer > 0.25f ? 4.f : 0.f)));
	if (Stun > 0.f) Body->SetSpriteColor(FLinearColor(1.f, 1.f, 1.f, 0.6f)); else Body->SetSpriteColor(FLinearColor::White);
}
