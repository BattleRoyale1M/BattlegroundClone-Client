#include "Interaction/InteractionComponent.h"

#include "Character/DropCharacter.h"
#include "Core/DropPlayerController.h"
#include "Core/Enums/DropTypes.h"
#include "Inventory/BagComponent.h"
#include "Weapon/WeaponBase.h"
#include "Weapon/WeaponInventoryComponent.h"
#include "Combat/HealthComponent.h"
#include "Interaction/Interactable/InteractableInterface.h"
#include "Interaction/Item/ItemPickupActor.h"
#include "Interaction/Item/ItemTypes.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Engine/World.h"

UInteractionComponent::UInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UInteractionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UInteractionComponent, UsingItemRow, COND_OwnerOnly);
}

void UInteractionComponent::BeginPlay()
{
	Super::BeginPlay();
	if (ACharacter* OwnerChar = Cast<ACharacter>(GetOwner()))
	{
		if (UCapsuleComponent* Capsule = OwnerChar->GetCapsuleComponent())
		{
			Capsule->OnComponentBeginOverlap.AddDynamic(this, &UInteractionComponent::OnInteractableBeginOverlap);
			Capsule->OnComponentEndOverlap.AddDynamic(this, &UInteractionComponent::OnInteractableEndOverlap);
		}
	}
}

ADropCharacter* UInteractionComponent::GetOwnerDropCharacter() const
{
	return Cast<ADropCharacter>(GetOwner());
}

void UInteractionComponent::OnInteractableBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor || !OtherActor->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()))
	{
		return;
	}
	NearbyInteractables.AddUnique(OtherActor);
	UpdateCurrentInteractable();
	const ADropCharacter* Character = GetOwnerDropCharacter();
	if (Character && Character->IsLocallyControlled())
	{
		OnNearbyPickupsChanged.Broadcast();
	}
}

void UInteractionComponent::OnInteractableEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	NearbyInteractables.Remove(OtherActor);
	UpdateCurrentInteractable();
	const ADropCharacter* Character = GetOwnerDropCharacter();
	if (Character && Character->IsLocallyControlled())
	{
		OnNearbyPickupsChanged.Broadcast();
	}
}

void UInteractionComponent::UpdateCurrentInteractable()
{
	AActor* Best = NearbyInteractables.Num() > 0 ? NearbyInteractables[0] : nullptr;
	if (Best == CurrentInteractable)
	{
		return;
	}
	CurrentInteractable = Best;

	const ADropCharacter* Character = GetOwnerDropCharacter();
	if (Character && Character->IsLocallyControlled())
	{
		const FText Prompt = CurrentInteractable
			? IInteractableInterface::Execute_GetInteractionPromptText(CurrentInteractable)
			: FText::GetEmpty();
		OnInteractableChanged.Broadcast(Prompt);
	}
}

void UInteractionComponent::OnInteractPressed()
{
	ADropCharacter* Character = GetOwnerDropCharacter();
	if (!Character || !Character->CanActOnGround() || !CurrentInteractable)
	{
		return;
	}
	ServerInteract(CurrentInteractable);
}

void UInteractionComponent::ServerInteract_Implementation(AActor* InteractActor)
{
	ADropCharacter* Character = GetOwnerDropCharacter();
	if (Character && InteractActor && InteractActor->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()))
	{
		IInteractableInterface::Execute_Interact(InteractActor, Character);
	}
}

TArray<AItemPickupActor*> UInteractionComponent::GetNearbyPickups() const
{
	TArray<AItemPickupActor*> Result;
	for (AActor* Actor : NearbyInteractables)
	{
		if (AItemPickupActor* Pickup = Cast<AItemPickupActor>(Actor); IsValid(Pickup))
		{
			Result.Add(Pickup);
		}
	}
	return Result;
}

void UInteractionComponent::RequestPickup(AItemPickupActor* Pickup)
{
	ADropCharacter* Character = GetOwnerDropCharacter();
	if (!Pickup || !Character || !Character->CanAct())
	{
		return;
	}
	ServerPickup(Pickup);
}

void UInteractionComponent::RequestEquipFromBag(int32 BagIndex, int32 TargetSlot)
{
	ADropCharacter* Character = GetOwnerDropCharacter();
	if (!Character || !Character->CanAct()) return;
	ServerEquipFromBag(BagIndex, TargetSlot);
}

