#include "WeaponUserInterface.h"


void ADropCharacter::ReceiveWeaponRecoil_Implementation(float Pitch, float YawRange, float RecoverySpeed)
{
	AddRecoil(Pitch, YawRange, RecoverySpeed);
}
void ADropCharacter::NotifyWeaponFired_Implementation()
{
	if (WeaponInventory) WeaponInventory->PlayFireMontage();
}
void ADropCharacter::NotifyWeaponReloadStarted_Implementation(float Duration)
{
	if (WeaponInventory) WeaponInventory->HandleReloadStarted(Duration);
}
void ADropCharacter::RequestMeleeAttack_Implementation(UAnimMontage* AttackMontage)
{
	MeleeAttack(AttackMontage);
}
void ADropCharacter::NotifyAmmoEmpty_Implementation()
{
	if (ADropPlayerController* PC = Cast<ADropPlayerController>(GetController()))
		PC->ShowCenterNotification(FText::GetEmpty(), FText::FromString(TEXT("탄약 없음")), FLinearColor(1.f, 0.3f, 0.1f));
}