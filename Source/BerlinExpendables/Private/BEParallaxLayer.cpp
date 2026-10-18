#include "BEParallaxLayer.h"
#include "BEGameState.h"
#include "BECameraActor.h"
#include "BETypes.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"

ABEParallaxLayer::ABEParallaxLayer()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void ABEParallaxLayer::BeginPlay()
{
	Super::BeginPlay();
	Rebuild();
	if (ABEGameState* GS = GetWorld()->GetGameState<ABEGameState>()) if (GS->Camera) AddTickPrerequisiteActor(GS->Camera);
}

void ABEParallaxLayer::Rebuild()
{
	for (UPaperSpriteComponent* C : Tiles) if (C) C->DestroyComponent();
	Tiles.Reset();
	if (!Sprite) return;
	const FBoxSphereBounds B = Sprite->GetRenderBounds();
	SpriteWidth = FMath::Max(1.f, static_cast<float>(B.BoxExtent.X) * 2.f);
	SpriteHeight = static_cast<float>(B.BoxExtent.Z) * 2.f;
	for (int32 i = 0; i < FMath::Max(2, Copies); ++i)
	{
		UPaperSpriteComponent* C = NewObject<UPaperSpriteComponent>(this);
		C->SetupAttachment(RootComponent);
		C->SetSprite(Sprite);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCastShadow(false);
		C->RegisterComponent();
		Tiles.Add(C);
	}
}

void ABEParallaxLayer::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ABEGameState* GS = GetWorld()->GetGameState<ABEGameState>();
	if (!GS || !GS->Camera || Tiles.Num() == 0) return;
	const float CamX = GS->Camera->View.X, CamY = GS->Camera->View.Y;
	const float Offset = FMath::Fmod(CamX * Factor, SpriteWidth);
	const float StartX = CamX - Offset - SpriteWidth;
	const float Bottom = BottomY + CamY * (1.f - Factor);
	for (int32 i = 0; i < Tiles.Num(); ++i)
	{
		const float X = FMath::RoundToFloat(StartX + i * SpriteWidth + SpriteWidth * 0.5f);
		Tiles[i]->SetWorldLocation(BE::ToWorld(FVector2D(X, FMath::RoundToFloat(Bottom - SpriteHeight * 0.5f)), Depth));
	}
}