void UInteractionComponent::RequestUnequipToBag(int32 SlotIndex)
{
	ADropCharacter* Character = GetOwnerDropCharacter();
	if (!Character || !Character->CanAct()) return;
	ServerUnequipToBag(SlotIndex);
}

void UInteractionComponent::ServerEquipFromBag_Implementation(int32 BagIndex, int32 TargetSlot)
{
	ADropCharacter* Character = GetOwnerDropCharacter();
	if (!Character || !Character->CanAct() || !Character->BagComp || !Character->WeaponInventory)
	{
		return;
	}
	UBagComponent* BagComp = Character->BagComp;
	UWeaponInventoryComponent* WeaponInventory = Character->WeaponInventory;

	const TArray<FBagEntry>& Items = BagComp->GetItems();
	if (!Items.IsValidIndex(BagIndex))
	{
		return;
	}
	const FName RowName = Items[BagIndex].ItemRowName;
	const FItemRow* Row = BagComp->FindItemRow(RowName);
	if (!Row || Row->ItemType != EBGItemType::Weapon || !Row->WeaponClass) return;
	if (Row->AllowedSlots.Num() > 0 && !Row->AllowedSlots.Contains(TargetSlot)) return;
	const TSubclassOf<AWeaponBase> OldClass = WeaponInventory->GetSlotWeaponClass(TargetSlot);
	if (!BagComp->RemoveItem(RowName, 1)) return;
	WeaponInventory->EquipWeaponClassAtSlot(TargetSlot, Row->WeaponClass);
	if (OldClass)
	{
		const FName OldRow = BagComp->FindWeaponRowName(OldClass);
		if (!OldRow.IsNone()) BagComp->AddItem(OldRow, 1);
	}
}

void UInteractionComponent::ServerUnequipToBag_Implementation(int32 SlotIndex)
{
	ADropCharacter* Character = GetOwnerDropCharacter();
	if (!Character || !Character->CanAct() || !Character->BagComp || !Character->WeaponInventory)
	{
		return;
	}
	UBagComponent* BagComp = Character->BagComp;
	UWeaponInventoryComponent* WeaponInventory = Character->WeaponInventory;

	const TSubclassOf<AWeaponBase> OldClass = WeaponInventory->GetSlotWeaponClass(SlotIndex);
	if (!OldClass)
	{
		return;
	}
	const FName RowName = BagComp->FindWeaponRowName(OldClass);
	if (RowName.IsNone())
	{
		return;
	}
	if (BagComp->AddItem(RowName, 1) > 0)
	{
		return;
	}
	WeaponInventory->ClearSlot(SlotIndex);
}

void UInteractionComponent::ServerPickup_Implementation(AItemPickupActor* Pickup)
{
	TryPickup(Pickup);
}

bool UInteractionComponent::TryPickup(AItemPickupActor* Pickup)
{
	ADropCharacter* Character = GetOwnerDropCharacter();
	if (!Character || !Character->HasAuthority() || !Character->CanAct() || !IsValid(Pickup))
	{
		return false;
	}
	if (FVector::Dist(Character->GetActorLocation(), Pickup->GetActorLocation()) > MaxPickupDistance)
	{
		return false;
	}
	const FItemRow* Row = Pickup->GetItemRow();
	if (!Row)
	{
		return false;
	}

	if (Row->ItemType == EBGItemType::Weapon)
	{
		if (Character->WeaponInventory && Character->WeaponInventory->TryEquipToEmptySlot(Row->AllowedSlots, Row->WeaponClass))
		{
			Pickup->Destroy();
			return true;
		}
	}

	if (!Character->BagComp)
	{
		return false;
	}
	const int32 Left = Character->BagComp->AddItem(Pickup->GetItemRowName(), Pickup->GetQuantity());
	if (Left == Pickup->GetQuantity())
	{
		return false;
	}
	if (Left > 0) Pickup->SetQuantity(Left);
	else          Pickup->Destroy();
	return true;
}

void UInteractionComponent::RequestUseItem(FName RowName)
{
	ADropCharacter* Character = GetOwnerDropCharacter();
	if (!Character || !Character->CanAct() || RowName.IsNone())
	{
		return;
	}
	ServerUseItem(RowName);
}

