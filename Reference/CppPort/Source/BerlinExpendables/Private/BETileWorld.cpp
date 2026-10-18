#include "BETileWorld.h"
#include "BEData.h"
#include "BESettings.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

ABETileWorld::ABETileWorld()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void ABETileWorld::BeginPlay()
{
	Super::BeginPlay();
	if (W == 0 && Level) BuildFromLevel(Level);
}

float ABETileWorld::MaxHp(EBETile T) const
{
	switch (T)
	{
	case EBETile::Dirt: return 3.f;
	case EBETile::Brick: return 6.f;
	case EBETile::Steel: return TNumericLimits<float>::Max();
	case EBETile::Concrete: return 9.f;
	case EBETile::Platform: return 4.f;
	case EBETile::Crate: return 2.f;
	default: return 0.f;
	}
}

void ABETileWorld::ParseRows(const TArray<FString>& Rows)
{
	H = Rows.Num();
	W = 0;
	for (const FString& R : Rows) W = FMath::Max(W, R.Len());
	Tiles.Init(0, W * H); Hp.Init(0.f, W * H); Back.Init(0, W * H); WasSolid.Init(0, W * H); Instance.Init(-1, W * H);
	Spawns.Reset();
	auto AddSpawn = [this](const TCHAR* Type, int32 X, int32 Y, const FString& Arg) { FBEEntitySpawn E; E.Type = Type; E.TX = X; E.TY = Y; E.Arg = Arg; Spawns.Add(E); };

	for (int32 Y = 0; Y < H; ++Y)
	{
		const FString& Row = Rows[Y];
		for (int32 X = 0; X < W; ++X)
		{
			const TCHAR C = X < Row.Len() ? Row[X] : TEXT('.');
			const int32 I = Index(X, Y);
			EBETile T = EBETile::Air; EBEBack B = EBEBack::None;
			switch (C)
			{
			case TEXT('#'): T = EBETile::Dirt; break;
			case TEXT('='): T = EBETile::Brick; break;
			case TEXT('S'): T = EBETile::Steel; break;
			case TEXT('C'): T = EBETile::Concrete; break;
			case TEXT('-'): T = EBETile::Platform; break;
			case TEXT('c'): T = EBETile::Crate; break;
			case TEXT('w'): B = EBEBack::Plaster; break;
			case TEXT('W'): B = EBEBack::Window; break;
			case TEXT('r'): B = EBEBack::Brick; break;
			case TEXT('g'): B = EBEBack::Graffiti; break;
			case TEXT('P'): SpawnTile = FIntPoint(X, Y); break;
			case TEXT('m'): AddSpawn(TEXT("mook"), X, Y, FString());  break;
			case TEXT('s'): AddSpawn(TEXT("security"), X, Y, FString());  break;
			case TEXT('b'): AddSpawn(TEXT("barrel"), X, Y, FString());  break;
			case TEXT('k'): AddSpawn(TEXT("cage"), X, Y, FString());  break;
			case TEXT('E'): AddSpawn(TEXT("exit"), X, Y, FString());  break;
			case TEXT('+'): AddSpawn(TEXT("life"), X, Y, FString());  break;
			case TEXT('H'): case TEXT('R'): case TEXT('F'): case TEXT('L'): case TEXT('B'):
				AddSpawn(TEXT("item"), X, Y, FString::Chr(C)); break;
			default: break;
			}
			Tiles[I] = static_cast<uint8>(T);
			Back[I] = static_cast<uint8>(B);
			Hp[I] = MaxHp(T);
			WasSolid[I] = (IsSolidTile(T) && T != EBETile::Crate) ? 1 : 0;
		}
	}
	// Hintergrund unter Entities in Innenräumen fortsetzen
	for (const FBEEntitySpawn& E : Spawns)
	{
		const int32 I = Index(E.TX, E.TY);
		const uint8 L = E.TX > 0 ? Back[I - 1] : 0, R = E.TX < W - 1 ? Back[I + 1] : 0;
		if (L && R) Back[I] = L;
	}
}

