#include "Input/InputBindingComponent.h"

#include "Character/DropCharacter.h"
#include "Weapon/WeaponInventoryComponent.h"
#include "Interaction/InteractionComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"

UInputBindingComponent::UInputBindingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

ADropCharacter* UInputBindingComponent::GetOwnerDropCharacter() const
{
	return Cast<ADropCharacter>(GetOwner());
}

void UInputBindingComponent::AddMappingContext(APlayerController* PC)
{
	if (!PC || !DefaultMappingContext)
	{
		return;
	}
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}
}

void UInputBindingComponent::SetupInput(UInputComponent* PlayerInputComponent)
{
	ADropCharacter* Character = GetOwnerDropCharacter();
	if (!Character)
	{
		return;
	}

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EIC)
	{
		return;
	}

	if (MoveAction)
	{
		EIC->BindAction(MoveAction, ETriggerEvent::Triggered, Character, &ADropCharacter::Move);
	}
	if (LookAction)
	{
		EIC->BindAction(LookAction, ETriggerEvent::Triggered, Character, &ADropCharacter::Look);
	}
	if (MouseLookAction)
	{
		EIC->BindAction(MouseLookAction, ETriggerEvent::Triggered, Character, &ADropCharacter::Look);
	}
	if (JumpAction)
	{
		EIC->BindAction(JumpAction, ETriggerEvent::Started,   Character, &ADropCharacter::OnJumpPressed);
		EIC->BindAction(JumpAction, ETriggerEvent::Completed, Character, &ACharacter::StopJumping);
	}
	if (FastFallAction)
	{
		EIC->BindAction(FastFallAction, ETriggerEvent::Started, Character, &ADropCharacter::OnFastFallPressed);
		EIC->BindAction(FastFallAction, ETriggerEvent::Completed, Character, &ADropCharacter::OnFastFallReleased);
	}
	if (ParachuteAction)
	{
		EIC->BindAction(ParachuteAction, ETriggerEvent::Started, Character, &ADropCharacter::OnParachutePressed);
	}
	if (FireAction)
	{
		EIC->BindAction(FireAction, ETriggerEvent::Started, Character, &ADropCharacter::StartFire);
		EIC->BindAction(FireAction, ETriggerEvent::Completed, Character, &ADropCharacter::StopFire);
	}
	if (ReloadAction)
	{
		EIC->BindAction(ReloadAction, ETriggerEvent::Started, Character, &ADropCharacter::OnReloadPressed);
	}
	if (AimAction)
	{
		EIC->BindAction(AimAction, ETriggerEvent::Started,   Character, &ADropCharacter::OnAimPressed);
		EIC->BindAction(AimAction, ETriggerEvent::Completed, Character, &ADropCharacter::OnAimReleased);
	}
	if (ChangeFireModeAction && Character->WeaponInventory)
	{
		EIC->BindAction(ChangeFireModeAction, ETriggerEvent::Started, Character->WeaponInventory.Get(), &UWeaponInventoryComponent::ChangeFireMode);
	}
	if (Character->WeaponInventory)
	{
		for (int32 i = 0; i < WeaponSlotActions.Num(); ++i)
		{
			if (WeaponSlotActions[i])
			{
				EIC->BindAction(WeaponSlotActions[i], ETriggerEvent::Started, Character->WeaponInventory.Get(), &UWeaponInventoryComponent::SwitchWeaponSlot, i);
			}
		}
	}
	if (InteractAction && Character->InteractionComp)
	{
		EIC->BindAction(InteractAction, ETriggerEvent::Started, Character->InteractionComp.Get(), &UInteractionComponent::OnInteractPressed);
	}
	if (CrawlAction)
	{
		EIC->BindAction(CrawlAction, ETriggerEvent::Started, Character, &ADropCharacter::OnPronePressed);
	}
}
