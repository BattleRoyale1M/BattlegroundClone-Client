#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractionComponent.generated.h"

class ADropCharacter;
class AItemPickupActor;
class UAnimMontage;
class UPrimitiveComponent;
struct FHitResult;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNearbyPickupsChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemUseStarted, FText, ItemName, float, Duration);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemUseEnded, bool, bCompleted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractableChanged, FText, PromptText);

/*
ADropCharacter 전용 컴포넌트. 근처 상호작용 대상 추적, 아이템 줍기, 가방<->무기 슬롯 교체,
소모품(힐/부스트) 사용을 담당. bIsDead/DropState 등 캐릭터 상태를 직접 참조하므로
다른 폰에서는 재사용을 가정하지 않음.
*/
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BATTLEGROUNDCLONE_API UInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInteractionComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// --- 근처 상호작용 대상 ---
	UFUNCTION(BlueprintPure, Category = "Interaction")
	AActor* GetCurrentInteractable() const { return CurrentInteractable; }

	void OnInteractPressed();

	// --- 줍기 ---
	UFUNCTION(BlueprintPure, Category = "Inventory")
	TArray<AItemPickupActor*> GetNearbyPickups() const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void RequestPickup(AItemPickupActor* Pickup);
	bool TryPickup(AItemPickupActor* Pickup);   // 서버 전용

	// --- 가방 <-> 무기 슬롯 ---
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void RequestEquipFromBag(int32 BagIndex, int32 TargetSlot);
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void RequestUnequipToBag(int32 SlotIndex);

	// --- 소모품 사용 ---
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void RequestUseItem(FName RowName);
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsUsingItem() const { return !UsingItemRow.IsNone(); }
	void RequestCancelUseItem();
	void CancelUseItem();

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnNearbyPickupsChanged OnNearbyPickupsChanged;

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnItemUseStarted OnItemUseStarted;

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnItemUseEnded OnItemUseEnded;

	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnInteractableChanged OnInteractableChanged;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnInteractableBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	void OnInteractableEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	void UpdateCurrentInteractable();

	UFUNCTION(Server, Reliable)
	void ServerInteract(AActor* InteractActor);
	UFUNCTION(Server, Reliable)
	void ServerPickup(AItemPickupActor* Pickup);
	UFUNCTION(Server, Reliable)
	void ServerEquipFromBag(int32 BagIndex, int32 TargetSlot);
	UFUNCTION(Server, Reliable)
	void ServerUnequipToBag(int32 SlotIndex);
	UFUNCTION(Server, Reliable)
	void ServerUseItem(FName RowName);
	UFUNCTION(Server, Reliable)
	void ServerCancelUseItem();

	void FinishUseItem();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayUseMontage(UAnimMontage* Montage, float Duration);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastStopUseMontage(UAnimMontage* Montage);

	UFUNCTION(Client, Reliable)
	void ClientItemUseStarted(const FText& ItemName, float Duration);
	UFUNCTION(Client, Reliable)
	void ClientItemUseEnded(bool bCompleted);

	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	float MaxPickupDistance = 300.f;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> NearbyInteractables;

	UPROPERTY()
	TObjectPtr<AActor> CurrentInteractable;

	UPROPERTY(Replicated)
	FName UsingItemRow;

	FTimerHandle UseItemTimer;

private:
	ADropCharacter* GetOwnerDropCharacter() const;
};
