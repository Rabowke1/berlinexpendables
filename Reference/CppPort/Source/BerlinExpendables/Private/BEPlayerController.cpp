#include "BEPlayerController.h"
#include "BEGameState.h"
#include "BEGameMode.h"
#include "BEBro.h"
#include "BESettings.h"
#include "BECameraActor.h"
#include "Kismet/GameplayStatics.h"

ABEPlayerController::ABEPlayerController()
{
	bShowMouseCursor = false;
}

void ABEPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (ABEGameState* GS = GetWorld()->GetGameState<ABEGameState>()) if (GS->Camera) SetViewTargetWithBlend(GS->Camera, 0.f);
}

int32 ABEPlayerController::LocalIndex() const
{
	return UGameplayStatics::GetPlayerControllerID(const_cast<ABEPlayerController*>(this));
}

FBEInputFrame ABEPlayerController::ReadLayout(const FBEKeyLayout& L, bool bGamepadSticks) const
{
	FBEInputFrame F;
	F.bLeft = IsInputKeyDown(L.Left); F.bRight = IsInputKeyDown(L.Right); F.bUp = IsInputKeyDown(L.Up); F.bDown = IsInputKeyDown(L.Down);
	F.bJump = IsInputKeyDown(L.Jump); F.bFire = IsInputKeyDown(L.Fire); F.bSpecial = IsInputKeyDown(L.Special); F.bMelee = IsInputKeyDown(L.Melee); F.bStart = IsInputKeyDown(L.Start);
	if (bGamepadSticks)
	{
		const float Dz = UBEInputSettings::Get()->StickDeadzone;
		const float X = GetInputAnalogKeyState(EKeys::Gamepad_LeftX), Y = GetInputAnalogKeyState(EKeys::Gamepad_LeftY);
		F.bLeft |= X < -Dz; F.bRight |= X > Dz; F.bUp |= Y > Dz; F.bDown |= Y < -Dz;
		F.bFire |= GetInputAnalogKeyState(EKeys::Gamepad_RightTriggerAxis) > 0.5f;
		F.bSpecial |= IsInputKeyDown(EKeys::Gamepad_RightShoulder);
		F.bMelee |= IsInputKeyDown(EKeys::Gamepad_LeftShoulder) || GetInputAnalogKeyState(EKeys::Gamepad_LeftTriggerAxis) > 0.5f;
	}
	return F;
}

void ABEPlayerController::Dispatch(FBEInputFrame& Frame, FBEInputFrame& Prev, TObjectPtr<ABEBro>& Bro, EBEInputSource Source)
{
	ABEGameState* GS = GetWorld()->GetGameState<ABEGameState>();
	ABEGameMode* GM = GetWorld()->GetAuthGameMode<ABEGameMode>();
	if (!GS || !GM) return;
	Frame.ComputeEdges(Prev);
	Prev = Frame;
	if (GS->MatchState != EBEMatchState::Playing)
	{
		if ((Frame.bStartPressed || Frame.bJumpPressed) && GS->StateTime > 1.5f) GM->RestartMatch();
		return;
	}
	if (!IsValid(Bro))
	{
		if (Frame.bJumpPressed || Frame.bStartPressed) Bro = GM->RequestJoin(this, Source);
		return;
	}
	Bro->SetInput(Frame);
}

void ABEPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	if (!PlayerInput) return;
	const UBEInputSettings* IS = UBEInputSettings::Get();
	if (LocalIndex() == 0)
	{
		FBEInputFrame A = ReadLayout(IS->KeyboardOne, false);
		Dispatch(A, PrevPrimary, PrimaryBro, EBEInputSource::Controller);
		FBEInputFrame B = ReadLayout(IS->KeyboardTwo, false);
		Dispatch(B, PrevKeyboardTwo, KeyboardTwoBro, EBEInputSource::KeyboardTwo);
		if (WasInputKeyJustPressed(EKeys::F1)) if (ABEGameState* GS = GetWorld()->GetGameState<ABEGameState>()) GS->bDebug = !GS->bDebug;
	}
	else
	{
		FBEInputFrame A = ReadLayout(IS->Gamepad, true);
		Dispatch(A, PrevPrimary, PrimaryBro, EBEInputSource::Controller);
	}
}
