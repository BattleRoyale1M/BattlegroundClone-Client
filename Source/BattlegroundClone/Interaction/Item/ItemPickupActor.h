#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable/InteractableInterface.h"
#include "ItemPickupActor.generated.h"

class UStaticMeshComponent;
class USphereComponent;
class AWeaponBase;

UCLASS()
class BATTLEGROUNDCLONE_API AItemPickupActor : public AActor, public IInteractableInterface
{
	GENERATED_BODY()
	
public:
	AItemPickupActor();
	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual FText GetInteractionPromptText_Implementation() const override;
	
protected:
	UPROPERTY(VisibleAnywhere, Category = "Pickup")
	TObjectPtr<UStaticMeshComponent> Mesh;
	
	UPROPERTY(VisibleAnywhere, Category = "Pickup")
	TObjectPtr<USphereComponent> InteractionSphere;
	
	UPROPERTY(EditDefaultsOnly, Category = "Pickup")
	int32 TargetSlotIndex = 0;
	
	UPROPERTY(EditDefaultsOnly, Category = "Pickup")
	TSubclassOf<AWeaponBase> WeaponClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "Pickup")
	FText DisplayName = FText::FromString(TEXT("Weapon"));
};