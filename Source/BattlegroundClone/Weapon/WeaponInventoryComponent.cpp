#include "Weapon/WeaponInventoryComponent.h"

#include "Weapon/WeaponBase.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Engine/World.h"

UWeaponInventoryComponent::UWeaponInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UWeaponInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UWeaponInventoryComponent, EquippedWeapon);
	DOREPLIFETIME(UWeaponInventoryComponent, WeaponSlotClasses);
	DOREPLIFETIME(UWeaponInventoryComponent, CurrentWeaponIndex);
}

ACharacter* UWeaponInventoryComponent::GetOwnerCharacter() const
{
	return Cast<ACharacter>(GetOwner());
}

void UWeaponInventoryComponent::InitialEquip()
{
	EquipWeaponSlot(0);
}

void UWeaponInventoryComponent::OnRep_EquippedWeapon()
{
	if (!EquippedWeapon)
	{
		return;
	}
	if (ACharacter* OwnerChar = GetOwnerCharacter())
	{
		EquippedWeapon->AttachToComponent(
			OwnerChar->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, WeaponAttachSocket);
	}
}

void UWeaponInventoryComponent::OnRep_WeaponSlotClasses()
{
	OnInventoryChanged.Broadcast();
}

void UWeaponInventoryComponent::OnRep_CurrentWeaponIndex()
{
	OnInventoryChanged.Broadcast();
}

TArray<FWeaponSlotInfo> UWeaponInventoryComponent::GetWeaponSlotInfo() const
{
	TArray<FWeaponSlotInfo> Result;
	Result.Reserve(WeaponSlotClasses.Num());
	for (int32 i = 0; i < WeaponSlotClasses.Num(); ++i)
	{
		FWeaponSlotInfo Info;
		Info.SlotIndex = i;
		Info.WeaponClass = WeaponSlotClasses[i];
		Info.bHasWeapon = WeaponSlotClasses[i] != nullptr;
		Info.bIsEquipped = (i == CurrentWeaponIndex);
		Result.Add(Info);
	}
	return Result;
}

void UWeaponInventoryComponent::EquipWeaponSlot(int32 Index)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}
	if (!WeaponSlotClasses.IsValidIndex(Index) || Index == CurrentWeaponIndex)
	{
		return;
	}
	if (!WeaponSlotClasses[Index])
	{
		if (EquippedWeapon)
		{
			EquippedWeapon->StopFire();
			EquippedWeapon->SetActorHiddenInGame(true);
		}
		CurrentWeaponIndex = Index;
		EquippedWeapon = nullptr;
		if (!WeaponSlots.IsValidIndex(Index) || !WeaponSlots[Index])
		{
			return;
		}
	}
	if (WeaponSlots.Num() != WeaponSlotClasses.Num())
	{
		WeaponSlots.SetNum(WeaponSlotClasses.Num());
	}
	if (EquippedWeapon)
	{
		EquippedWeapon->StopFire();
		EquippedWeapon->SetActorHiddenInGame(true);
	}
	CurrentWeaponIndex = Index;
	
	if (!WeaponSlots[Index])
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Owner;
		SpawnParams.Instigator = Cast<APawn>(Owner);
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		WeaponSlots[Index] = GetWorld()->SpawnActor<AWeaponBase>(WeaponSlotClasses[Index], SpawnParams);
	}
	EquippedWeapon = WeaponSlots[Index];
	if (EquippedWeapon)
	{
		EquippedWeapon->SetActorHiddenInGame(false);
		if (ACharacter* OwnerChar = GetOwnerCharacter())
		{
			EquippedWeapon->AttachToComponent(OwnerChar->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, WeaponAttachSocket);
		}
		EquippedWeapon->SetFireMode(CurrentFireMode);
		MulticastPlayEquipMontage();
	}
}

void UWeaponInventoryComponent::SwitchWeaponSlot(int32 Index)
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}
	if (!Owner->HasAuthority())
	{
		ServerSwtichWeaponSlot(Index);
		return;
	}
	EquipWeaponSlot(Index);
}

void UWeaponInventoryComponent::ServerSwtichWeaponSlot_Implementation(int32 Index)
{
	EquipWeaponSlot(Index);
}

