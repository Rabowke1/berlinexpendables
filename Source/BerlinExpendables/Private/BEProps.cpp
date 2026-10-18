#include "BEProps.h"
#include "BEGameState.h"
#include "BETileWorld.h"
#include "BEBro.h"
#include "BESettings.h"
#include "BEData.h"
#include "BECombatLibrary.h"
#include "PaperSpriteComponent.h"
#include "PaperFlipbookComponent.h"
#include "PaperFlipbook.h"

static UBESpriteLibrary* SpriteLib() { return UBEGameSettings::Get()->SpriteLibrary.LoadSynchronous(); }

template <typename T>
static T* MakeVisual(AActor* Owner, const TCHAR* Name)
{
	T* C = Owner->CreateDefaultSubobject<T>(Name);
	C->SetupAttachment(Owner->GetRootComponent());
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetCastShadow(false);
	return C;
}

// --- Gasflasche ----------------------------------------------------------------------
ABEBarrel::ABEBarrel()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Sprite = MakeVisual<UPaperSpriteComponent>(this, TEXT("Sprite"));
}
void ABEBarrel::Init(FVector2D TopLeft)
{
	Box.Pos = TopLeft; Box.Size = FVector2D(16.f, 26.f);
	if (UBESpriteLibrary* L = SpriteLib()) Sprite->SetSprite(L->Sprite(TEXT("barrel")));
	SetActorLocation(BE::ToWorld(Box.Center() + FVector2D(0.f, -3.f), BE::DepthGameplay + 0.5f));
}
void ABEBarrel::Ignite(float Delay) { if (Fuse < 0.f) Fuse = Delay; }
void ABEBarrel::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (Fuse < 0.f || bDead) return;
	Fuse -= DeltaSeconds;
	Sprite->SetSpriteColor(FMath::Fmod(Fuse * 30.f, 2.f) < 1.f ? FLinearColor(3.f, 2.f, 2.f) : FLinearColor::White);
	if (Fuse <= 0.f)
	{
		bDead = true;
		if (ABEGameState* S = UBECombatLibrary::GS(this)) S->AddScore(50, Box.Pos, true);
		UBECombatLibrary::Explode(this, Box.Center(), 80.f, false, -1, true);
		Destroy();
	}
}

// --- Käfig -----------------------------------------------------------------------------
ABECage::ABECage()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Captive = MakeVisual<UPaperFlipbookComponent>(this, TEXT("Captive"));
	CageSprite = MakeVisual<UPaperSpriteComponent>(this, TEXT("Cage"));
}
void ABECage::Init(FVector2D TopLeft)
{
	Box.Pos = TopLeft; Box.Size = FVector2D(32.f, 48.f);
	if (UBESpriteLibrary* L = SpriteLib()) CageSprite->SetSprite(L->Sprite(TEXT("cage")));
	if (UBECharacterData* D = UBEGameSettings::Get()->CaptiveData.LoadSynchronous())
	{
		Captive->SetFlipbook(D->GetFlipbook(TEXT("idle"), EBEAim::Horizontal));
		Captive->SetRelativeLocation(FVector(0.f, 1.f, -Box.Size.Y * 0.5f));   // Füße auf Käfigboden
	}
	SetActorLocation(BE::ToWorld(Box.Center(), BE::DepthGameplay + 0.5f));
}
void ABECage::Hurt(float Damage, int32 RescuerSlot)
{
	if (bDead) return;
	Hp -= Damage;
	UBECombatLibrary::Sparks(this, Box.Center(), 3);
	if (Hp <= 0.f) Open(RescuerSlot);
}
void ABECage::Open(int32 RescuerSlot)
{
	if (bDead) return;
	bDead = true;
	if (ABEGameState* S = UBECombatLibrary::GS(this)) S->Rescue(this, RescuerSlot);
	Destroy();
}

