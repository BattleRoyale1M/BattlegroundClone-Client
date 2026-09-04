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
	
	/*
	낙하산
	*/
	UFUNCTION(BlueprintCallable, Category = "Drop")
	void BeginFreefall();
	
	UFUNCTION(BlueprintCallable, Category = "Drop")
	void DeployParachute();
	
	UFUNCTION(BlueprintImplementableEvent, Category = "Drop")
	void OnDropStateChanged(EDropState NewState, EDropState OldState);
	
	float GroundDistance() const;
	void  UpdateFreefall(float Dt);
	void  UpdateParachute(float Dt);


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
	
	/*
	낙하산
	*/
	void OnJumpPressed();
	
	UPROPERTY(EditDefaultsOnly, Category = "Drop|Freefall")
	float FreefallMinSpeed = 3600.f; // 슈가글라이더 자세
	
	UPROPERTY(EditDefaultsOnly, Category = "Drop|Freefall")
	float FreefallMaxSpeed = 6400.f; // 수직자세
	
	UPROPERTY(EditDefaultsOnly, Category = "Drop|Freefall")
	float FreefallAccel = 2.5f; // 속도
	
	UPROPERTY(EditDefaultsOnly, Category = "Drop|Freefall")
	float AutoDeployHeight = 12000.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Drop|Parachute")
	float ParachuteDescentSpeed = 600.f; // 하강
	
	UPROPERTY(EditDefaultsOnly, Category = "Drop|Parachute")
	float ParachuteForwardSpeed = 1800.f; // 전진
	
	UPROPERTY(EditDefaultsOnly, Category = "Drop|Parachute")
	float LandHeight = 80.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float DefaultArmLength = 400.f;

	// --- Input handlers ------------------------------------------------
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
};
