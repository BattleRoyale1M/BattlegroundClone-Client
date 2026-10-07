#include "Combat/ArmorComponent.h"
#include "Net/UnrealNetwork.h"

#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

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

void UArmorComponent::BeginPlay()
{
	Super::BeginPlay();
	VestMeshComp = CreateArmorMesh(VestSocket, VestOffset);
	HelmetMeshComp = CreateArmorMesh(HelmetSocket, HelmetOffset);
	RefreshVisuals();
}

UStaticMeshComponent* UArmorComponent::CreateArmorMesh(FName Socket, const FTransform& Offset)
{
	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	if (!OwnerChar || !OwnerChar->GetMesh())
	{
		return nullptr;
	}
	UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(OwnerChar);
	OwnerChar->AddInstanceComponent(Comp);
	Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Comp->RegisterComponent();
	Comp->AttachToComponent(OwnerChar->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, Socket);
	Comp->SetRelativeTransform(Offset);
	return Comp;
}

void UArmorComponent::RefreshVisuals()
{
	if (VestMeshComp) VestMeshComp->SetStaticMesh(Vest.IsEquipped() ? Vest.Mesh.Get() : nullptr);
	if (HelmetMeshComp) HelmetMeshComp->SetStaticMesh(Helmet.IsEquipped() ? Helmet.Mesh.Get() : nullptr);
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
	Target.Mesh = Row.PickupMesh;
	RefreshVisuals();
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
	RefreshVisuals();
	OnArmorChanged.Broadcast();
	return Reduced;
}

void UArmorComponent::OnRep_Armor()
{
	RefreshVisuals();
	OnArmorChanged.Broadcast();
}