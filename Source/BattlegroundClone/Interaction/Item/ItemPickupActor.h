#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DataTable.h"
#include "Interaction/Interactable/InteractableInterface.h"
#include "Interaction/Item/ItemTypes.h"
#include "ItemPickupActor.generated.h"

class UStaticMeshComponent;
class USphereComponent;
class UTexture2D;

UCLASS()
class BATTLEGROUNDCLONE_API AItemPickupActor : public AActor, public IInteractableInterface
{
	GENERATED_BODY()
	
public:
	AItemPickupActor();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual FText GetInteractionPromptText_Implementation() const override;
	
	const FItemRow* GetItemRow() const;
	UFUNCTION(BlueprintPure, Category = "Pickup")
	FName GetItemRowName() const { return ItemHandle.RowName; }
	UFUNCTION(BlueprintPure, Category = "Pickup")
	int32 GetQuantity() const { return Quantity; }
	UFUNCTION(BlueprintPure, Category = "Pickup")
	FText GetDisplayName() const;
	UFUNCTION(BlueprintPure, Category = "Pickup")
	UTexture2D* GetIcon() const;
	void SetQuantity(int32 NewQuantity);
	
protected:
	virtual void BeginPlay() override;
	void ApplyItemMesh();
	UPROPERTY(VisibleAnywhere, Category = "Pickup")
	TObjectPtr<UStaticMeshComponent> Mesh;
	
	UPROPERTY(VisibleAnywhere, Category = "Pickup")
	TObjectPtr<USphereComponent> InteractionSphere;
	
	UPROPERTY(EditAnywhere, Replicated, Category = "Pickup", meta = (RowType = "/Script/BattlegroundClone.ItemRow"))
	FDataTableRowHandle ItemHandle;
	UPROPERTY(EditAnywhere, Replicated, Category = "Pickup", meta = (ClampMin = "1"))
	int32 Quantity = 1;
};