void ABETileWorld::BuildFromLevel(UBELevelAsset* InLevel)
{
	if (!InLevel) return;
	Level = InLevel;
	TileSize = UBEGameSettings::Get()->TileSize;
	ParseRows(Level->Rows);
	BuildVisuals();
}

UInstancedStaticMeshComponent* ABETileWorld::MakeLayer(FName Name)
{
	UInstancedStaticMeshComponent* ISM = NewObject<UInstancedStaticMeshComponent>(this, Name);
	ISM->SetupAttachment(RootComponent);
	ISM->SetMobility(EComponentMobility::Movable);
	ISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ISM->SetCastShadow(false);
	ISM->SetNumCustomDataFloats(5);   // u0 v0 u1 v1 damage
	if (QuadMesh) ISM->SetStaticMesh(QuadMesh);
	if (TileMaterial) ISM->SetMaterial(0, TileMaterial);
	ISM->RegisterComponent();
	return ISM;
}

FTransform ABETileWorld::TileTransform(int32 TX, int32 TY, float Depth) const
{
	// Engine-Plane (100x100, Normale +Z) -> Roll 90° legt es in die XZ-Ebene mit Normale -Y (zur Kamera).
	const float S = TileSize / 100.f;
	return FTransform(FRotator(0.f, 0.f, 90.f), BE::ToWorld(TileCenter(TX, TY), Depth), FVector(S, S, 1.f));
}

void ABETileWorld::ApplyRect(UInstancedStaticMeshComponent* ISM, int32 Idx, FName RectName, float Damage) const
{
	FBETileRect R;
	if (!TileSet || !TileSet->GetRect(RectName, R)) { R.UV0 = FVector2D::ZeroVector; R.UV1 = FVector2D(0.01f, 0.01f); }
	const UBEGameSettings* GS = UBEGameSettings::Get();
	FVector2D A = R.UV0, Bv = R.UV1;
	if (GS->bFlipTileU) Swap(A.X, Bv.X);
	if (GS->bFlipTileV) Swap(A.Y, Bv.Y);
	ISM->SetCustomDataValue(Idx, 0, A.X, false);
	ISM->SetCustomDataValue(Idx, 1, A.Y, false);
	ISM->SetCustomDataValue(Idx, 2, Bv.X, false);
	ISM->SetCustomDataValue(Idx, 3, Bv.Y, false);
	ISM->SetCustomDataValue(Idx, 4, Damage, true);
}

FName ABETileWorld::VisualName(int32 TX, int32 TY) const
{
	const EBETile T = GetTile(TX, TY);
	const EBETile Above = GetTile(TX, TY - 1);
	switch (T)
	{
	case EBETile::Dirt: return (Above == EBETile::Air || Above == EBETile::Platform) ? TEXT("dirt_top") : TEXT("dirt");
	case EBETile::Brick: return Above == EBETile::Air ? TEXT("brick_top") : TEXT("brick");
	case EBETile::Steel: return TEXT("steel");
	case EBETile::Concrete: return TEXT("concrete");
	case EBETile::Platform: return TEXT("platform");
	case EBETile::Crate: return TEXT("crate");
	default: return NAME_None;
	}
}

