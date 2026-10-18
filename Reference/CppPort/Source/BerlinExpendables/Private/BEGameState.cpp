#include "BEGameState.h"
#include "BEBro.h"
#include "BEMook.h"
#include "BEProps.h"
#include "BETileWorld.h"
#include "BECameraActor.h"
#include "BEData.h"
#include "BESettings.h"
#include "BECombatLibrary.h"
#include "Kismet/GameplayStatics.h"

ABEGameState::ABEGameState()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ABEGameState::BeginPlay()
{
	Super::BeginPlay();
	Lives = UBEGameSettings::Get()->StartLives;
}

void ABEGameState::AddScore(int32 Amount, FVector2D At, bool bShow)
{
	Score += Amount;
	if (bShow) AddPopup(FString::Printf(TEXT("+%d"), Amount), At + FVector2D(10.f, -12.f), FLinearColor(1.f, 0.9f, 0.47f));
}

void ABEGameState::AddPopup(const FString& Text, FVector2D At, FLinearColor Color)
{
	FBEPopup P; P.Text = Text; P.Pos = At; P.Color = Color; P.Life = 0.9f;
	Popups.Add(P);
}

void ABEGameState::Freeze(float Seconds)
{
	UWorld* W = GetWorld(); if (!W) return;
	FreezeUntilRealTime = FMath::Max<double>(FreezeUntilRealTime, static_cast<double>(W->GetRealTimeSeconds()) + Seconds);
	if (!bFrozen) { bFrozen = true; UGameplayStatics::SetGlobalTimeDilation(this, 0.05f); }
}

void ABEGameState::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UWorld* W = GetWorld(); if (!W) return;
	if (bFrozen && static_cast<double>(W->GetRealTimeSeconds()) >= FreezeUntilRealTime) { bFrozen = false; UGameplayStatics::SetGlobalTimeDilation(this, 1.f); }

	MatchTime += DeltaSeconds;
	if (MatchState != EBEMatchState::Playing) StateTime += DeltaSeconds;

	for (int32 i = Popups.Num() - 1; i >= 0; --i)
	{
		Popups[i].Life -= DeltaSeconds;
		Popups[i].Pos.Y -= 24.f * DeltaSeconds;
		if (Popups[i].Life <= 0.f) Popups.RemoveAtSwap(i);
	}

	// Gegnerkontakt tötet (außer unverwundbar, Panik-Gegner sind harmlos)
	for (ABEBro* B : Bros)
	{
		if (!IsValid(B) || !B->bAlive || B->Invulnerable > 0.f) continue;
		for (ABEMook* M : Mooks)
		{
			if (!IsValid(M) || M->bDead || M->State == EBEMookState::Panic) continue;
			if (B->Box.Overlaps(M->Box)) { KillBro(B, EBEHitKind::Touch); break; }
		}
	}
	CleanupLists();
}

void ABEGameState::CleanupLists()
{
	Mooks.RemoveAll([](ABEMook* M) { return !IsValid(M) || M->bDead; });
	Barrels.RemoveAll([](ABEBarrel* B) { return !IsValid(B) || B->bDead; });
	Cages.RemoveAll([](ABECage* C) { return !IsValid(C) || C->bDead; });
	Items.RemoveAll([](ABEItem* I) { return !IsValid(I) || I->bDead; });
	Exits.RemoveAll([](ABEExit* E) { return !IsValid(E); });
	Bros.RemoveAll([](ABEBro* B) { return !IsValid(B); });
}

