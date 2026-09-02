#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Core/Enums/DropTypes.h"
#include "DropCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
class AAirPlane;
class USceneComponent;

UCLASS()
class BATTLEGROUNDCLONE_API ADropCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ADropCharacter();

	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** Current phase of the jump -> freefall -> parachute flow. Drives movement params + AnimBP. */
	UPROPERTY(BlueprintReadOnly, Category = "Drop")
	EDropState DropState = EDropState::Ground;
	
	UFUNCTION(BlueprintCallable, Category = "Drop")
	void SetDropState(EDropState NewState);
	
	/*
	비행기 좌석에 탑승
	*/
	void EnterPlane(AAirPlane* Plane, USceneComponent* Seat);

protected:
	virtual void BeginPlay() override;

	// --- Components -------------------------------------------------------
	/** Third person camera boom. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	/** Follow camera on the end of the boom. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;
	
	UPROPERTY(EditDefaultsOnly, Category = "Camera|InPlane")
	float InPlaneArmLength = 3000.f;
	UPROPERTY(EditDefaultsOnly, Category = "Camera|InPlane")
	FVector InPlaneSocketOffset = FVector(0.f, 0.f, 1000.f);
	UPROPERTY(EditDefaultsOnly, Category = "Camera|InPlane")
	float InPlaneCameraPitch = -22.f;

	// --- Input (assign these in BP_DropCharacter defaults) --------------
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
	
	/*
	현재 탑승 중인 비행기 
	*/
	UPROPERTY()
	TObjectPtr<AAirPlane> BoardedPlane;

	// --- Input handlers ------------------------------------------------
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
};