void UInteractionComponent::ServerUseItem_Implementation(FName RowName)
{
	ADropCharacter* Character = GetOwnerDropCharacter();
	UBagComponent* BagComp = Character ? Character->BagComp.Get() : nullptr;
	UHealthComponent* HealthComp = Character ? Character->GetHealthComp() : nullptr;

	if (!Character || !Character->CanActOnGround() || IsUsingItem()
		|| !BagComp || BagComp->GetItemCount(RowName) <= 0)
	{
		return;
	}
	const FItemRow* Row = BagComp->FindItemRow(RowName);
	if (!Row || (Row->ItemType != EBGItemType::Heal && Row->ItemType != EBGItemType::Boost))
	{
		return;
	}
	if (Row->ItemType == EBGItemType::Heal && HealthComp
		&& HealthComp->GetHealth() >= FMath::Min(Row->HealCap, HealthComp->GetMaxHealth()))
	{
		if (ADropPlayerController* PC = Cast<ADropPlayerController>(Character->GetController()))
		{
			PC->ClientShowCenterNotification(FText::GetEmpty(), FText::FromString(TEXT("체력이 충분합니다")), FLinearColor(1.f, 0.3f, 0.1f));
		}
		return;
	}

	if (Character->WeaponInventory)
	{
		Character->WeaponInventory->StopFire();
	}
	UsingItemRow = RowName;
	const float Duration = FMath::Max(Row->UseDuration, 0.1f);
	GetWorld()->GetTimerManager().SetTimer(UseItemTimer, this, &UInteractionComponent::FinishUseItem, Duration, false);

	MulticastPlayUseMontage(Row->UseMontage, Duration);
	ClientItemUseStarted(Row->DisplayName, Duration);
}

void UInteractionComponent::FinishUseItem()
{
	ADropCharacter* Character = GetOwnerDropCharacter();
	UBagComponent* BagComp = Character ? Character->BagComp.Get() : nullptr;
	UHealthComponent* HealthComp = Character ? Character->GetHealthComp() : nullptr;

	const FName RowName = UsingItemRow;
	UsingItemRow = NAME_None;

	const FItemRow* Row = BagComp ? BagComp->FindItemRow(RowName) : nullptr;
	if (Row && Character && Character->CanAct() && HealthComp && BagComp->RemoveItem(RowName, 1))
	{
		if (Row->ItemType == EBGItemType::Heal)
		{
			HealthComp->Heal(Row->HealAmount, Row->HealCap);
		}
		else if (Row->ItemType == EBGItemType::Boost)
		{
			HealthComp->AddBoost(Row->BoostGainAmount);
		}
	}
	ClientItemUseEnded(true);
}

void UInteractionComponent::RequestCancelUseItem()
{
	if (!IsUsingItem())
	{
		return;
	}
	ServerCancelUseItem();
	UsingItemRow = NAME_None;
}

void UInteractionComponent::ServerCancelUseItem_Implementation()
{
	CancelUseItem();
}

void UInteractionComponent::CancelUseItem()
{
	const ADropCharacter* Character = GetOwnerDropCharacter();
	if (!Character || !Character->HasAuthority() || !IsUsingItem())
	{
		return;
	}
	const UBagComponent* BagComp = Character->BagComp;

	GetWorld()->GetTimerManager().ClearTimer(UseItemTimer);
	const FItemRow* Row = BagComp ? BagComp->FindItemRow(UsingItemRow) : nullptr;
	UsingItemRow = NAME_None;

	MulticastStopUseMontage(Row ? Row->UseMontage.Get() : nullptr);
	ClientItemUseEnded(false);
}

void UInteractionComponent::MulticastPlayUseMontage_Implementation(UAnimMontage* Montage, float Duration)
{
	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	UAnimInstance* Anim = (OwnerChar && OwnerChar->GetMesh()) ? OwnerChar->GetMesh()->GetAnimInstance() : nullptr;
	if (Anim && Montage && Duration > 0.f)
	{
		Anim->Montage_Play(Montage, Montage->GetPlayLength() / Duration);
	}
}

void UInteractionComponent::MulticastStopUseMontage_Implementation(UAnimMontage* Montage)
{
	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	UAnimInstance* Anim = (OwnerChar && OwnerChar->GetMesh()) ? OwnerChar->GetMesh()->GetAnimInstance() : nullptr;
	if (Anim && Montage)
	{
		Anim->Montage_Stop(0.2f, Montage);
	}
}

void UInteractionComponent::ClientItemUseStarted_Implementation(const FText& ItemName, float Duration)
{
	OnItemUseStarted.Broadcast(ItemName, Duration);
}

void UInteractionComponent::ClientItemUseEnded_Implementation(bool bCompleted)
{
	OnItemUseEnded.Broadcast(bCompleted);
}