void ABEGameState::HandleTileDestroyed(int32 TX, int32 TY, EBETile Tile, FVector2D Center)
{
	FLinearColor Color;
	switch (Tile)
	{
	case EBETile::Dirt: Color = FLinearColor(0.42f, 0.29f, 0.18f); break;
	case EBETile::Brick: Color = FLinearColor(0.55f, 0.23f, 0.23f); break;
	case EBETile::Concrete: Color = FLinearColor(0.43f, 0.42f, 0.53f); break;
	case EBETile::Platform: Color = FLinearColor(0.29f, 0.28f, 0.4f); break;
	case EBETile::Crate: Color = FLinearColor(0.36f, 0.23f, 0.12f); break;
	default: Color = FLinearColor::Gray; break;
	}
	UBECombatLibrary::SpawnDebris(this, Center, 6, Color, 260.f, 5.f);
	UBECombatLibrary::SpawnFX(this, UBEGameSettings::Get()->DebrisFX, Center, Color);
	if (Tile == EBETile::Crate)
	{
		AddScore(10, Center, false);
		if (FMath::FRand() < 0.35f)
		{
			static const EBEItemKind Kinds[] = { EBEItemKind::Bombs, EBEItemKind::Heavy, EBEItemKind::Flame, EBEItemKind::Rocket, EBEItemKind::Laser, EBEItemKind::Life };
			if (ABEItem* It = GetWorld()->SpawnActor<ABEItem>(ABEItem::StaticClass(), BE::ToWorld(Center), FRotator::ZeroRotator))
			{
				It->Init(Center - FVector2D(10.f, 12.f), Kinds[FMath::RandRange(0, 5)]);
				Items.Add(It);
			}
		}
	}
}

void ABEGameState::KillBro(ABEBro* Bro, EBEHitKind Kind)
{
	if (!IsValid(Bro) || !Bro->bAlive || Bro->Invulnerable > 0.f) return;
	const FVector2D C = Bro->Center();
	Bro->Die();
	UBECombatLibrary::Blood(this, C, 24);
	if (Bro->Data)
	{
		UBECombatLibrary::SpawnDebris(this, C, Kind == EBEHitKind::Explosion ? 7 : 4, Bro->Data->Accent, 380.f, 7.f);
		UBECombatLibrary::SpawnDebris(this, C, 3, Bro->Data->Skin, 300.f, 6.f);
	}
	if (Camera) Camera->Shake(0.45f);
	Freeze(0.06f);
	if (Lives <= 0 && AliveBroCount() == 0) GameOver();
}

void ABEGameState::HurtMook(ABEMook* Mook, float Damage, int32 Slot, EBEHitKind Kind)
{
	if (!IsValid(Mook) || Mook->bDead) return;
	Mook->Hp -= Damage;
	UBECombatLibrary::Blood(this, Mook->Center(), 4);
	if (Mook->Hp > 0.f) { Mook->Stun = 0.17f; return; }

	const FVector2D C = Mook->Center();
	UBECombatLibrary::Blood(this, C, 16);
	if (Mook->Data)
	{
		UBECombatLibrary::SpawnDebris(this, C, Kind == EBEHitKind::Explosion ? 6 : 4, FLinearColor(0.18f, 0.17f, 0.29f), 360.f, 7.f);
		UBECombatLibrary::SpawnDebris(this, C, 2, Mook->Data->Skin, 300.f, 6.f);
		AddScore(Mook->Data->Score, Mook->Box.Pos);
	}
	if (Slot >= 0 && Slot < Bros.Num() && IsValid(Bros[Slot])) Bros[Slot]->Kills++;
	// Umstehende geraten in Panik
	for (ABEMook* O : Mooks)
	{
		if (!IsValid(O) || O == Mook || O->bDead || O->State == EBEMookState::Panic) continue;
		if (FMath::Abs(O->Box.Pos.X - Mook->Box.Pos.X) < 140.f && FMath::Abs(O->Box.Pos.Y - Mook->Box.Pos.Y) < 80.f && FMath::FRand() < 0.5f)
			O->Panic(1.5f + FMath::FRand(), Mook->Box.Pos.X);
	}
	Mook->Kill();
}

