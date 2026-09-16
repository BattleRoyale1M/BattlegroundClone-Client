#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/Enums/EFireMode.h"
#include "WeaponInventoryComponent.generated.h"

class AWeaponBase;
class UAnimMontage;
class ACharacter;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BATTLEGROUNDCLONE_API UWeaponInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWeaponInventoryComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	void InitialEquip();

	void SwitchWeaponSlot(int32 Index);
	void EquipWeaponClassAtSlot(int32 Index, TSubclassOf<AWeaponBase> NewWeaponClass);

	void ChangeFireMode();
	void StartFire();
	void StopFire();
	void OnReloadPressed();

	void PlayFireMontage();
	void HandleReloadStarted(float Duration);

	UFUNCTION(BlueprintPure, Category = "Weapon")
	AWeaponBase* GetEquippedWeapon() const { return EquippedWeapon; }

	UFUNCTION(BlueprintPure, Category = "Weapon")
	EFireMode GetCurrentFireMode() const { return CurrentFireMode; }

	FName GetWeaponAttachSocket() const { return WeaponAttachSocket; }

	UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
	FOnFireModeChanged OnFireModeChanged;

protected:
	void EquipWeaponSlot(int32 Index);

	UFUNCTION(Server, Reliable)
	void ServerSwtichWeaponSlot(int32 Index);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayEquipMontage();

	UFUNCTION()
	void OnRep_EquippedWeapon();

	void FinishWeaponSwitch();

	ACharacter* GetOwnerCharacter() const;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TArray<TSubclassOf<AWeaponBase>> WeaponSlotClasses;

	UPROPERTY()
	TArray<TObjectPtr<AWeaponBase>> WeaponSlots;

	int32 CurrentWeaponIndex = -1;

	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	TObjectPtr<AWeaponBase> EquippedWeapon;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FName WeaponAttachSocket = TEXT("hand_r");

	UPROPERTY(VisibleAnywhere, Category = "Weapon")
	EFireMode CurrentFireMode = EFireMode::Single;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TObjectPtr<UAnimMontage> EquipAnimMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TObjectPtr<UAnimMontage> FireAnimMontage;

	UPROPERTY(EditDefaultsOnly, Category = "reload")
	TObjectPtr<UAnimMontage> ReloadAnimMontage;

	bool bIsSwitchingWeapon = false;
	FTimerHandle EquipTimerHandle;
};
