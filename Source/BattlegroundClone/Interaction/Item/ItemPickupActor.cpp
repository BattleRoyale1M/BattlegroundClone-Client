#include "ItemPickupActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "Character/DropCharacter.h"

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

void AItemPickupActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AItemPickupActor, ItemHandle);
	DOREPLIFETIME(AItemPickupActor, Quantity);
}


void AItemPickupActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyItemMesh();
}

void AItemPickupActor::BeginPlay()
{
	Super::BeginPlay();
	ApplyItemMesh();
}

void AItemPickupActor::ApplyItemMesh()
{
	if (const FItemRow* Row = GetItemRow(); Row && Row->PickupMesh)
	{
		Mesh->SetStaticMesh(Row->PickupMesh);
	}
}

const FItemRow* AItemPickupActor::GetItemRow() const
{
	return ItemHandle.GetRow<FItemRow>(TEXT("ItemPickupActor"));
}

FText AItemPickupActor::GetDisplayName() const
{
	const FItemRow* Row = GetItemRow();
	return Row ? Row->DisplayName : FText::GetEmpty();
}

UTexture2D* AItemPickupActor::GetIcon() const
{
	const FItemRow* Row = GetItemRow();
	return Row ? Row->Icon.Get() : nullptr;
}

void AItemPickupActor::SetQuantity(int32 NewQuantity)
{
	if (HasAuthority())
	{
		Quantity = FMath::Max(1, NewQuantity);
	}
}

void AItemPickupActor::Interact_Implementation(AActor* Interactor)
{
	if (!HasAuthority())
	{
		return;
	}
	if (ADropCharacter* Character = Cast<ADropCharacter>(Interactor))
	{
		Character->TryPickup(this);
	}
}

FText AItemPickupActor::GetInteractionPromptText_Implementation() const
{
	return GetDisplayName();
}