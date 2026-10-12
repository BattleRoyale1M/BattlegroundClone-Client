#include "UI/DynamicCrosshairWidget.h"

#include "Character/DropCharacter.h"
#include "Weapon/WeaponBase.h"
#include "GameFramework/CharacterMovementComponent.h"

void UDynamicCrosshairWidget::NativeConstruct()
{
	Super::NativeConstruct();
	CurrentSpread = 0.f;
	FireSpread = 0.f;
	LastAmmo = -1;
	LastWeapon.Reset();
	ApplySpread(BaseGap);
}

void UDynamicCrosshairWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const ADropCharacter* Character = Cast<ADropCharacter>(GetOwningPlayerPawn());
	if (!Character)
	{
		return;
	}

	float TargetSpread = 0.f;
	const FVector Velocity = Character->GetVelocity();
	TargetSpread += MoveSpread * FMath::Clamp(Velocity.Size2D() / MoveSpeedForFullSpread, 0.f, 1.f);

	if (const UCharacterMovementComponent* Move = Character->GetCharacterMovement())
	{
		if (Move->IsFalling())
		{
			TargetSpread += AirSpread;
		}
	}

	AWeaponBase* Weapon = Character->GetEquippedWeapon();
	if (Weapon != LastWeapon.Get())
	{
		LastWeapon = Weapon;
		LastAmmo = Weapon ? Weapon->GetCurrentAmmo() : -1;
	}
	else if (Weapon)
	{
		const int32 Ammo = Weapon->GetCurrentAmmo();
		if (LastAmmo >= 0 && Ammo < LastAmmo)
		{
			FireSpread = FMath::Min(FireSpread + FireKick * (LastAmmo - Ammo), FireKickMax);
		}
		LastAmmo = Ammo;
	}

	const float InterpSpeed = TargetSpread > CurrentSpread ? ExpandSpeed : ContractSpeed;
	CurrentSpread = FMath::FInterpTo(CurrentSpread, TargetSpread, InDeltaTime, InterpSpeed);
	FireSpread = FMath::FInterpTo(FireSpread, 0.f, InDeltaTime, FireRecoverSpeed);

	const float StanceMultiplier = Character->IsProne() ? ProneMultiplier : 1.f;

	ApplySpread(FMath::Min(BaseGap + (CurrentSpread + FireSpread) * StanceMultiplier, MaxSpread));
}

void UDynamicCrosshairWidget::ApplySpread(float Spread)
{
	if (LineTop)
	{
		LineTop->SetRenderTranslation(FVector2D(0.f, -Spread));
	}
	if (LineBottom)
	{
		LineBottom->SetRenderTranslation(FVector2D(0.f, Spread));
	}
	if (LineLeft)
	{
		LineLeft->SetRenderTranslation(FVector2D(-Spread, 0.f));
	}
	if (LineRight)
	{
		LineRight->SetRenderTranslation(FVector2D(Spread, 0.f));
	}
}