void UWeaponInventoryComponent::MulticastPlayEquipMontage_Implementation()
{
	bIsSwitchingWeapon = true;
	float Duration = 0.f;
	if (EquipAnimMontage)
	{
		Duration = EquipAnimMontage->GetPlayLength();
		if (ACharacter* OwnerChar = GetOwnerCharacter())
		{
			if (UAnimInstance* Anim = OwnerChar->GetMesh() ? OwnerChar->GetMesh()->GetAnimInstance() : nullptr)
			{
				Anim->Montage_Play(EquipAnimMontage);
			}
		}
	}
	GetWorld()->GetTimerManager().SetTimer(EquipTimerHandle, this, &UWeaponInventoryComponent::FinishWeaponSwitch, FMath::Max(Duration, 0.01f), false);
}

void UWeaponInventoryComponent::FinishWeaponSwitch()
{
	bIsSwitchingWeapon = false;
}

void UWeaponInventoryComponent::EquipWeaponClassAtSlot(int32 Index, TSubclassOf<AWeaponBase> NewWeaponClass)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || !NewWeaponClass)
	{
		return;
	}
	if (!WeaponSlotClasses.IsValidIndex(Index))
	{
		WeaponSlotClasses.SetNum(Index + 1);
	}
	WeaponSlotClasses[Index] = NewWeaponClass;

	if (WeaponSlots.IsValidIndex(Index) && WeaponSlots[Index])
	{
		if (WeaponSlots[Index] == EquippedWeapon)
		{
			EquippedWeapon = nullptr;
		}
		WeaponSlots[Index]->Destroy();
		WeaponSlots[Index] = nullptr;
	}

	CurrentWeaponIndex = -1; // 강제로 재장착되게 가드 우회
	EquipWeaponSlot(Index);
}

void UWeaponInventoryComponent::EquipWeaponClassInSlots(const TArray<int32>& AllowedSlots, TSubclassOf<AWeaponBase> NewWeaponClass)
{
	if (AllowedSlots.Num() == 0 || !NewWeaponClass)
	{
		return;
	}

	int32 TargetIndex = -1;
	for (int32 Slot : AllowedSlots)
	{
		if (!WeaponSlotClasses.IsValidIndex(Slot) || !WeaponSlotClasses[Slot])
		{
			TargetIndex = Slot;
			break;
		}
	}
	if (TargetIndex == -1)
	{
		TargetIndex = AllowedSlots.Contains(CurrentWeaponIndex) ? CurrentWeaponIndex : AllowedSlots[0];
	}

	EquipWeaponClassAtSlot(TargetIndex, NewWeaponClass);
}


void UWeaponInventoryComponent::ChangeFireMode()
{
	uint8 NextMode = (static_cast<uint8>(CurrentFireMode) + 1) % 2;
	CurrentFireMode = static_cast<EFireMode>(NextMode);

	if (EquippedWeapon)
	{
		EquippedWeapon->SetFireMode(CurrentFireMode);
	}
	if (OnFireModeChanged.IsBound())
	{
		OnFireModeChanged.Broadcast(CurrentFireMode);
	}
}

void UWeaponInventoryComponent::StartFire()
{
	if (!EquippedWeapon || bIsSwitchingWeapon)
	{
		return;
	}
	EquippedWeapon->StartFire();
}

void UWeaponInventoryComponent::StopFire()
{
	if (EquippedWeapon)
	{
		EquippedWeapon->StopFire();
	}
}

void UWeaponInventoryComponent::OnReloadPressed()
{
	if (bIsSwitchingWeapon)
	{
		return;
	}
	if (EquippedWeapon)
	{
		EquippedWeapon->StartReload();
	}
}

void UWeaponInventoryComponent::PlayFireMontage()
{
	if (!FireAnimMontage) return;
	ACharacter* OwnerChar = GetOwnerCharacter();
	UAnimInstance* Anim = (OwnerChar && OwnerChar->GetMesh()) ? OwnerChar->GetMesh()->GetAnimInstance() : nullptr;
	if (!Anim) return;
	Anim->Montage_Play(FireAnimMontage);
}

void UWeaponInventoryComponent::HandleReloadStarted(float Duration)
{
	if (!ReloadAnimMontage) return;
	ACharacter* OwnerChar = GetOwnerCharacter();
	UAnimInstance* Anim = (OwnerChar && OwnerChar->GetMesh()) ? OwnerChar->GetMesh()->GetAnimInstance() : nullptr;
	if (!Anim) return;
	const float MontageLen = ReloadAnimMontage->GetPlayLength();
	const float Rate = (Duration > 0.f && MontageLen > 0.f) ? (MontageLen / Duration) : 1.f;
	Anim->Montage_Play(ReloadAnimMontage, Rate);
}
