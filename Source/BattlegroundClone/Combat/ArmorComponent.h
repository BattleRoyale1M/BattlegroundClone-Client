#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interaction/Item/ItemTypes.h"
#include "ArmorComponent.generated.h"

USTRUCT(BlueprintType)
struct FEquippedArmor
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Armor")
	FName RowName;

	UPROPERTY(BlueprintReadOnly, Category = "Armor")
	float DamageReduction = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Armor")
	float Durability = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Armor")
	float MaxDurability = 0.f;

	bool IsEquipped() const
	{
		return !RowName.IsNone() && Durability > 0.f;
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnArmorChanged);

UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class BATTLEGROUNDCLONE_API UArmorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UArmorComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	FName Equip(FName RowName, const FItemRow& Row);
	float AbsorbDamage(float Damage, bool bHead);

	UFUNCTION(BlueprintPure, Category = "Armor") const FEquippedArmor& GetVest() const { return Vest; }
	UFUNCTION(BlueprintPure, Category = "Armor") const FEquippedArmor& GetHelmet() const { return Helmet; }

	UPROPERTY(BlueprintAssignable, Category = "Armor")
	FOnArmorChanged OnArmorChanged;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_Armor)
	FEquippedArmor Vest;

	UPROPERTY(ReplicatedUsing = OnRep_Armor)
	FEquippedArmor Helmet;

	UFUNCTION()
	void OnRep_Armor();
};
