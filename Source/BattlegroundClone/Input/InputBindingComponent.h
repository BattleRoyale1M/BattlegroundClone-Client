#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputBindingComponent.generated.h"

class ADropCharacter;
class APlayerController;
class UInputComponent;
class UInputMappingContext;
class UInputAction;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BATTLEGROUNDCLONE_API UInputBindingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInputBindingComponent();

	void SetupInput(UInputComponent* PlayerInputComponent);
	void AddMappingContext(APlayerController* PC);

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MouseLookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> FastFallAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> ParachuteAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> FireAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> ReloadAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> AimAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> ChangeFireModeAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> CrawlAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TArray<TObjectPtr<UInputAction>> WeaponSlotActions;

private:
	ADropCharacter* GetOwnerDropCharacter() const;
};
