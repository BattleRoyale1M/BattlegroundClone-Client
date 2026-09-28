#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable/InteractableInterface.h"
#include "LootContainer.generated.h"

class USphereComponent;
class AWeaponBase;
class UTexture2D;

USTRUCT(BlueprintType)
struct FLootItemEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Loot")
	TSubclassOf<AWeaponBase> WeaponClass;
	
	UPROPERTY(BlueprintReadOnly, Category = "Loot")
	TArray<int32> AllowedSlots;

	UPROPERTY(BlueprintReadOnly, Category = "Loot")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Loot")
	TObjectPtr<UTexture2D> Icon;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLootItemsChanged);

UCLASS()
class BATTLEGROUNDCLONE_API ALootContainer : public AActor, public IInteractableInterface
{
	GENERATED_BODY()

public:
	ALootContainer();

	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual FText GetInteractionPromptText_Implementation() const override;

	UFUNCTION(BlueprintCallable, Category = "Loot")
	void AddWeaponLoot(TSubclassOf<AWeaponBase> WeaponClass, const TArray<int32>& AllowedSlots);

	UFUNCTION(BlueprintPure, Category = "Loot")
	const TArray<FLootItemEntry>& GetLootItems() const { return LootItems; }

	UFUNCTION(BlueprintCallable, Category = "Loot")
	void TakeItem(int32 ItemIndex, AActor* Taker);

	UPROPERTY(BlueprintAssignable, Category = "Loot")
	FOnLootItemsChanged OnLootItemsChanged;

	UFUNCTION(BlueprintImplementableEvent, Category = "Loot")
	void OnLootContainerOpened(AActor* Interactor);

protected:
	UPROPERTY(VisibleAnywhere, Category = "Loot")
	TObjectPtr<USphereComponent> InteractionSphere;

	UPROPERTY(EditDefaultsOnly, Category = "Loot")
	FText PromptText = FText::FromString(TEXT("루팅"));

	UPROPERTY(ReplicatedUsing = OnRep_LootItems, BlueprintReadOnly, Category = "Loot")
	TArray<FLootItemEntry> LootItems;

	UFUNCTION()
	void OnRep_LootItems();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 다 털리면 컨테이너 정리
	void DestroyIfEmpty();
};
