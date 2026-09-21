#include "ItemPickupActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Character/DropCharacter.h"
#include "Weapon/WeaponBase.h"
#include "Weapon/WeaponInventoryComponent.h"

AItemPickupActor::AItemPickupActor()
{
	PrimaryActorTick.bCanEverTick = false;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	
	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetupAttachment(Mesh);
	InteractionSphere->SetSphereRadius(150.f);
	InteractionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	
	bReplicates = true;
}

void AItemPickupActor::Interact_Implementation(AActor* Interactor)
{
	if (!HasAuthority() || !WeaponClass)
	{
		return;
	}
	if (ADropCharacter* Character = Cast<ADropCharacter>(Interactor))
	{
		if (UWeaponInventoryComponent* Inv = Character->GetWeaponInventory())
		{
			Inv->EquipWeaponClassInSlots(AllowedSlots, WeaponClass);
			Destroy();
		}
	}
}

FText AItemPickupActor::GetInteractionPromptText_Implementation() const
{
	return DisplayName;
}