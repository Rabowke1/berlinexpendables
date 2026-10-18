#include "BEBro.h"
#include "BEData.h"
#include "BESettings.h"
#include "BEGameState.h"
#include "BETileWorld.h"
#include "BECameraActor.h"
#include "BEMook.h"
#include "BEProps.h"
#include "BEProjectile.h"
#include "BECombatLibrary.h"
#include "PaperFlipbookComponent.h"
#include "PaperSpriteComponent.h"
#include "PaperFlipbook.h"

ABEBro::ABEBro() {}

void ABEBro::SetCharacter(UBECharacterData* InData)
{
	Super::SetCharacter(InData);
	if (Data) Grenades = FMath::Max(Grenades, Data->Grenades);
	if (Pickup.bActive) SetWeaponId(PickupProfile.WeaponId);
}

void ABEBro::SetInput(const FBEInputFrame& F)
{
	const bool J = Input.bJumpPressed || F.bJumpPressed, Fi = Input.bFirePressed || F.bFirePressed;
	const bool Sp = Input.bSpecialPressed || F.bSpecialPressed, Me = Input.bMeleePressed || F.bMeleePressed, St = Input.bStartPressed || F.bStartPressed;
	Input = F;
	Input.bJumpPressed = J; Input.bFirePressed = Fi; Input.bSpecialPressed = Sp; Input.bMeleePressed = Me; Input.bStartPressed = St;
}

const FBEWeaponProfile& ABEBro::Weapon() const
{
	static const FBEWeaponProfile Empty;
	if (Pickup.bActive) return PickupProfile;
	return Data ? Data->Weapon : Empty;
}

FLinearColor ABEBro::SlotColor() const
{
	static const FLinearColor C[] = { FLinearColor(0.18f, 0.5f, 0.93f), FLinearColor(0.89f, 0.23f, 0.23f), FLinearColor(0.5f, 0.82f, 0.31f), FLinearColor(1.f, 0.81f, 0.36f) };
	return C[FMath::Clamp(Slot, 0, 3)];
}

void ABEBro::BuildPickupProfile()
{
	UBESpriteLibrary* Lib = UBEGameSettings::Get()->SpriteLibrary.LoadSynchronous();
	auto FB = [Lib](const TCHAR* N) { return Lib ? Lib->Flipbook(FName(N)) : nullptr; };
	FBEWeaponProfile P;
	switch (Pickup.Kind)
	{
	case EBEItemKind::Heavy:
		P.WeaponId = TEXT("heavy"); P.Cooldown = 0.067f; P.Spread = 0.12f; P.Recoil = 30.f; P.Shake = 0.08f;
		P.Bullet.Flipbook = FB(TEXT("heavybullet")); P.Bullet.Speed = 1080.f; P.Bullet.LifeTime = 1.f; P.Bullet.Damage = 2.f; P.Bullet.TileDamage = 2.f; break;
	case EBEItemKind::Rocket:
		P.WeaponId = TEXT("rocket"); P.Cooldown = 0.5f; P.Recoil = 120.f; P.Shake = 0.2f;
		P.Bullet.Flipbook = FB(TEXT("rocket")); P.Bullet.Speed = 600.f; P.Bullet.LifeTime = 1.5f; P.Bullet.TileDamage = 3.f; P.Bullet.ExplodeRadius = 76.f; break;
	case EBEItemKind::Flame:
		P.WeaponId = TEXT("flame"); P.Cooldown = 0.05f; P.Spread = 0.15f; P.Recoil = 12.f; P.Shake = 0.03f;
		P.Bullet.Flipbook = FB(TEXT("flame")); P.Bullet.Speed = 432.f; P.Bullet.LifeTime = 0.4f; P.Bullet.TileDamage = 0.35f; P.Bullet.Pierce = 99; P.Bullet.Gravity = -216.f; break;
	case EBEItemKind::Laser:
		P.WeaponId = TEXT("laser"); P.Cooldown = 0.033f; P.Shake = 0.04f;
		P.Bullet.Flipbook = FB(TEXT("laserbeam")); P.Bullet.Speed = 1440.f; P.Bullet.LifeTime = 0.66f; P.Bullet.TileDamage = 1.5f; P.Bullet.Pierce = 99; break;
	default: break;
	}
	PickupProfile = P;
}

void ABEBro::GivePickup(EBEItemKind Kind, int32 Ammo)
{
	Pickup.bActive = true; Pickup.Kind = Kind; Pickup.Ammo = Ammo;
	BuildPickupProfile();
	SetWeaponId(PickupProfile.WeaponId);
}

