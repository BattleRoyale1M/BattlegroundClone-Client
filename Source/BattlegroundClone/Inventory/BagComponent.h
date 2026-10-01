#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BagComponent.generated.h"

class UDataTable;
class AWeaponBase;

struct FItemRow;

USTRUCT(BlueprintType)
struct FBagEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Bag")
	FName ItemRowName;

	UPROPERTY(BlueprintReadOnly, Category = "Bag")
	int32 Count = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBagChanged);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class BATTLEGROUNDCLONE_API UBagComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBagComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// --- 서버 전용 ---
	int32 AddItem(FName RowName, int32 Count);
	bool RemoveItem(FName RowName, int32 Count);

	// --- 조회 ---
	UFUNCTION(BlueprintPure, Category = "Bag|View")
	const TArray<FBagEntry>& GetItems() const { return Items; }

	UFUNCTION(BlueprintPure, Category = "Bag|View")
	int32 GetItemCount(FName RowName) const;

	const FItemRow* FindItemRow(FName RowName) const;

	UPROPERTY(BlueprintAssignable, Category = "Bag|View")
	FOnBagChanged OnBagChanged;
	
	UFUNCTION()
	void OnRep_Items();
	
	// --- 슬롯교체 ---
	FName FindWeaponRowName(TSubclassOf<AWeaponBase> WeaponClass) const;
	
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Bag")
	TObjectPtr<UDataTable> ItemTable;

	UPROPERTY(EditDefaultsOnly, Category = "Bag")
	int32 MaxSlots = 20;
	
	UPROPERTY(ReplicatedUsing = OnRep_Items)
	TArray<FBagEntry> Items;
};