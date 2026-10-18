#include "BEGameMode.h"
#include "BEGameState.h"
#include "BEPlayerController.h"
#include "BEHUD.h"
#include "BEBro.h"
#include "BEMook.h"
#include "BEProps.h"
#include "BETileWorld.h"
#include "BECameraActor.h"
#include "BEParallaxLayer.h"
#include "BEData.h"
#include "BESettings.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

ABEGameMode::ABEGameMode()
{
	GameStateClass = ABEGameState::StaticClass();
	PlayerControllerClass = ABEPlayerController::StaticClass();
	HUDClass = ABEHUD::StaticClass();
	DefaultPawnClass = nullptr;
}

void ABEGameMode::BeginPlay()
{
	Super::BeginPlay();
	UWorld* Wd = GetWorld();
	ABEGameState* GS = Wd->GetGameState<ABEGameState>();
	if (!GS) return;
	const UBEGameSettings* Cfg = UBEGameSettings::Get();
	UBELevelAsset* Level = LevelOverride ? LevelOverride.Get() : Cfg->StartLevel.LoadSynchronous();

	// Tile-Welt: platzierte verwenden, sonst erzeugen
	ABETileWorld* TW = nullptr;
	for (TActorIterator<ABETileWorld> It(Wd); It; ++It) { TW = *It; break; }
	if (!TW) TW = Wd->SpawnActor<ABETileWorld>(ABETileWorld::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
	if (!TW->TileSet) TW->TileSet = Cfg->TileSet.LoadSynchronous();
	if (!TW->QuadMesh) TW->QuadMesh = Cfg->TileQuadMesh.LoadSynchronous();
	if (!TW->QuadMesh) TW->QuadMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (!TW->TileMaterial) TW->TileMaterial = Cfg->TileMaterial.LoadSynchronous();
	if (TW->Width() == 0)
	{
		if (TW->Level) TW->BuildFromLevel(TW->Level);
		else if (Level) TW->BuildFromLevel(Level);
		else UE_LOG(LogTemp, Error, TEXT("BerlinExpendables: Kein Level-Asset. Project Settings > Berlin Expendables > StartLevel setzen oder ABETileWorld mit Level platzieren."));
	}
	GS->TileWorld = TW;
	TW->OnTileDestroyed.AddDynamic(GS, &ABEGameState::HandleTileDestroyed);

	// Kamera
	ABECameraActor* Cam = nullptr;
	for (TActorIterator<ABECameraActor> It(Wd); It; ++It) { Cam = *It; break; }
	if (!Cam) Cam = Wd->SpawnActor<ABECameraActor>(ABECameraActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
	GS->Camera = Cam;
	Cam->SnapTo(TW->SpawnPoint() + FVector2D(16.f, 16.f));

	// Parallax-Standardebenen, falls keine platziert sind
	bool bHasParallax = false;
	for (TActorIterator<ABEParallaxLayer> It(Wd); It; ++It) { bHasParallax = true; break; }
	if (!bHasParallax)
	{
		if (UBESpriteLibrary* Lib = Cfg->SpriteLibrary.LoadSynchronous())
		{
			struct FDef { const TCHAR* Name; float Factor; float Bottom; float Depth; };
			static const FDef Defs[] = { { TEXT("bg_sky"), 0.f, 540.f, 900.f }, { TEXT("bg_skyline"), 0.15f, 500.f, 700.f }, { TEXT("bg_mid"), 0.4f, 570.f, 500.f } };
			for (const FDef& D : Defs)
			{
				UPaperSprite* Sp = Lib->Sprite(FName(D.Name));
				if (!Sp) continue;
				ABEParallaxLayer* L = Wd->SpawnActor<ABEParallaxLayer>(ABEParallaxLayer::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
				L->Sprite = Sp; L->Factor = D.Factor; L->BottomY = D.Bottom; L->Depth = D.Depth;
				L->Rebuild();
			}
		}
	}

	SpawnEntities();

	// Weitere lokale Spieler (Gamepads) anlegen, Splitscreen aus, alle auf die Koop-Kamera
	for (int32 i = 1; i < Cfg->MaxPlayers; ++i)
		if (!UGameplayStatics::GetPlayerControllerFromID(this, i)) UGameplayStatics::CreatePlayer(this, i, true);
	if (GEngine && GEngine->GameViewport) GEngine->GameViewport->SetForceDisableSplitscreen(true);
	for (FConstPlayerControllerIterator It = Wd->GetPlayerControllerIterator(); It; ++It)
		if (APlayerController* PC = It->Get()) PC->SetViewTargetWithBlend(Cam, 0.f);

	// Wiederbeitritt nach Neustart
	if (UBEGameInstance* GI = Cast<UBEGameInstance>(GetGameInstance()))
	{
		if (GI->bRestarting)
		{
			const TArray<FBEJoinRecord> Joins = GI->PersistentJoins;
			GI->PersistentJoins.Reset(); GI->bRestarting = false;
			for (const FBEJoinRecord& R : Joins)
				if (ABEPlayerController* PC = Cast<ABEPlayerController>(UGameplayStatics::GetPlayerControllerFromID(this, R.ControllerIndex))) RequestJoin(PC, R.Source);
		}
	}
}

void ABEGameMode::SpawnEntities()
{
	UWorld* Wd = GetWorld();
	ABEGameState* GS = Wd->GetGameState<ABEGameState>();
	const UBEGameSettings* Cfg = UBEGameSettings::Get();
	if (!GS || !GS->TileWorld) return;
	const float TS = GS->TileWorld->GetTileSize();
	FActorSpawnParameters P; P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (const FBEEntitySpawn& E : GS->TileWorld->GetSpawns())
	{
		const FVector2D TL(E.TX * TS, E.TY * TS);
		const FVector2D Feet(TL.X + TS * 0.5f, TL.Y + TS);
		if (E.Type == TEXT("mook") || E.Type == TEXT("security"))
		{
			UBECharacterData* D = (E.Type == TEXT("mook") ? Cfg->MookData : Cfg->SecurityData).LoadSynchronous();
			if (!D) continue;
			if (ABEMook* M = Wd->SpawnActor<ABEMook>(ABEMook::StaticClass(), BE::ToWorld(Feet), FRotator::ZeroRotator, P)) { M->SetCharacter(D); M->SetFeet(Feet); GS->Mooks.Add(M); }
		}
		else if (E.Type == TEXT("barrel")) { if (ABEBarrel* B = Wd->SpawnActor<ABEBarrel>(ABEBarrel::StaticClass(), BE::ToWorld(Feet), FRotator::ZeroRotator, P)) { B->Init(TL + FVector2D(8.f, 6.f)); GS->Barrels.Add(B); } }
		else if (E.Type == TEXT("cage")) { if (ABECage* C = Wd->SpawnActor<ABECage>(ABECage::StaticClass(), BE::ToWorld(Feet), FRotator::ZeroRotator, P)) { C->Init(TL + FVector2D(0.f, TS - 48.f)); GS->Cages.Add(C); } }
		else if (E.Type == TEXT("exit")) { if (ABEExit* X = Wd->SpawnActor<ABEExit>(ABEExit::StaticClass(), BE::ToWorld(Feet), FRotator::ZeroRotator, P)) { X->Init(TL); GS->Exits.Add(X); } }
		else if (E.Type == TEXT("life") || E.Type == TEXT("item"))
		{
			EBEItemKind K = EBEItemKind::Life;
			if (E.Type == TEXT("item"))
			{
				const TCHAR C = E.Arg.Len() ? E.Arg[0] : TEXT('B');
				K = C == TEXT('H') ? EBEItemKind::Heavy : C == TEXT('R') ? EBEItemKind::Rocket : C == TEXT('F') ? EBEItemKind::Flame : C == TEXT('L') ? EBEItemKind::Laser : EBEItemKind::Bombs;
			}
			if (ABEItem* I = Wd->SpawnActor<ABEItem>(ABEItem::StaticClass(), BE::ToWorld(Feet), FRotator::ZeroRotator, P)) { I->Init(TL + FVector2D(6.f, 6.f), K); GS->Items.Add(I); }
		}
	}
}

ABEBro* ABEGameMode::SpawnBro(int32 ControllerIndex, EBEInputSource Source, UBECharacterData* Data)
{
	UWorld* Wd = GetWorld();
	ABEGameState* GS = Wd->GetGameState<ABEGameState>();
	if (!GS || !GS->TileWorld || !Data) return nullptr;
	const float TS = GS->TileWorld->GetTileSize();
	ABEBro* Anchor = GS->AnyAliveBro();
	const FVector2D Feet = Anchor ? Anchor->Feet() - FVector2D(0.f, 40.f) : GS->TileWorld->SpawnPoint() + FVector2D(TS * 0.5f, TS - 8.f);
	FActorSpawnParameters P; P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ABEBro* B = Wd->SpawnActor<ABEBro>(ABEBro::StaticClass(), BE::ToWorld(Feet), FRotator::ZeroRotator, P);
	if (!B) return nullptr;
	B->Slot = GS->Bros.Num(); B->ControllerIndex = ControllerIndex; B->Source = Source;
	B->SetCharacter(Data);
	B->SetFeet(Feet);
	B->Vel = FVector2D(0.f, -240.f);
	B->Banner = 2.5f;
	B->Invulnerable = UBEGameSettings::Get()->InvulnerableTime;
	GS->Bros.Add(B);
	return B;
}

ABEBro* ABEGameMode::RequestJoin(ABEPlayerController* PC, EBEInputSource Source)
{
	ABEGameState* GS = GetWorld()->GetGameState<ABEGameState>();
	if (!PC || !GS || GS->MatchState != EBEMatchState::Playing) return nullptr;
	if (GS->Bros.Num() >= UBEGameSettings::Get()->MaxPlayers) return nullptr;
	UBECharacterData* Data = GS->PickCharacter(nullptr, false);
	if (!Data) { UE_LOG(LogTemp, Error, TEXT("BerlinExpendables: Keine Charaktere in Project Settings > Berlin Expendables > PlayerCharacters.")); return nullptr; }
	ABEBro* B = SpawnBro(PC->LocalIndex(), Source, Data);
	if (!B) return nullptr;
	if (Source == EBEInputSource::KeyboardTwo) PC->KeyboardTwoBro = B; else PC->PrimaryBro = B;
	GS->AddPopup(Data->DisplayName.ToString() + TEXT(" IST DABEI!"), B->Center() - FVector2D(0.f, 40.f), B->SlotColor());
	if (UBEGameInstance* GI = Cast<UBEGameInstance>(GetGameInstance()))
	{
		const int32 Idx = PC->LocalIndex();
		if (!GI->PersistentJoins.ContainsByPredicate([&](const FBEJoinRecord& R) { return R.ControllerIndex == Idx && R.Source == Source; }))
		{
			FBEJoinRecord R; R.ControllerIndex = Idx; R.Source = Source; GI->PersistentJoins.Add(R);
		}
	}
	return B;
}

void ABEGameMode::RestartMatch()
{
	if (UBEGameInstance* GI = Cast<UBEGameInstance>(GetGameInstance())) GI->bRestarting = true;
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)));
}
