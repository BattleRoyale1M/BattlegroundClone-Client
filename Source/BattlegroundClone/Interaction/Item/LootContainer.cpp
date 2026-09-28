#include "Interaction/Item/LootContainer.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "Weapon/WeaponBase.h"
#include "Character/DropCharacter.h"
#include "Weapon/WeaponInventoryComponent.h"

ALootContainer::ALootContainer()
{
	PrimaryActorTick.bCanEverTick = false;

	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	SetRootComponent(InteractionSphere);
	InteractionSphere->SetSphereRadius(150.f);
	InteractionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	bReplicates = true;
}

void ALootContainer::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALootContainer, LootItems);
}

void ALootContainer::AddWeaponLoot(TSubclassOf<AWeaponBase> WeaponClass, const TArray<int32>& AllowedSlots)
{
	if (!HasAuthority() || !WeaponClass)
	{
		return;
	}

	FLootItemEntry Entry;
	Entry.WeaponClass = WeaponClass;
	Entry.AllowedSlots = AllowedSlots;
	if (const AWeaponBase* CDO = WeaponClass->GetDefaultObject<AWeaponBase>())
	{
		Entry.DisplayName = CDO->GetWeaponDisplayName();
		Entry.Icon = CDO->GetWeaponIcon();
	}
	LootItems.Add(Entry);
	OnLootItemsChanged.Broadcast();
}

void ALootContainer::TakeItem(int32 ItemIndex, AActor* Taker)
{
	if (!HasAuthority() || !LootItems.IsValidIndex(ItemIndex))
	{
		return;
	}

	const FLootItemEntry Entry = LootItems[ItemIndex];
	if (ADropCharacter* Character = Cast<ADropCharacter>(Taker))
	{
		if (UWeaponInventoryComponent* Inv = Character->GetWeaponInventory())
		{
			Inv->EquipWeaponClassInSlots(Entry.AllowedSlots, Entry.WeaponClass);
		}
	}

	LootItems.RemoveAt(ItemIndex);
	OnLootItemsChanged.Broadcast();
	DestroyIfEmpty();
}

void ALootContainer::OnRep_LootItems()
{
	OnLootItemsChanged.Broadcast();
}

void ALootContainer::DestroyIfEmpty()
{
	if (LootItems.Num() == 0)
	{
		Destroy();
	}
}

void ALootContainer::Interact_Implementation(AActor* Interactor)
{
	OnLootContainerOpened(Interactor);
}

FText ALootContainer::GetInteractionPromptText_Implementation() const
{
	return PromptText;
}