void ABETileWorld::BuildVisuals()
{
	for (auto& KV : Chunks) if (KV.Value) KV.Value->DestroyComponent();
	Chunks.Reset();
	if (BackLayer) BackLayer->DestroyComponent();
	if (EarthLayer) EarthLayer->DestroyComponent();
	BackLayer = MakeLayer(TEXT("BackLayer"));
	EarthLayer = MakeLayer(TEXT("EarthLayer"));

	static const TCHAR* BackNames[] = { TEXT(""), TEXT("back_plaster"), TEXT("back_window"), TEXT("back_brick"), TEXT("back_graffiti") };

	for (int32 Y = 0; Y < H; ++Y)
	{
		for (int32 X = 0; X < W; ++X)
		{
			const int32 I = Index(X, Y);
			if (Back[I])
			{
				const int32 Idx = BackLayer->AddInstance(TileTransform(X, Y, BE::DepthBackTiles));
				ApplyRect(BackLayer, Idx, FName(BackNames[Back[I]]), 0.f);
			}
			if (WasSolid[I])
			{
				const int32 Idx = EarthLayer->AddInstance(TileTransform(X, Y, BE::DepthEarth));
				ApplyRect(EarthLayer, Idx, TEXT("dirt"), 1.f);   // Material dunkelt bei Damage=1 stark ab
			}
			if (Tiles[I])
			{
				const FIntPoint CK(X / ChunkSize, Y / ChunkSize);
				TObjectPtr<UInstancedStaticMeshComponent>& Chunk = Chunks.FindOrAdd(CK);
				if (!Chunk) Chunk = MakeLayer(*FString::Printf(TEXT("Chunk_%d_%d"), CK.X, CK.Y));
				Instance[I] = Chunk->AddInstance(TileTransform(X, Y, 0.f));
				ApplyRect(Chunk, Instance[I], VisualName(X, Y), 0.f);
			}
		}
	}
}

EBETile ABETileWorld::GetTile(int32 TX, int32 TY) const
{
	if (TX < 0 || TX >= W) return EBETile::Steel;   // Levelränder sind Wände
	if (TY < 0 || TY >= H) return EBETile::Air;
	return static_cast<EBETile>(Tiles[Index(TX, TY)]);
}

void ABETileWorld::RefreshTile(int32 TX, int32 TY)
{
	if (TX < 0 || TY < 0 || TX >= W || TY >= H) return;
	const int32 I = Index(TX, TY);
	if (Instance[I] < 0) return;
	const TObjectPtr<UInstancedStaticMeshComponent>* Chunk = Chunks.Find(FIntPoint(TX / ChunkSize, TY / ChunkSize));
	if (!Chunk || !*Chunk) return;
	const EBETile T = static_cast<EBETile>(Tiles[I]);
	if (T == EBETile::Air)
	{
		// Unsichtbar: Skalierung 0 (Instanzindizes bleiben stabil)
		FTransform Tr = TileTransform(TX, TY, 0.f); Tr.SetScale3D(FVector::ZeroVector);
		(*Chunk)->UpdateInstanceTransform(Instance[I], Tr, false, true, true);
		return;
	}
	const float Max = MaxHp(T);
	const float Damage = FMath::IsFinite(Max) && Max > 0.f ? FMath::Clamp(1.f - Hp[I] / Max, 0.f, 1.f) * 0.6f : 0.f;
	ApplyRect(*Chunk, Instance[I], VisualName(TX, TY), Damage);
}

bool ABETileWorld::DamageTile(int32 TX, int32 TY, float Amount)
{
	if (TX < 0 || TY < 0 || TX >= W || TY >= H) return false;
	const int32 I = Index(TX, TY);
	const EBETile T = static_cast<EBETile>(Tiles[I]);
	if (T == EBETile::Air || T == EBETile::Steel) return false;
	Hp[I] -= Amount;
	if (Hp[I] > 0.f) { RefreshTile(TX, TY); return false; }
	Tiles[I] = 0; Hp[I] = 0.f;
	RefreshTile(TX, TY);
	RefreshTile(TX, TY + 1);   // Tile darunter bekommt ggf. eine Oberkante
	OnTileDestroyed.Broadcast(TX, TY, T, TileCenter(TX, TY));
	return true;
}

int32 ABETileWorld::DamageRadius(FVector2D Center, float Radius, float Power)
{
	int32 Destroyed = 0;
	const int32 TX0 = FMath::FloorToInt32((Center.X - Radius) / TileSize), TX1 = FMath::FloorToInt32((Center.X + Radius) / TileSize);
	const int32 TY0 = FMath::FloorToInt32((Center.Y - Radius) / TileSize), TY1 = FMath::FloorToInt32((Center.Y + Radius) / TileSize);
	for (int32 TY = TY0; TY <= TY1; ++TY)
	{
		for (int32 TX = TX0; TX <= TX1; ++TX)
		{
			const float D = FVector2D::Distance(TileCenter(TX, TY), Center);
			if (D > Radius) continue;
			if (DamageTile(TX, TY, Power * (1.f - D / Radius) + 2.f)) ++Destroyed;
		}
	}
	return Destroyed;
}