// --- Befreiter Gefangener ---------------------------------------------------------------
ABECaptive::ABECaptive()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Body = MakeVisual<UPaperFlipbookComponent>(this, TEXT("Body"));
}
void ABECaptive::Init(FVector2D TopLeft, int32 InFacing)
{
	Box.Pos = TopLeft; Box.Size = FVector2D(20.f, 44.f); Facing = InFacing;
	if (UBECharacterData* D = UBEGameSettings::Get()->CaptiveData.LoadSynchronous()) Body->SetFlipbook(D->GetFlipbook(TEXT("run"), EBEAim::Horizontal));
}
void ABECaptive::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float Dt = FMath::Min(DeltaSeconds, 0.05f);
	ABEGameState* S = UBECombatLibrary::GS(this);
	ABETileWorld* W = S ? S->TileWorld.Get() : nullptr;
	if (!W) { Destroy(); return; }
	VelY = FMath::Min(720.f, VelY + 2160.f * Dt);
	if (W->MoveX(Box, Facing * 144.f * Dt)) Facing *= -1;
	if (W->MoveY(Box, VelY * Dt)) VelY = 0.f;
	Life -= Dt;
	if (Life <= 0.f || Box.Pos.Y > W->PixelHeight()) { UBECombatLibrary::Dust(this, Box.Center(), 5); Destroy(); return; }
	SetActorLocation(BE::ToWorld(Box.Feet(), BE::DepthGameplay));
	Body->SetRelativeScale3D(FVector(static_cast<float>(Facing), 1.f, 1.f));
	Body->SetVisibility(!(Life < 0.35f && FMath::Fmod(Life * 20.f, 2.f) < 1.f));
}

// --- Item ----------------------------------------------------------------------------------
ABEItem::ABEItem()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Sprite = MakeVisual<UPaperSpriteComponent>(this, TEXT("Sprite"));
}
FName ABEItem::SpriteName(EBEItemKind K)
{
	switch (K)
	{
	case EBEItemKind::Heavy: return TEXT("item_H");
	case EBEItemKind::Rocket: return TEXT("item_R");
	case EBEItemKind::Flame: return TEXT("item_F");
	case EBEItemKind::Laser: return TEXT("item_L");
	case EBEItemKind::Bombs: return TEXT("item_B");
	default: return TEXT("item_life");
	}
}
FString ABEItem::DisplayName(EBEItemKind K)
{
	switch (K)
	{
	case EBEItemKind::Heavy: return TEXT("HEAVY MACHINE GUN");
	case EBEItemKind::Rocket: return TEXT("ROCKET LAUNCHER");
	case EBEItemKind::Flame: return TEXT("FLAME SHOT");
	case EBEItemKind::Laser: return TEXT("LASER SHOT");
	case EBEItemKind::Bombs: return TEXT("+5 GRANATEN");
	default: return TEXT("+1 LEBEN");
	}
}
void ABEItem::Init(FVector2D TopLeft, EBEItemKind InKind)
{
	Kind = InKind; Box.Pos = TopLeft; Box.Size = FVector2D(20.f, 20.f); Vel = FVector2D(0.f, -240.f);
	if (UBESpriteLibrary* L = SpriteLib()) Sprite->SetSprite(L->Sprite(SpriteName(Kind)));
	SetActorLocation(BE::ToWorld(Box.Center(), BE::DepthGameplay - 1.f));
}
void ABEItem::Apply(ABEBro* Bro)
{
	ABEGameState* S = UBECombatLibrary::GS(this); if (!S) return;
	FLinearColor C(1.f, 0.81f, 0.36f);
	switch (Kind)
	{
	case EBEItemKind::Bombs: Bro->Grenades = FMath::Min(9, Bro->Grenades + 5); C = FLinearColor(0.5f, 0.82f, 0.31f); break;
	case EBEItemKind::Life: S->Lives++; C = FLinearColor(1.f, 0.25f, 0.64f); break;
	case EBEItemKind::Heavy: Bro->GivePickup(Kind, 200); break;
	case EBEItemKind::Rocket: Bro->GivePickup(Kind, 15); break;
	case EBEItemKind::Flame: Bro->GivePickup(Kind, 150); break;
	case EBEItemKind::Laser: Bro->GivePickup(Kind, 200); break;
	}
	S->AddPopup(DisplayName(Kind), Box.Center() - FVector2D(0.f, 24.f), C);
	S->AddScore(100, Box.Pos, false);
}
void ABEItem::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float Dt = FMath::Min(DeltaSeconds, 0.05f);
	ABEGameState* S = UBECombatLibrary::GS(this);
	ABETileWorld* W = S ? S->TileWorld.Get() : nullptr;
	if (!S || !W || bDead) return;
	Vel.Y = FMath::Min(600.f, Vel.Y + 900.f * Dt);
	if (W->MoveX(Box, Vel.X * Dt)) Vel.X = 0.f;
	if (W->MoveY(Box, Vel.Y * Dt)) { Vel.Y = 0.f; Vel.X *= 0.8f; }
	Life -= Dt; Bob += Dt;
	if (Life <= 0.f || Box.Pos.Y > W->PixelHeight()) { bDead = true; Destroy(); return; }
	for (ABEBro* B : S->Bros)
	{
		if (!IsValid(B) || !B->bAlive || !B->Box.Overlaps(Box)) continue;
		bDead = true; Apply(B); Destroy(); return;
	}
	SetActorLocation(BE::ToWorld(Box.Center() + FVector2D(0.f, FMath::Sin(Bob * 6.f) * 2.f), BE::DepthGameplay - 1.f));
	Sprite->SetVisibility(!(Life < 3.f && FMath::Fmod(Life * 8.f, 2.f) < 1.f));
}

