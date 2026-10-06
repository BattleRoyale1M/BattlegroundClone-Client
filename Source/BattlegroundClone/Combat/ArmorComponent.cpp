#include "Combat/ArmorComponent.h"
#include "Net/UnrealNetwork.h"

UArmorComponent::UArmorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UArmorComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UArmorComponent, Vest);
	DOREPLIFETIME(UArmorComponent, Helmet);
}

FName UArmorComponent::Equip(FName RowName, const FItemRow& Row)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return NAME_None;
	}
	FEquippedArmor& Target = Row.EquipSlot == EBGEquipSlot::Helmet ? Helmet : Vest;
	const FName Previous = Target.IsEquipped() ? Target.RowName : NAME_None;
	Target.RowName = RowName;
	Target.DamageReduction = Row.DamageReduction;
	Target.MaxDurability = Row.MaxDurability;
	Target.Durability = Row.MaxDurability;
	OnArmorChanged.Broadcast();
	return Previous;
}

float UArmorComponent::AbsorbDamage(float Damage, bool bHead)
{
	FEquippedArmor& Target = bHead ? Helmet : Vest;
	if (!GetOwner() || !GetOwner()->HasAuthority() || Damage <= 0.f || !Target.IsEquipped())
	{
		return Damage;
	}
	const float Reduced = Damage * (1.f - Target.DamageReduction);
	Target.Durability = FMath::Max(0.f, Target.Durability - Damage);
	if (Target.Durability <= 0.f)
	{
		Target = FEquippedArmor();
	}
	OnArmorChanged.Broadcast();
	return Reduced;
}

void UArmorComponent::OnRep_Armor()
{
	OnArmorChanged.Broadcast();
}