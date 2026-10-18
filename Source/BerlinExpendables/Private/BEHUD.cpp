#include "BEHUD.h"
#include "BEGameState.h"
#include "BEBro.h"
#include "BEMook.h"
#include "BEProps.h"
#include "BECameraActor.h"
#include "BETileWorld.h"
#include "BEData.h"
#include "BESettings.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"

void ABEHUD::Text(const FString& S, float X, float Y, FLinearColor Color, float Size, bool bCenter, bool bRight)
{
	UFont* F = GEngine->GetMediumFont();
	const float Sc = Size * Scale * 0.6f;
	float TW = 0.f, TH = 0.f;
	GetTextSize(S, TW, TH, F, Sc);
	const float XX = bCenter ? X - TW * 0.5f : bRight ? X - TW : X;
	DrawText(S, FLinearColor(0.04f, 0.04f, 0.07f), XX + Sc, Y + Sc, F, Sc);
	DrawText(S, Color, XX, Y, F, Sc);
}

FVector2D ABEHUD::ToScreen(FVector2D LevelPos) const
{
	ABEGameState* GS = GetWorld()->GetGameState<ABEGameState>();
	const FVector2D View = (GS && GS->Camera) ? GS->Camera->View : FVector2D::ZeroVector;
	return (LevelPos - View) * Scale;
}

void ABEHUD::DrawRectFill(float X, float Y, float W, float H, FLinearColor Color) { DrawRect(Color, X, Y, W, H); }

void ABEHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas) return;
	ABEGameState* GS = GetWorld()->GetGameState<ABEGameState>();
	if (!GS) return;
	const UBEGameSettings* Cfg = UBEGameSettings::Get();
	Scale = Canvas->SizeX / Cfg->ViewWidth;
	const float W = Canvas->SizeX, H = Canvas->SizeY;
	const float Time = GetWorld()->GetTimeSeconds();
	static const TCHAR* Labels[] = { TEXT("P1"), TEXT("P2"), TEXT("P3"), TEXT("P4") };

	// Spielerkarten
	for (int32 i = 0; i < GS->Bros.Num(); ++i)
	{
		ABEBro* B = GS->Bros[i]; if (!IsValid(B)) continue;
		const float X = (8.f + i * 236.f) * Scale, Y = 8.f * Scale;
		DrawRectFill(X, Y, 224.f * Scale, 44.f * Scale, FLinearColor(0.04f, 0.04f, 0.07f, 0.6f));
		DrawRectFill(X, Y, 4.f * Scale, 44.f * Scale, B->SlotColor());
		if (B->Data && B->Data->Portrait) DrawTexture(B->Data->Portrait, X + 8.f * Scale, Y + 6.f * Scale, 32.f * Scale, 32.f * Scale, 0.f, 0.f, 1.f, 1.f);
		Text(Labels[FMath::Clamp(B->Slot, 0, 3)], X + 46.f * Scale, Y + 4.f * Scale, B->SlotColor());
		FString Name = B->Data ? B->Data->DisplayName.ToString() : TEXT("?");
		int32 SpaceIdx; if (Name.FindLastChar(TEXT(' '), SpaceIdx)) Name = Name.Mid(SpaceIdx + 1);
		Text(Name, X + 78.f * Scale, Y + 4.f * Scale, FLinearColor(0.85f, 0.84f, 0.9f));
		if (!B->bAlive) Text(GS->Lives > 0 ? FString::Printf(TEXT("RESPAWN %d"), FMath::CeilToInt(B->DeadTimer)) : TEXT("TOT"), X + 46.f * Scale, Y + 24.f * Scale, FLinearColor(0.62f, 0.61f, 0.69f));
		else
		{
			Text(FString::Printf(TEXT("GRANATEN %d"), B->Grenades), X + 46.f * Scale, Y + 24.f * Scale, FLinearColor(0.85f, 0.84f, 0.9f));
			if (B->Pickup.bActive) Text(FString::Printf(TEXT("%s %d"), *ABEItem::DisplayName(B->Pickup.Kind).Left(1), B->Pickup.Ammo), X + 150.f * Scale, Y + 24.f * Scale, FLinearColor(1.f, 0.81f, 0.36f));
		}
	}

	Text(FString::Printf(TEXT("LEBEN %d"), GS->Lives), W * 0.5f, 60.f * Scale, GS->Lives <= 1 ? FLinearColor(0.89f, 0.23f, 0.23f) : FLinearColor::White, 1.3f, true);
	Text(FString::Printf(TEXT("%07d"), GS->Score), W - 8.f * Scale, 60.f * Scale, FLinearColor(1.f, 0.9f, 0.47f), 1.3f, false, true);

	if (GS->Bros.Num() < Cfg->MaxPlayers && GS->MatchState == EBEMatchState::Playing && FMath::Fmod(Time, 2.f) < 1.5f)
		Text(GS->Bros.Num() == 0 ? TEXT("SPRUNG ODER START DRUECKEN ZUM MITSPIELEN") : FString::Printf(TEXT("P%d: SPRUNG/START DRUECKEN ZUM BEITRETEN"), GS->Bros.Num() + 1), W * 0.5f, H - 24.f * Scale, FLinearColor(0.62f, 0.61f, 0.69f), 1.f, true);

	// Namens-Banner
	for (ABEBro* B : GS->Bros)
	{
		if (!IsValid(B) || B->Banner <= 0.f || !B->bAlive || !B->Data) continue;
		const float T = B->Banner;
		const float Slide = T > 2.2f ? (T - 2.2f) / 0.3f * W : T < 0.3f ? (0.3f - T) / 0.3f * W : 0.f;
		const float Y = (120.f + B->Slot * 4.f) * Scale;
		DrawRectFill(0.f, Y - 8.f * Scale, W, 60.f * Scale, FLinearColor(0.04f, 0.04f, 0.07f, 0.7f));
		DrawRectFill(0.f, Y - 8.f * Scale, W, 2.f * Scale, B->SlotColor());
		DrawRectFill(0.f, Y + 50.f * Scale, W, 2.f * Scale, B->SlotColor());
		Text(B->Data->DisplayName.ToString(), W * 0.5f + Slide, Y, B->SlotColor(), 2.6f, true);
		Text(B->Data->Tagline.ToString(), W * 0.5f - Slide, Y + 32.f * Scale, FLinearColor(0.85f, 0.84f, 0.9f), 1.1f, true);
	}

	// Titel, solange niemand spielt
	if (GS->Bros.Num() == 0)
	{
		DrawRectFill(0.f, 140.f * Scale, W, 180.f * Scale, FLinearColor(0.04f, 0.04f, 0.07f, 0.55f));
		Text(TEXT("BERLIN"), W * 0.5f, 150.f * Scale, FLinearColor(1.f, 0.81f, 0.36f), 5.f, true);
		Text(TEXT("EXPENDABLES"), W * 0.5f, 215.f * Scale, FLinearColor::White, 3.6f, true);
		Text(TEXT("P1 TASTATUR: WASD + J FEUER  K SPRUNG  L GRANATE  U NAHKAMPF"), W * 0.5f, 270.f * Scale, FLinearColor(0.62f, 0.61f, 0.69f), 1.f, true);
		Text(TEXT("P2 TASTATUR: PFEILE + , FEUER  . SPRUNG  / GRANATE  M NAHKAMPF"), W * 0.5f, 286.f * Scale, FLinearColor(0.62f, 0.61f, 0.69f), 1.f, true);
		Text(TEXT("GAMEPAD: X FEUER  A SPRUNG  Y GRANATE  B NAHKAMPF  START BEITRETEN"), W * 0.5f, 302.f * Scale, FLinearColor(0.62f, 0.61f, 0.69f), 1.f, true);
	}

	// Popups & Trabi-Hinweis
	for (const FBEPopup& P : GS->Popups)
	{
		if (P.Life < 0.2f && FMath::Fmod(P.Life * 20.f, 2.f) < 1.f) continue;
		const FVector2D S = ToScreen(P.Pos);
		Text(P.Text, S.X, S.Y, P.Color, 1.f, true);
	}
	for (ABEExit* E : GS->Exits)
	{
		if (!IsValid(E) || E->Leaving >= 0.f) continue;
		const FVector2D S = ToScreen(E->Box.Center() - FVector2D(0.f, 30.f + FMath::Sin(Time * 5.f) * 4.f));
		Text(TEXT("ZUM TRABI!"), S.X, S.Y, FLinearColor(1.f, 0.81f, 0.36f), 1.1f, true);
	}

	// Ende
	if (GS->MatchState != EBEMatchState::Playing)
	{
		DrawRectFill(0.f, 0.f, W, H, FLinearColor(0.04f, 0.04f, 0.07f, FMath::Min(0.75f, GS->StateTime / 1.f)));
		if (GS->StateTime > 0.5f)
		{
			const bool bWon = GS->MatchState == EBEMatchState::Won;
			Text(bWon ? TEXT("KIEZ BEFREIT!") : TEXT("GAME OVER"), W * 0.5f, 180.f * Scale, bWon ? FLinearColor(0.5f, 0.82f, 0.31f) : FLinearColor(0.89f, 0.23f, 0.23f), 4.f, true);
			Text(FString::Printf(TEXT("PUNKTE %d"), GS->Score), W * 0.5f, 250.f * Scale, FLinearColor(1.f, 0.9f, 0.47f), 2.4f, true);
			ABEBro* Best = nullptr;
			for (ABEBro* B : GS->Bros) if (IsValid(B) && (!Best || B->Kills > Best->Kills)) Best = B;
			if (Best && Best->Data) Text(FString::Printf(TEXT("MVP: %s %s (%d KILLS)"), Labels[FMath::Clamp(Best->Slot, 0, 3)], *Best->Data->DisplayName.ToString(), Best->Kills), W * 0.5f, 300.f * Scale, Best->SlotColor(), 1.3f, true);
			if (GS->StateTime > 1.5f && FMath::Fmod(Time, 1.f) < 0.7f) Text(TEXT("START/SPRUNG = NOCHMAL"), W * 0.5f, 360.f * Scale, FLinearColor(0.85f, 0.84f, 0.9f), 1.3f, true);
		}
	}

	// Debug
	if (GS->bDebug)
	{
		auto Outline = [this](const FBEBody& B, FLinearColor C) {
			const FVector2D S = ToScreen(B.Pos); const float BW = B.Size.X * Scale, BH = B.Size.Y * Scale;
			DrawRectFill(S.X, S.Y, BW, 1.f, C); DrawRectFill(S.X, S.Y + BH, BW, 1.f, C); DrawRectFill(S.X, S.Y, 1.f, BH, C); DrawRectFill(S.X + BW, S.Y, 1.f, BH, C);
		};
		for (ABEBro* B : GS->Bros) if (IsValid(B) && B->bAlive) { Outline(B->Box, B->SlotColor()); const FVector2D M = ToScreen(B->MuzzlePos()); DrawRectFill(M.X - 2.f, M.Y - 2.f, 4.f, 4.f, FLinearColor::White); }
		for (ABEMook* M : GS->Mooks) if (IsValid(M)) Outline(M->Box, FLinearColor(0.89f, 0.23f, 0.23f));
		for (ABEBarrel* B : GS->Barrels) if (IsValid(B)) Outline(B->Box, FLinearColor(1.f, 0.55f, 0.16f));
		for (ABECage* C : GS->Cages) if (IsValid(C)) Outline(C->Box, FLinearColor(0.53f, 0.65f, 0.78f));
		Text(FString::Printf(TEXT("T %.1f  BROS %d  MOOKS %d  CAM %.0f,%.0f  FPS %.0f"), GS->MatchTime, GS->Bros.Num(), GS->Mooks.Num(), GS->Camera ? GS->Camera->View.X : 0.f, GS->Camera ? GS->Camera->View.Y : 0.f, 1.f / FMath::Max(0.0001f, GetWorld()->GetDeltaSeconds())), 8.f * Scale, H - 40.f * Scale, FLinearColor(0.22f, 0.94f, 0.9f));
	}
}
