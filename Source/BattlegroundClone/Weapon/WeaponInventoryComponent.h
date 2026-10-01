#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/Enums/EFireMode.h"
#include "WeaponInventoryComponent.generated.h"

class AWeaponBase;
class UTexture2D;
class UAnimMontage;
class ACharacter;

USTRUCT(BlueprintType)
struct FWeaponSlotInfo
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	int32 SlotIndex = -1;
	
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<AWeaponBase> WeaponClass;
	
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	bool bHasWeapon = false;
	
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	bool bIsEquipped = false;
	
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	FText WeaponDisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UTexture2D> WeaponIcon;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	int32 CurrentAmmo = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	int32 ReserveAmmo = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BATTLEGROUNDCLONE_API UWeaponInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// 무기 슬롯 인덱스 가이드 (참고용)
	// 0: 주무기 1 (Primary 1)
	// 1: 주무기 2 (Primary 2)
	// 2: 보조무기 (Secondary / Handgun)
	// 3: 근접무기 (Melee)
	// 4: 투척무기 (Throwable)
	
	UWeaponInventoryComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	void InitialEquip();

	void SwitchWeaponSlot(int32 Index);
	void EquipWeaponClassAtSlot(int32 Index, TSubclassOf<AWeaponBase> NewWeaponClass);
	void EquipWeaponClassInSlots(const TArray<int32>& AllowedSlots, TSubclassOf<AWeaponBase> NewWeaponClass);
	bool TryEquipToEmptySlot(const TArray<int32>& AllowedSlots, TSubclassOf<AWeaponBase> NewWeaponClass);
	
	void ChangeFireMode();
	void StartFire();
	void StopFire();
	void OnReloadPressed();

	void PlayFireMontage();
	void HandleReloadStarted(float Duration);

	UFUNCTION(BlueprintPure, Category = "Weapon")
	AWeaponBase* GetEquippedWeapon() const
	{
		return EquippedWeapon;
	}

	UFUNCTION(BlueprintPure, Category = "Weapon")
	EFireMode GetCurrentFireMode() const
	{
		return CurrentFireMode;
	}

	FName GetWeaponAttachSocket() const
	{
		return WeaponAttachSocket;
	}

	FName GetCurrentAttachSocket() const;
	void RefreshWeaponAttach();

	UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
	FOnFireModeChanged OnFireModeChanged;
	
	UPROPERTY(BlueprintAssignable, Category="Weapon|Events")
	FOnInventoryChanged OnInventoryChanged;
	
	UFUNCTION(BlueprintPure, Category = "Weapon")
	TArray<FWeaponSlotInfo> GetWeaponSlotInfo() const;

	UFUNCTION(BlueprintPure, Category="Weapon")
	AWeaponBase* GetWeaponAtSlot(int32 Index) const
	{
		return WeaponSlots.IsValidIndex(Index) ? WeaponSlots[Index] : nullptr;
	}
	
	TSubclassOf<AWeaponBase> GetSlotWeaponClass(int32 Index) const
	{
		return WeaponSlotClasses.IsValidIndex(Index) ? WeaponSlotClasses[Index] : nullptr;
	}
	void ClearSlot(int32 Index);
	
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SwapWeaponSlots(int32 IndexA, int32 IndexB);
	
protected:
	void EquipWeaponSlot(int32 Index);

	UFUNCTION(Server, Reliable)
	void ServerSwtichWeaponSlot(int32 Index);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayEquipMontage();

	UFUNCTION()
	void OnRep_EquippedWeapon();
	
	UFUNCTION()
	void OnRep_WeaponSlotClasses();

	UFUNCTION()
	void OnRep_CurrentWeaponIndex();

	void FinishWeaponSwitch();

	ACharacter* GetOwnerCharacter() const;

	UPROPERTY(ReplicatedUsing = OnRep_WeaponSlotClasses, EditDefaultsOnly, Category = "Weapon")
	TArray<TSubclassOf<AWeaponBase>> WeaponSlotClasses;

	UPROPERTY(Replicated)
	TArray<TObjectPtr<AWeaponBase>> WeaponSlots;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentWeaponIndex)
	int32 CurrentWeaponIndex = -1;

	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	TObjectPtr<AWeaponBase> EquippedWeapon;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FName WeaponAttachSocket = TEXT("hand_r");

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FName ProneAttachSocket = TEXT("HandGrip_R_Prone");

	UPROPERTY(VisibleAnywhere, Category = "Weapon")
	EFireMode CurrentFireMode = EFireMode::Single;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TObjectPtr<UAnimMontage> EquipAnimMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TObjectPtr<UAnimMontage> FireAnimMontage;
	
	UFUNCTION(Server, Reliable)
	void ServerSwapWeaponSlots(int32 IndexA, int32 IndexB);

	UPROPERTY(EditDefaultsOnly, Category = "reload")
	TObjectPtr<UAnimMontage> ReloadAnimMontage;

	bool bIsSwitchingWeapon = false;
	FTimerHandle EquipTimerHandle;
};
