#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Core/Enums/DropTypes.h"
#include "DropCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UStaticMeshComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
class AAirPlane;
class USceneComponent;
class UserWidget;

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
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drop|Parachute", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> ParachuteMesh;

	/** Follow camera on the end of the boom. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;
	
	UPROPERTY(EditDefaultsOnly, Category = "Camera|InPlane")
	float InPlaneArmLength = 3500.f;
	UPROPERTY(EditDefaultsOnly, Category = "Camera|InPlane")
	FVector InPlaneSocketOffset = FVector(0.f, 500.f, 350.f);
	UPROPERTY(EditDefaultsOnly, Category = "Camera|InPlane")
	float InPlaneCameraPitch = -22.f;
	UPROPERTY(EditDefaultsOnly, Category = "Camera|InPlane")
	float InPlaneYawOffset = -30.f;

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
	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> ParachuteAction;
	
	/*
	현재 탑승 중인 비행기 
	*/
	UPROPERTY()
	TObjectPtr<AAirPlane> BoardedPlane;
	
	/*
	낙하산
	*/
	void OnJumpPressed();
	void OnParachutePressed();
	
	UPROPERTY(EditDefaultsOnly, Category = "Drop|Freefall")
	float FreefallMinSpeed = 3600.f; // 슈가글라이더 자세
	
	UPROPERTY(EditDefaultsOnly, Category = "Drop|Freefall")
	float FreefallMaxSpeed = 6400.f; // 수직자세
	
	UPROPERTY(EditDefaultsOnly, Category = "Drop|Freefall")
	float FreefallAccel = 2.5f; // 속도
	
	UPROPERTY(EditDefaultsOnly, Category = "Drop|Freefall")
	float AutoDeployHeight = 1000.f; // 자유낙하 중에 플레이어가 F를 안 눌러도 자동으로 낙하산이 펴지는 고도
	
	UPROPERTY(EditDefaultsOnly, Category = "Drop|Parachute")
	float ParachuteDescentSpeed = 600.f; // 하강
	
	UPROPERTY(EditDefaultsOnly, Category = "Drop|Parachute")
	float ParachuteForwardSpeed = 1800.f; // 전진
	
	UPROPERTY(EditDefaultsOnly, Category = "Drop|Parachute")
	float LandHeight = 80.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Drop|Parachute")
	TSubclassOf<UUserWidget> ParachutePromptWidgetClass;
	
	// --- 낙하산 캐노피 연출 --------------------------------------------
	UPROPERTY(EditDefaultsOnly, Category = "Drop|Parachute")
	FName ParachuteAttachSocket = TEXT("spine_05");
	
	UPROPERTY(EditDefaultsOnly, Category = "Drop|Parachute")
	FVector ParachuteRelativeLocation = FVector(-20.f, 0.f, 40.f);
	
	UPROPERTY(EditDefaultsOnly, Category = "Drop|Parachute")
	FRotator ParachuteRelativeRotation = FRotator(0.f, 0.f, -90.f);
	
	UPROPERTY(EditDefaultsOnly, Category = "Drop|Parachute")
	float ParachuteOpenScale = 60.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Drop|Parachute")
	float ParachuteDeployTime = 1.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Drop|Parachute")
	float ParachuteSwayAngle = 4.f;
	UPROPERTY(EditDefaultsOnly, Category = "Drop|Parachute")
	float ParachuteSwaySpeed = 1.5f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Drop|Parachute")
	float ParachuteLeanScale = 12.f;
	UPROPERTY(EditDefaultsOnly, Category = "Drop|Parachute")
	float ParachuteMaxLean = 18.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Drop|Parachute")
	float ParachuteTurnLeanScale = 0.05f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float DefaultArmLength = 400.f;

	// --- Input handlers ------------------------------------------------
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	
	// --- Parachute prompt widget ----------------------------------------
	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> ParachutePromptWidget;
	void ShowParachutePrompt();
	void HideParachutePrompt();
	
	// --- 낙하산 캐노피 상태 ------------------------------------------
	float ParachuteDeployElapsed = -1.f;
	float LastYawForLean = 0.f;
	void ShowParachute();
	void HideParachute();
	void UpdateParachuteVisual(float Dt);
	
};
