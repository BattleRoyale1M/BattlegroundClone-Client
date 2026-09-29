#include "Inventory/BagComponent.h"
#include "Interaction/Item/ItemTypes.h"
#include "Engine/DataTable.h"
#include "Net/UnrealNetwork.h"

UBagComponent::UBagComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UBagComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UBagComponent, Items, COND_OwnerOnly);   // 내 가방은 나만
}

const FItemRow* UBagComponent::FindItemRow(FName RowName) const
{
	return ItemTable ? ItemTable->FindRow<FItemRow>(RowName, TEXT("BagComponent")) : nullptr;
}

int32 UBagComponent::AddItem(FName RowName, int32 Count)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || Count <= 0)
	{
		return Count;
	}
	const FItemRow* Row = FindItemRow(RowName);
	if (!Row)
	{
		return Count;
	}
	const int32 MaxStack = FMath::Max(1, Row->MaxStack);
	const int32 Before = Count;

	for (FBagEntry& Entry : Items)
	{
		if (Count <= 0) break;
		if (Entry.ItemRowName != RowName || Entry.Count >= MaxStack) continue;

		const int32 Add = FMath::Min(Count, MaxStack - Entry.Count);
		Entry.Count += Add;
		Count -= Add;
	}

	while (Count > 0 && Items.Num() < MaxSlots)
	{
		FBagEntry NewEntry;
		NewEntry.ItemRowName = RowName;
		NewEntry.Count = FMath::Min(Count, MaxStack);
		Count -= NewEntry.Count;
		Items.Add(NewEntry);
	}

	if (Count != Before)
	{
		OnBagChanged.Broadcast();
	}
	return Count;
}

//뒤쪽 칸부터 차감
bool UBagComponent::RemoveItem(FName RowName, int32 Count)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || Count <= 0 || GetItemCount(RowName) < Count)
	{
		return false;
	}
	for (int32 i = Items.Num() - 1; i >= 0 && Count > 0; --i)
	{
		if (Items[i].ItemRowName != RowName) continue;

		const int32 Sub = FMath::Min(Count, Items[i].Count);
		Items[i].Count -= Sub;
		Count -= Sub;
		if (Items[i].Count <= 0)
		{
			Items.RemoveAt(i);
		}
	}
	OnBagChanged.Broadcast();
	return true;
}

int32 UBagComponent::GetItemCount(FName RowName) const
{
	int32 Total = 0;
	for (const FBagEntry& Entry : Items)
	{
		if (Entry.ItemRowName == RowName) Total += Entry.Count;
	}
	return Total;
}

void UBagComponent::OnRep_Items()
{
	OnBagChanged.Broadcast();
}