void ABEBro::Die()
{
	bAlive = false;
	DeadTimer = UBEGameSettings::Get()->RespawnDelay;
	Pickup.bActive = false;
	if (Data) SetWeaponId(Data->Weapon.WeaponId);
	MeleeT = 0.f;
	if (bCrouch && Data) { Box.Pos.Y -= (Data->BodySize.Y - Data->CrouchSize.Y); Box.Size = Data->BodySize; bCrouch = false; }
	SetActorHiddenInGame(true);
}

void ABEBro::RespawnAt(FVector2D FeetPos)
{
	bAlive = true;
	SetActorHiddenInGame(false);
	SetFeet(FeetPos);
	Vel = FVector2D(0.f, -360.f);
	Invulnerable = UBEGameSettings::Get()->InvulnerableTime;
	Banner = 2.5f;
	if (Data) Grenades = FMath::Max(Grenades, Data->Grenades);
	UpdateVisuals();
}

void ABEBro::SwitchCharacter(UBECharacterData* NewData)
{
	if (!NewData) return;
	SetCharacter(NewData);
	Banner = 2.5f;
}

void ABEBro::SetCrouch(bool bOn)
{
	if (bOn == bCrouch || !Data) return;
	const float Diff = Data->BodySize.Y - Data->CrouchSize.Y;
	if (bOn) { Box.Pos.Y += Diff; Box.Size = Data->CrouchSize; bCrouch = true; return; }
	FBEBody Test = Box; Test.Pos.Y -= Diff; Test.Size = Data->BodySize;
	if (ABETileWorld* W = World()) if (!W->FitsAt(Test)) return;
	Box = Test; bCrouch = false;
}

bool ABEBro::EnemyInReach() const
{
	ABEGameState* S = GS(); if (!S) return false;
	for (ABEMook* M : S->Mooks)
	{
		if (!IsValid(M) || M->bDead) continue;
		const bool bInFront = Facing > 0
			? (M->Box.Pos.X + M->Box.Size.X > Box.Pos.X + Box.Size.X - 4.f && M->Box.Pos.X < Box.Pos.X + Box.Size.X + 24.f)
			: (M->Box.Pos.X < Box.Pos.X + 4.f && M->Box.Pos.X + M->Box.Size.X > Box.Pos.X - 24.f);
		if (bInFront && FMath::Abs(M->Box.Pos.Y - Box.Pos.Y) < 32.f) return true;
	}
	return false;
}

void ABEBro::MeleeHit()
{
	ABEGameState* S = GS(); ABETileWorld* W = World(); if (!S || !W) return;
	FBEBody Hit; Hit.Size = FVector2D(36.f, Box.Size.Y + 8.f);
	Hit.Pos = FVector2D(Facing > 0 ? Box.Pos.X + Box.Size.X - 4.f : Box.Pos.X - 32.f, Box.Pos.Y - 4.f);
	bool bAny = false;
	for (ABEMook* M : TArray<TObjectPtr<ABEMook>>(S->Mooks)) if (IsValid(M) && !M->bDead && Hit.Overlaps(M->Box)) { S->HurtMook(M, 3.f, Slot, EBEHitKind::Melee); bAny = true; }
	for (ABEBarrel* B : S->Barrels) if (IsValid(B) && !B->bDead && Hit.Overlaps(B->Box)) B->Ignite(0.02f);
	for (ABECage* C : TArray<TObjectPtr<ABECage>>(S->Cages)) if (IsValid(C) && !C->bDead && Hit.Overlaps(C->Box)) C->Hurt(3.f, Slot);
	// Graben
	const float TS = W->GetTileSize();
	const int32 TX = FMath::FloorToInt((Facing > 0 ? Box.Pos.X + Box.Size.X + 10.f : Box.Pos.X - 10.f) / TS);
	const int32 TY = FMath::FloorToInt((Box.Pos.Y + Box.Size.Y * 0.5f) / TS);
	if (W->IsSolidTile(W->GetTile(TX, TY))) { W->DamageTile(TX, TY, 2.5f); UBECombatLibrary::Sparks(this, W->TileCenter(TX, TY), 3); bAny = true; }
	if (bAny && S->Camera) S->Camera->Shake(0.15f);
}

