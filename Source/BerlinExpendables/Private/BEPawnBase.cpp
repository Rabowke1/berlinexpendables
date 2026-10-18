#include "BEPawnBase.h"
#include "BEData.h"
#include "BESettings.h"
#include "BEGameState.h"
#include "BETileWorld.h"
#include "PaperFlipbookComponent.h"
#include "PaperSpriteComponent.h"
#include "PaperFlipbook.h"
#include "PaperSprite.h"

ABEPawnBase::ABEPawnBase()
{
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessAI = EAutoPossessAI::Disabled;
	AutoPossessPlayer = EAutoReceiveInput::Disabled;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Body = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("Body"));
	Body->SetupAttachment(RootComponent);
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->SetCastShadow(false);
	WeaponSprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Weapon"));
	WeaponSprite->SetupAttachment(RootComponent);
	WeaponSprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponSprite->SetCastShadow(false);
}

void ABEPawnBase::BeginPlay()
{
	Super::BeginPlay();
	UpdateVisuals();
}

ABETileWorld* ABEPawnBase::World() const { ABEGameState* S = GS(); return S ? S->TileWorld.Get() : nullptr; }
ABEGameState* ABEPawnBase::GS() const { return GetWorld() ? GetWorld()->GetGameState<ABEGameState>() : nullptr; }

void ABEPawnBase::SetCharacter(UBECharacterData* InData)
{
	Data = InData;
	if (!Data) return;
	const FVector2D Feet = Box.Feet();
	Box.Size = bCrouch ? Data->CrouchSize : Data->BodySize;
	SetFeet(Feet);
	CurrentAnim = NAME_None;
	SetWeaponId(Data->Weapon.WeaponId);
	UpdateVisuals();
}

void ABEPawnBase::SetFeet(FVector2D FeetPos)
{
	Box.Pos = FVector2D(FeetPos.X - Box.Size.X * 0.5f, FeetPos.Y - Box.Size.Y);
}

void ABEPawnBase::SetWeaponId(FName InWeaponId)
{
	WeaponId = InWeaponId;
	ApplyWeaponSprite();
}

void ABEPawnBase::ApplyWeaponSprite()
{
	UBEWeaponLibrary* Lib = UBEGameSettings::Get()->WeaponLibrary.LoadSynchronous();
	UPaperSprite* Sp = Lib ? Lib->Find(WeaponId, Aim) : nullptr;
	if (WeaponSprite->GetSprite() != Sp) WeaponSprite->SetSprite(Sp);
}

FVector2D ABEPawnBase::MuzzlePos() const
{
	static const FName MuzzleName(TEXT("Muzzle"));
	if (WeaponSprite && WeaponSprite->GetSprite() && WeaponSprite->DoesSocketExist(MuzzleName))
		return BE::ToLevel(WeaponSprite->GetSocketLocation(MuzzleName));
	// Fallback ohne Waffensprite
	switch (Aim)
	{
	case EBEAim::Up: return Center() + FVector2D(Facing * 4.f, -Box.Size.Y * 0.8f);
	case EBEAim::Down: return Center() + FVector2D(Facing * 4.f, Box.Size.Y * 0.6f);
	default: return Center() + FVector2D(Facing * 24.f, -4.f);
	}
}

void ABEPawnBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Accum += FMath::Min(DeltaSeconds, 0.1f);
	int32 N = 0;
	while (Accum >= Step && N < 12) { SimStep(Step); Accum -= Step; ++N; }
	if (N == 12) Accum = 0.f;
	UpdateVisuals();
}

FName ABEPawnBase::AnimName() const
{
	if (!bOnGround) return Vel.Y < 0.f ? TEXT("jump") : TEXT("fall");
	if (FMath::Abs(Vel.X) > 12.f) return TEXT("run");
	return TEXT("idle");
}

void ABEPawnBase::SetAnim(FName Anim)
{
	CurrentAnim = Anim; CurrentAnimAim = Aim;
	UPaperFlipbook* FB = Data ? Data->GetFlipbook(Anim, Aim) : nullptr;
	if (FB && Body->GetFlipbook() != FB)
	{
		Body->SetFlipbook(FB);
		Body->SetLooping(Anim != TEXT("melee"));
		Body->PlayFromStart();
	}
	ApplyWeaponSprite();
}

void ABEPawnBase::UpdateVisuals()
{
	SetActorLocation(BE::ToWorld(Feet(), BE::DepthGameplay));
	Body->SetRelativeScale3D(FVector(static_cast<float>(Facing), 1.f, 1.f));
	const FName A = AnimName();
	if (A != CurrentAnim || Aim != CurrentAnimAim) SetAnim(A);

	static const FName HandName(TEXT("Hand"));
	const bool bShow = ShowWeapon() && WeaponSprite->GetSprite() != nullptr && Body->DoesSocketExist(HandName);
	WeaponSprite->SetVisibility(bShow && Body->IsVisible());
	if (bShow)
	{
		FVector L = Body->GetSocketLocation(HandName);
		L.Y = BE::DepthGameplay - 0.5f;
		WeaponSprite->SetWorldLocation(L);
		WeaponSprite->SetWorldScale3D(FVector(static_cast<float>(Facing), 1.f, 1.f));
	}
}