void ABEGameState::Rescue(ABECage* Cage, int32 RescuerSlot)
{
	if (!IsValid(Cage)) return;
	Lives++;
	const FVector2D C = Cage->Box.Center();
	AddScore(500, Cage->Box.Pos - FVector2D(0.f, 20.f));
	AddPopup(TEXT("+1 LEBEN"), C - FVector2D(0.f, 44.f), FLinearColor(1.f, 0.25f, 0.64f));
	UBECombatLibrary::SpawnDebris(this, C, 12, FLinearColor(0.53f, 0.65f, 0.78f), 300.f, 5.f);

	ABEBro* Rescuer = (RescuerSlot >= 0 && RescuerSlot < Bros.Num()) ? Bros[RescuerSlot].Get() : nullptr;
	const int32 Facing = IsValid(Rescuer) ? (Rescuer->Box.Pos.X < Cage->Box.Pos.X ? 1 : -1) : 1;
	if (ABECaptive* Cap = GetWorld()->SpawnActor<ABECaptive>(ABECaptive::StaticClass(), BE::ToWorld(C), FRotator::ZeroRotator))
		Cap->Init(Cage->Box.Pos + FVector2D(6.f, 4.f), Facing);

	static const EBEItemKind Kinds[] = { EBEItemKind::Heavy, EBEItemKind::Rocket, EBEItemKind::Flame, EBEItemKind::Laser, EBEItemKind::Bombs };
	if (ABEItem* It = GetWorld()->SpawnActor<ABEItem>(ABEItem::StaticClass(), BE::ToWorld(C), FRotator::ZeroRotator))
	{
		It->Init(Cage->Box.Pos + FVector2D(6.f, 8.f), Kinds[FMath::RandRange(0, 4)]);
		Items.Add(It);
	}
	// Broforce-Regel: der Retter wechselt den Bro
	if (IsValid(Rescuer) && Rescuer->bAlive)
	{
		if (UBECharacterData* Next = PickCharacter(Rescuer, false))
		{
			if (Next != Rescuer->Data) { Rescuer->SwitchCharacter(Next); UBECombatLibrary::Dust(this, Rescuer->Center(), 6); }
		}
	}
}

void ABEGameState::TryRespawn(ABEBro* Bro)
{
	if (!IsValid(Bro) || MatchState != EBEMatchState::Playing) return;
	if (Lives <= 0) { if (AliveBroCount() == 0) GameOver(); return; }
	Lives--;
	ABEBro* Anchor = AnyAliveBro();
	FVector2D Feet = Anchor ? Anchor->Feet() - FVector2D(0.f, 60.f) : (TileWorld ? TileWorld->SpawnPoint() + FVector2D(16.f, 8.f) : FVector2D(100.f, 100.f));
	if (UBECharacterData* Next = PickCharacter(Bro, true)) Bro->SwitchCharacter(Next);
	Bro->RespawnAt(Feet);
}

ABEBro* ABEGameState::NearestAliveBro(FVector2D From, float* OutDistance) const
{
	ABEBro* Best = nullptr; float BestD = TNumericLimits<float>::Max();
	for (ABEBro* B : Bros)
	{
		if (!IsValid(B) || !B->bAlive) continue;
		const float D = FVector2D::Distance(B->Center(), From);
		if (D < BestD) { BestD = D; Best = B; }
	}
	if (OutDistance) *OutDistance = BestD;
	return Best;
}

ABEBro* ABEGameState::AnyAliveBro() const
{
	for (ABEBro* B : Bros) if (IsValid(B) && B->bAlive) return B;
	return nullptr;
}

int32 ABEGameState::AliveBroCount() const
{
	int32 N = 0;
	for (ABEBro* B : Bros) if (IsValid(B) && B->bAlive) ++N;
	return N;
}

UBECharacterData* ABEGameState::PickCharacter(const ABEBro* For, bool bRandom) const
{
	const UBEGameSettings* S = UBEGameSettings::Get();
	TArray<UBECharacterData*> All;
	for (const TSoftObjectPtr<UBECharacterData>& P : S->PlayerCharacters) if (UBECharacterData* D = P.LoadSynchronous()) All.Add(D);
	if (All.Num() == 0) return nullptr;
	TSet<UBECharacterData*> Used;
	for (ABEBro* B : Bros) if (IsValid(B) && B != For && B->bAlive && B->Data) Used.Add(B->Data);
	int32 Start = 0;
	if (For && For->Data) Start = All.IndexOfByKey(For->Data.Get());
	if (Start < 0) Start = 0;
	int32 Idx = bRandom ? (Start + 1 + FMath::RandRange(0, FMath::Max(0, All.Num() - 2))) % All.Num() : (For ? (Start + 1) % All.Num() : Bros.Num() % All.Num());
	for (int32 i = 0; i < All.Num() && (Used.Contains(All[Idx]) || (For && For->Data == All[Idx] && All.Num() > 1)); ++i) Idx = (Idx + 1) % All.Num();
	return All[Idx];
}

void ABEGameState::Win() { if (MatchState == EBEMatchState::Playing) { MatchState = EBEMatchState::Won; StateTime = 0.f; } }
void ABEGameState::GameOver() { if (MatchState == EBEMatchState::Playing) { MatchState = EBEMatchState::GameOver; StateTime = 0.f; } }