void ABEBro::Fire()
{
	ABEGameState* S = GS(); if (!S) return;
	const FBEWeaponProfile& P = Weapon();
	FireCd = P.Cooldown; MuzzleT = 0.08f;
	const FVector2D Mz = MuzzlePos();
	const FVector2D Dir = Aim == EBEAim::Up ? FVector2D(0.f, -1.f) : Aim == EBEAim::Down ? FVector2D(0.f, 1.f) : FVector2D(static_cast<float>(Facing), 0.f);
	const float Base = FMath::Atan2(Dir.Y, Dir.X);
	for (int32 i = 0; i < FMath::Max(1, P.Count); ++i)
	{
		const float Fan = P.Count > 1 ? (static_cast<float>(i) / (P.Count - 1) - 0.5f) * 2.f * P.Spread : 0.f;
		const float Ang = Base + Fan + (FMath::FRand() - 0.5f) * P.Spread * 0.6f;
		const float Sp = P.Bullet.Speed * (P.Count > 1 ? 0.85f + FMath::FRand() * 0.3f : 1.f);
		FVector2D V(FMath::Cos(Ang) * Sp, FMath::Sin(Ang) * Sp);
		if (Aim == EBEAim::Horizontal) V.X += Vel.X * 0.3f;
		if (ABEProjectile* Pr = GetWorld()->SpawnActor<ABEProjectile>(ABEProjectile::StaticClass(), BE::ToWorld(Mz, BE::DepthProjectile), FRotator::ZeroRotator))
			Pr->Init(Mz, V, P.Bullet, true, Slot);
	}
	if (Aim == EBEAim::Horizontal) Vel.X -= Facing * P.Recoil * (bOnGround ? 0.5f : 1.f);
	else if (Aim == EBEAim::Down && !bOnGround) Vel.Y -= P.Recoil * 1.6f;   // Ketchup-Jump
	if (S->Camera) S->Camera->Shake(P.Shake);
	UBECombatLibrary::SpawnFX(this, UBEGameSettings::Get()->SparksFX, Mz);
	UBECombatLibrary::Flash(this, Mz, FLinearColor(1.f, 0.85f, 0.4f), 6.f, 140.f, 0.08f);
	UBECombatLibrary::PlaySound2D(this, P.Sound);
	if (Pickup.bActive && --Pickup.Ammo <= 0)
	{
		Pickup.bActive = false;
		S->AddPopup(TEXT("LEER!"), Center() - FVector2D(0.f, 30.f), FLinearColor(0.62f, 0.61f, 0.69f));
		if (Data) SetWeaponId(Data->Weapon.WeaponId);
	}
}

void ABEBro::ThrowGrenade()
{
	Grenades--; SpecialCd = 0.3f;
	const FVector2D V(Facing * 372.f + Vel.X * 0.5f, Aim == EBEAim::Up ? -720.f : -504.f);
	const FVector2D At = Center() - FVector2D(0.f, 8.f);
	if (ABEGrenade* G = GetWorld()->SpawnActor<ABEGrenade>(ABEGrenade::StaticClass(), BE::ToWorld(At, BE::DepthProjectile), FRotator::ZeroRotator))
		G->Init(At, V, Data ? Data->GrenadeRadius : 64.f, true, Slot);
}

