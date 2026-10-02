#pragma once
#include "WeaponUserInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UWeaponUserInterface : public UInterface
{
	GENERATED_BODY()
};
class BATTLEGROUNDCLONE_API IWeaponUserInterface
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintNativeEvent, Category = "Weapon")
	void ReceiveWeaponRecoil(float Pitch, float YawRange, float RecoverySpeed);
	UFUNCTION(BlueprintNativeEvent, Category = "Weapon")
	void NotifyWeaponFired();
	UFUNCTION(BlueprintNativeEvent, Category = "Weapon")
	void NotifyWeaponReloadStarted(float Duration);
	UFUNCTION(BlueprintNativeEvent, Category = "Weapon")
	void RequestMeleeAttack(UAnimMontage* AttackMontage);
	UFUNCTION(BlueprintNativeEvent, Category = "Weapon")
	void NotifyAmmoEmpty();
};