// --- Fluchttrabi -------------------------------------------------------------------------------
ABEExit::ABEExit()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Sprite = MakeVisual<UPaperSpriteComponent>(this, TEXT("Sprite"));
}
void ABEExit::Init(FVector2D TileTopLeft)
{
	const float TS = UBEGameSettings::Get()->TileSize;
	Box.Size = FVector2D(80.f, 40.f); Box.Pos = FVector2D(TileTopLeft.X, TileTopLeft.Y + TS - Box.Size.Y);
	if (UBESpriteLibrary* L = SpriteLib()) Sprite->SetSprite(L->Sprite(TEXT("trabi")));
	SetActorLocation(BE::ToWorld(Box.Center(), BE::DepthGameplay + 0.5f));
}
void ABEExit::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ABEGameState* S = UBECombatLibrary::GS(this); if (!S) return;
	if (Leaving >= 0.f)
	{
		Leaving += DeltaSeconds;
		Box.Pos.X += (60.f + Leaving * 90.f) * DeltaSeconds;
		PuffT -= DeltaSeconds;
		if (PuffT <= 0.f) { PuffT = 0.08f; UBECombatLibrary::Dust(this, Box.Pos + FVector2D(0.f, Box.Size.Y - 4.f), 2); }
		SetActorLocation(BE::ToWorld(Box.Center() + FVector2D(0.f, FMath::Fmod(Leaving * 20.f, 2.f) < 1.f ? 1.f : 0.f), BE::DepthGameplay + 0.5f));
		return;
	}
	int32 Alive = 0, Touching = 0; bool bAllNear = true;
	for (ABEBro* B : S->Bros)
	{
		if (!IsValid(B) || !B->bAlive) continue;
		++Alive;
		if (B->Box.Overlaps(Box)) ++Touching;
		if (FMath::Abs(B->Center().X - Box.Center().X) > 180.f) bAllNear = false;
	}
	if (Alive > 0 && Touching > 0 && bAllNear) { Leaving = 0.f; S->Win(); for (ABEBro* B : S->Bros) if (IsValid(B)) B->SetActorHiddenInGame(true); }
}
