#include "BEData.h"
#include "PaperFlipbook.h"
#include "PaperSprite.h"

static const TCHAR* AimSuffix(EBEAim Aim)
{
	switch (Aim)
	{
	case EBEAim::Up: return TEXT("u");
	case EBEAim::Down: return TEXT("d");
	default: return TEXT("h");
	}
}

UPaperFlipbook* UBECharacterData::GetFlipbook(FName Anim, EBEAim Aim) const
{
	const FName Key(*FString::Printf(TEXT("%s_%s"), *Anim.ToString(), AimSuffix(Aim)));
	if (const TObjectPtr<UPaperFlipbook>* F = Flipbooks.Find(Key)) return F->Get();
	// Fallback: horizontale Variante, dann idle
	const FName KeyH(*FString::Printf(TEXT("%s_h"), *Anim.ToString()));
	if (const TObjectPtr<UPaperFlipbook>* F = Flipbooks.Find(KeyH)) return F->Get();
	if (const TObjectPtr<UPaperFlipbook>* F = Flipbooks.Find(TEXT("idle_h"))) return F->Get();
	return nullptr;
}

UPaperSprite* UBEWeaponLibrary::Find(FName WeaponId, EBEAim Aim) const
{
	const FName Key(*FString::Printf(TEXT("%s_%s"), *WeaponId.ToString(), AimSuffix(Aim)));
	if (const TObjectPtr<UPaperSprite>* S = Sprites.Find(Key)) return S->Get();
	const FName KeyH(*FString::Printf(TEXT("%s_h"), *WeaponId.ToString()));
	if (const TObjectPtr<UPaperSprite>* S = Sprites.Find(KeyH)) return S->Get();
	return nullptr;
}