// --- AABB-Bewegung ------------------------------------------------------------------
bool ABETileWorld::MoveX(FBEBody& B, float DX) const
{
	if (FMath::IsNearlyZero(DX)) return false;
	const float NX = B.Pos.X + DX;
	const int32 Y0 = FMath::FloorToInt32(B.Pos.Y / TileSize), Y1 = FMath::FloorToInt32((B.Pos.Y + B.Size.Y - 0.01f) / TileSize);
	if (DX > 0.f)
	{
		const int32 TX = FMath::FloorToInt32((NX + B.Size.X - 0.01f) / TileSize);
		for (int32 TY = Y0; TY <= Y1; ++TY) if (IsSolidTile(GetTile(TX, TY))) { B.Pos.X = TX * TileSize - B.Size.X; return true; }
	}
	else
	{
		const int32 TX = FMath::FloorToInt32(NX / TileSize);
		for (int32 TY = Y0; TY <= Y1; ++TY) if (IsSolidTile(GetTile(TX, TY))) { B.Pos.X = (TX + 1) * TileSize; return true; }
	}
	B.Pos.X = NX;
	return false;
}

bool ABETileWorld::MoveY(FBEBody& B, float DY) const
{
	if (FMath::IsNearlyZero(DY)) return false;
	const float NY = B.Pos.Y + DY;
	const int32 X0 = FMath::FloorToInt32(B.Pos.X / TileSize), X1 = FMath::FloorToInt32((B.Pos.X + B.Size.X - 0.01f) / TileSize);
	if (DY > 0.f)
	{
		const int32 TY = FMath::FloorToInt32((NY + B.Size.Y - 0.01f) / TileSize);
		const bool bWasAbove = B.Pos.Y + B.Size.Y - 0.01f < TY * TileSize;
		for (int32 TX = X0; TX <= X1; ++TX)
		{
			const EBETile T = GetTile(TX, TY);
			if (IsSolidTile(T) || (T == EBETile::Platform && bWasAbove && B.DropThrough <= 0.f)) { B.Pos.Y = TY * TileSize - B.Size.Y; return true; }
		}
	}
	else
	{
		const int32 TY = FMath::FloorToInt32(NY / TileSize);
		for (int32 TX = X0; TX <= X1; ++TX) if (IsSolidTile(GetTile(TX, TY))) { B.Pos.Y = (TY + 1) * TileSize; return true; }
	}
	B.Pos.Y = NY;
	return false;
}

bool ABETileWorld::OnPlatformOnly(const FBEBody& B) const
{
	const int32 TY = FMath::FloorToInt32((B.Pos.Y + B.Size.Y + 0.5f) / TileSize);
	const int32 X0 = FMath::FloorToInt32(B.Pos.X / TileSize), X1 = FMath::FloorToInt32((B.Pos.X + B.Size.X - 0.01f) / TileSize);
	bool bPlat = false;
	for (int32 TX = X0; TX <= X1; ++TX)
	{
		const EBETile T = GetTile(TX, TY);
		if (IsSolidTile(T)) return false;
		if (T == EBETile::Platform) bPlat = true;
	}
	return bPlat;
}

bool ABETileWorld::FitsAt(const FBEBody& B) const
{
	const int32 X0 = FMath::FloorToInt32(B.Pos.X / TileSize), X1 = FMath::FloorToInt32((B.Pos.X + B.Size.X - 0.01f) / TileSize);
	const int32 Y0 = FMath::FloorToInt32(B.Pos.Y / TileSize), Y1 = FMath::FloorToInt32((B.Pos.Y + B.Size.Y - 0.01f) / TileSize);
	for (int32 TY = Y0; TY <= Y1; ++TY) for (int32 TX = X0; TX <= X1; ++TX) if (IsSolidTile(GetTile(TX, TY))) return false;
	return true;
}
