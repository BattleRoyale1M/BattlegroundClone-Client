#pragma once
#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ItemTypes.generated.h"
class AWeaponBase;
class UTexture2D;
class UStaticMesh;

UENUM(BlueprintType)
enum class EBGItemType : uint8
{
	Weapon,
	Heal,
	Boost,
	Ammo,
	Equipment,
};
USTRUCT(BlueprintType)
struct FItemRow : public FTableRowBase
{
	GENERATED_BODY()
	
	// --- 공통 ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	EBGItemType ItemType = EBGItemType::Heal;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UTexture2D> Icon;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UStaticMesh> PickupMesh;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "1"))
	int32 MaxStack = 1;
	
	// --- 무기 ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Weapon", meta = (EditCondition = "ItemType == EBGItemType::Weapon"))
	TSubclassOf<AWeaponBase> WeaponClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Weapon", meta = (EditCondition = "ItemType == EBGItemType::Weapon"))
	TArray<int32> AllowedSlots;
	
	// --- 회복/부스터 공통 ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Use")
	float UseDuration = 6.f;

	// --- 회복 아이템 전용 (붕대, 구급상자) ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Heal", meta = (EditCondition = "ItemType == EBGItemType::Heal"))
	float HealAmount = 75.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Heal", meta = (EditCondition = "ItemType == EBGItemType::Heal"))
	float HealCap = 75.f;

	// --- 부스터 아이템 전용 (진통제, 에너지 드링크) ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Boost", meta = (EditCondition = "ItemType == EBGItemType::Boost"))
	float BoostGainAmount = 40.f;
};