void ABEBro::SimStep(float Dt)
{
	const UBEGameSettings* S = UBEGameSettings::Get();
	ABEGameState* G = GS(); ABETileWorld* W = World();
	auto ClearPressed = [this]() { Input.bJumpPressed = Input.bFirePressed = Input.bSpecialPressed = Input.bMeleePressed = Input.bStartPressed = false; };
	AnimClock += Dt;
	if (Banner > 0.f) Banner -= Dt;
	if (!G || !W) { ClearPressed(); return; }

	if (!bAlive)
	{
		DeadTimer -= Dt;
		if (DeadTimer <= 0.f) { DeadTimer = 0.5f; G->TryRespawn(this); }
		ClearPressed(); return;
	}
	if (Invulnerable > 0.f) Invulnerable -= Dt;

	// Ducken & Zielen
	SetCrouch(Input.bDown && bOnGround && !Input.bUp && MeleeT <= 0.f);
	Aim = Input.bUp ? EBEAim::Up : (Input.bDown && !bOnGround) ? EBEAim::Down : EBEAim::Horizontal;

	// Horizontal
	const float AX = (Input.bLeft ? -1.f : 0.f) + (Input.bRight ? 1.f : 0.f);
	const float MaxSpeed = bCrouch ? S->CrouchSpeed : S->RunSpeed;
	if (AX != 0.f) { Facing = AX > 0.f ? 1 : -1; Vel.X += AX * (bOnGround ? S->GroundAcceleration : S->AirAcceleration) * Dt; }
	else if (bOnGround) { Vel.X *= FMath::Pow(0.68f, Dt * 60.f); if (FMath::Abs(Vel.X) < 6.f) Vel.X = 0.f; }
	else Vel.X *= FMath::Pow(0.97f, Dt * 60.f);
	if (FMath::Abs(Vel.X) > MaxSpeed) Vel.X = FMath::Sign(Vel.X) * FMath::Max(MaxSpeed, FMath::Abs(Vel.X) * FMath::Pow(0.9f, Dt * 60.f));

	// Springen: Coyote-Zeit, Eingabepuffer, variable Höhe, Fallen durch Plattformen
	Coyote = bOnGround ? S->CoyoteTime : Coyote - Dt;
	JumpBuffer = Input.bJumpPressed ? S->JumpBufferTime : JumpBuffer - Dt;
	if (JumpBuffer > 0.f && Coyote > 0.f)
	{
		if (Input.bDown && W->OnPlatformOnly(Box)) { Box.DropThrough = 0.17f; SetCrouch(false); Box.Pos.Y += 1.f; }
		else { SetCrouch(false); Vel.Y = -S->JumpVelocity; UBECombatLibrary::Dust(this, Feet(), 4); }
		Coyote = 0.f; JumpBuffer = 0.f; bOnGround = false;
	}
	if (!Input.bJump && Vel.Y < -S->JumpCutVelocity) Vel.Y = -S->JumpCutVelocity;
	if (Box.DropThrough > 0.f) Box.DropThrough -= Dt;
	Vel.Y = FMath::Min(S->MaxFallSpeed, Vel.Y + S->Gravity * Dt);

	// Bewegung
	const bool bWasGround = bOnGround;
	if (W->MoveX(Box, Vel.X * Dt)) Vel.X = 0.f;
	if (W->MoveY(Box, Vel.Y * Dt)) { bOnGround = Vel.Y > 0.f; if (Vel.Y > 300.f && bOnGround) UBECombatLibrary::Dust(this, Feet(), 3); Vel.Y = 0.f; }
	else bOnGround = false;
	(void)bWasGround;

	// Im Kamerabild bleiben (Koop: niemand rennt davon)
	if (G->Camera)
	{
		const float MinX = G->Camera->Left() + 2.f, MaxX = G->Camera->Right() - Box.Size.X - 2.f;
		if (Box.Pos.X < MinX) { Box.Pos.X = MinX; if (Vel.X < 0.f) Vel.X = 0.f; }
		if (Box.Pos.X > MaxX) { Box.Pos.X = MaxX; if (Vel.X > 0.f) Vel.X = 0.f; }
	}
	if (Box.Pos.Y > W->PixelHeight() + 60.f) { Invulnerable = 0.f; G->KillBro(this, EBEHitKind::Fall); ClearPressed(); return; }

	// Nahkampf: automatisch bei Gegnerkontakt (Metal Slug) oder per Taste
	if (MeleeT > 0.f)
	{
		const float Prev = MeleeT; MeleeT -= Dt;
		if (Prev > 0.12f && MeleeT <= 0.12f) MeleeHit();
	}
	else if (Input.bMeleePressed || EnemyInReach()) { MeleeT = 0.2f; }

	// Feuer
	if (FireCd > 0.f) FireCd -= Dt;
	if (MuzzleT > 0.f) MuzzleT -= Dt;
	if (Input.bFire && FireCd <= 0.f && MeleeT <= 0.f) Fire();

	// Granate
	if (SpecialCd > 0.f) SpecialCd -= Dt;
	if (Input.bSpecialPressed && SpecialCd <= 0.f)
	{
		if (Grenades > 0) ThrowGrenade();
		else { G->AddPopup(TEXT("KEINE GRANATEN"), Center() - FVector2D(0.f, 30.f), FLinearColor(0.62f, 0.61f, 0.69f)); SpecialCd = 0.5f; }
	}
	ClearPressed();
}

FName ABEBro::AnimName() const
{
	if (MeleeT > 0.f) return TEXT("melee");
	if (bCrouch) return TEXT("crouch");
	return Super::AnimName();
}

void ABEBro::UpdateVisuals()
{
	if (bCrouch && Aim == EBEAim::Down) Aim = EBEAim::Horizontal;
	Super::UpdateVisuals();
	const bool bBlinkOff = Invulnerable > 0.f && FMath::Fmod(AnimClock * 15.f, 2.f) >= 1.f;
	Body->SetVisibility(bAlive && !bBlinkOff);
	if (bBlinkOff || !bAlive) WeaponSprite->SetVisibility(false);
}
