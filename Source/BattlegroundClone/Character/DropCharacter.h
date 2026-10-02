#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Combat/HealthComponent.h"
#include "Core/Enums/DropTypes.h"
#include "Core/Enums/EFireMode.h"
#include "Weapon/WeaponUserInterface.h"
#include "Interaction/InteractionComponent.h"

#include "DropCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UStaticMeshComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
class AAirPlane;
class AWeaponBase;
class USceneComponent;
class UserWidget;
class UHealthComponent;
class UWeaponInventoryComponent;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UImage;
class AItemPickupActor;
class UBagComponent;

UCLASS()
class BATTLEGROUNDCLONE_API ADropCharacter : public ACharacter, public IWeaponUserInterface
{
	GENERATED_BODY()

public:
	ADropCharacter();

	// --- IWeaponUserInterface : 무기가 캐릭터(카메라/애니메이션 제어권 보유자)에게 알리는 콜백 ---
	virtual void ReceiveWeaponRecoil_Implementation(float Pitch, float YawRange, float RecoverySpeed) override;
	virtual void NotifyWeaponFired_Implementation() override;
	virtual void NotifyWeaponReloadStarted_Implementation(float Duration) override;
	virtual void RequestMeleeAttack_Implementation(UAnimMontage* AttackMontage) override;
	virtual void NotifyAmmoEmpty_Implementation() override;

	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override; /* RPC Server */
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;
	
	/** Current phase of the jump -> freefall -> parachute flow. Drives movement params + AnimBP. */
	UPROPERTY(ReplicatedUsing = OnRep_DropState, BlueprintReadOnly, Category = "Drop")
	EDropState DropState = EDropState::Ground;
	EDropState PrevDropState = EDropState::Ground;

	void ApplyDropState(EDropState OldState, EDropState NewState);
	UFUNCTION() void OnRep_DropState();

	UFUNCTION(Server, Reliable) void ServerBeginFreefall();
	UFUNCTION(Server, Reliable) void ServerDeployParachute();
	
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
	
	/*
	포복
	*/
	UFUNCTION(BlueprintPure, Category = "Movement")
	bool IsProne() const
	{
		return bIsCrouched;
	}
	
	/*
	반동
	*/
	void AddRecoil(float Pitch, float YawRange, float RecoverySpeed);
	void UpdateRecoilRecovery(float DeltaTime);

	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

	UFUNCTION(BlueprintPure, Category="Combat")
	UHealthComponent* GetHealthComp() const
	{
		return HealthComp;
	}

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UWeaponInventoryComponent> WeaponInventory;

	UFUNCTION(BlueprintPure, Category = "Weapon")
	UWeaponInventoryComponent* GetWeaponInventory() const { return WeaponInventory; }
	
	UFUNCTION(BlueprintPure, Category = "Weapon")
	AWeaponBase* GetEquippedWeapon() const;

	UFUNCTION(BlueprintPure, Category = "Weapon")
	EFireMode GetCurrentFireMode() const;
	
	void MeleeAttack(UAnimMontage* AttackMontage);
	
	UFUNCTION(Server, Reliable)
	void ServerMeleeAttack(UAnimMontage* AttackMontage);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayMeleeMontage(UAnimMontage* AttackMontage);

	UFUNCTION()
	void HandleDeath(AController* Killer, AActor* DamageCauser);
	UFUNCTION()
	void HandleOwnDeath(AController* Killer, AActor* DamageCauser);
	void ForceStopAim();
	
	UFUNCTION()
	void HandleHit(AController* InstigatorController, AActor* DamageCauser, FVector ShotDirection);

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_HitReact(FVector ShotDirection);

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	TArray<UAnimMontage*> HitReactMontages; // 0=Front,1=Back,2=Left,3=Right

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float KnockbackPower = 600.f;

	int32 GetHitDirectionIndex(const FVector& ShotDirection) const;

	
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_Die();
	
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	TArray<UAnimMontage*> DeathMontages;
	bool bIsDead = false;
	
	/*
	Death and Destroy()
	*/
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float DeathDestroyDelay = 5.f;

	FTimerHandle DeathDestroyTimerHandle;

	void DestroySelf();
	
	/*
	 DEBUG
	*/
	UFUNCTION(Exec)
	void DbgHurt(float Amt = 20.f);
	
	UFUNCTION(Server, Reliable)
	void Server_DbgHurt(float Amt);

	// --- 인벤토리 캐릭터 프리뷰 (로컬 전용, 컨트롤러가 호출) ----------------

	void StartInventoryPreview(UImage* TargetImage);
	void StopInventoryPreview();
	
	// --- 줍기 / 가방 ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UBagComponent> BagComp;

	// --- 인터랙션/줍기/가방/소모품 사용 (UInteractionComponent로 위임) ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UInteractionComponent> InteractionComp;

	bool TryPickup(AItemPickupActor* Pickup) { return InteractionComp && InteractionComp->TryPickup(Pickup); }   // 서버 전용

protected:
	virtual void BeginPlay() override;
	virtual void PawnClientRestart() override;

	// --- Components -------------------------------------------------------
	/** Third person camera boom. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drop|Parachute", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> ParachuteMesh;

	/** Follow camera on the end of the boom. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;

	// 스코프 렌즈용 별도 카메라. 좁은 FOV로 렌더타겟에 찍어서 원형 UI에 씌움 (메인 카메라는 줌 안 함)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|Scope", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneCaptureComponent2D> ScopeCapture;

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
	
	void OnPronePressed(const FInputActionValue& Value);
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float ProneSpeed = 120.f;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float ProneCapsuleHalfHeight = 40.f;
	

	UPROPERTY(ReplicatedUsing = OnRep_AimMode, BlueprintReadOnly, Category="Combat")
	EDropAimMode AimMode = EDropAimMode::Hip;
	EDropAimMode PrevAimMode = EDropAimMode::Hip;
	
	void ApplyAimVisuals(EDropAimMode OldMode, EDropAimMode NewMode);
	
	UFUNCTION() 
	void OnRep_AimMode();
	
	UFUNCTION(Server, Reliable) 
	void ServerSetAimMode(EDropAimMode NewMode);
	
	// HP
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<class UHealthComponent> HealthComp;
	
	UPROPERTY(BlueprintReadOnly, Category="Combat")
	bool bIsAiming = false;
	
	// Aim: 조준
	// Tap: 살짝/짧게 누르기 (탭)
	// Threshold: 임계값 / 기준치
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float AimTapThreshold = 0.18f;
	
	float AimPressTime = 0.f;
	
	void OnAimPressed();
	void OnAimReleased();
	void SetAimMode(EDropAimMode NewMode);
	void UpdateAimCamera(float Dt);
	
	void StartFire();
	void StopFire();
	void OnReloadPressed();

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TArray<TObjectPtr<UInputAction>> WeaponSlotActions;

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
	float FreefallMinSpeed = 6000.f; // 슈가글라이더 자세
	
	UPROPERTY(EditDefaultsOnly, Category = "Drop|Freefall")
	float FreefallMaxSpeed = 8000.f; // 수직자세
	
	UPROPERTY(EditDefaultsOnly, Category = "Drop|Freefall")
	float FreefallAccel = 2.5f; // 속도
	
	UPROPERTY(EditDefaultsOnly, Category = "Drop|Freefall")
	float AutoDeployHeight = 8000.f; // 자유낙하 중에 플레이어가 F를 안 눌러도 자동으로 낙하산이 펴지는 고도
	
	UPROPERTY(EditDefaultsOnly, Category = "Drop|Parachute")
	float ParachuteDescentSpeed = 600.f; // 하강
	
	UPROPERTY(EditDefaultsOnly, Category = "Drop|Parachute")
	float ParachuteForwardSpeed = 1800.f; // 전진
	
	UPROPERTY(EditDefaultsOnly, Category = "Drop|Parachute")
	float LandHeight = 80.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Drop|Parachute")
	TSubclassOf<UUserWidget> ParachutePromptWidgetClass;
	
	// --- 수직 낙하 --------------------------------------------
	UPROPERTY(BlueprintReadOnly, Category = "Drop")
	bool bIsFastFalling = false;

	void OnFastFallPressed();
	void OnFastFallReleased();
	
	// --- 낙하산 캐노피 연출 --------------------------------------------
	UPROPERTY(EditDefaultsOnly, Category = "Drop|Parachute")
	FName ParachuteAttachSocket = TEXT("spine_05");
	
	UPROPERTY(EditDefaultsOnly, Category = "Drop|Parachute")
	FVector ParachuteRelativeLocation = FVector(0.f, 0.f, 50.f);

	// Blender에서 정렬하면 추가로 건드리기x
	UPROPERTY(EditDefaultsOnly, Category = "Drop|Parachute")
	FRotator ParachuteRelativeRotation = FRotator(0.f, 90.f, 0.f);

	// SM_Parachute는 Blender에서 실측(~9m) 크기로 맞춰둠 → 1 기준. 크면 0.8, 작으면 1.3
	UPROPERTY(EditDefaultsOnly, Category = "Drop|Parachute")
	float ParachuteOpenScale = 1.f;
	
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

	// 자유낙하 / 낙하산 중 카메라를 뒤로 빼서 캐노피까지 화면에 담기게
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float DescentArmLength = 1100.f;
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	FVector DescentSocketOffset = FVector(0.f, 0.f, 120.f);
	
	// Aim
	UPROPERTY(EditDefaultsOnly, Category = "Camera|Aim")
	float HipArmLength = 400.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Camera|Aim")
	FVector HipSocketOffset = FVector::ZeroVector;
	
	UPROPERTY(EditDefaultsOnly, Category = "Camera|Aim")
	float HipFOV = 90.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Camera|Aim")
	float ShoulderArmLength = 220.f;   // 배그 TPP 견착: 상반신~허리까지 보이게 뒤로 뺌

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Aim")
	FVector ShoulderSocketOffset = FVector(0.f, 45.f, 55.f);

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Aim")
	float ShoulderFOV = 72.f;
	
	// Scoped
	UPROPERTY(EditDefaultsOnly, Category = "Camera|Aim")
	float ScopedArmLength = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Aim")
	FVector ScopedSocketOffset = FVector(10.f, 0.f, 50.f);   // 붐 피벗(캡슐중심) 기준 눈 위치

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Aim")
	float ScopedFOV = 45.f;

	// AWP 전용 하드코딩 : BP 디폴트에서 PIE 보며 튜닝
	UPROPERTY(EditAnywhere, Category = "Camera|Aim")
	FVector ScopedWeaponOffset = FVector(40.f, 0.f, -18.4f);   // 카메라 기준 (앞, 오른쪽, 아래)

	UPROPERTY(EditAnywhere, Category = "Camera|Aim")
	FRotator ScopedWeaponRotation = FRotator(0.f, -90.f, 0.f); // 총열을 시야 방향으로 (WeaponMesh Yaw90 상쇄)

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Aim")
	float AimInterpSpeed = 12.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Camera|Aim")
	float ScopeTransitionTime = 0.18f;

	UPROPERTY(EditAnywhere, Category = "Camera|Aim")
	FVector ScopedRaiseOffset = FVector(-6.f, 4.f, -16.f);

	UPROPERTY(EditAnywhere, Category = "Camera|Aim")
	FRotator ScopedRaiseRotation = FRotator(-14.f, 4.f, 10.f);

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Aim")
	float ScopeSpringOvershoot = 1.2f;

	TWeakObjectPtr<AWeaponBase> ScopedWeapon;
	float ScopeAlpha = 0.f;
	float ScopeFromArm = 0.f;
	FVector ScopeFromOffset = FVector::ZeroVector;
	float ScopeFromFOV = 90.f;

	void BeginScopedWeapon(AWeaponBase* Weapon);
	void EndScopedWeapon();
	FTransform GetScopedWeaponTransform(const AWeaponBase* Weapon) const;
	
	UPROPERTY(EditDefaultsOnly, Category = "Camera|Aim")
	TSubclassOf<UUserWidget> ScopeOverlayClass;

	// --- Input handlers ------------------------------------------------
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	
	// --- Parachute prompt widget ----------------------------------------
	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> ParachutePromptWidget;
	void ShowParachutePrompt();
	void HideParachutePrompt();
	
	// --- Scope (Shoulder 크로스헤어) ----------------------------------------
	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> ScopeOverlayWidget;
	void ShowScopeOverlay();
	void HideScopeOverlay();

	// --- 저격 스코프 렌즈 (Scoped, SceneCapture 렌더타겟) --------------------
	UPROPERTY(EditDefaultsOnly, Category = "Camera|Scope")
	TSubclassOf<UUserWidget> SniperScopeOverlayClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "Camera|Scope")
	TObjectPtr<UMaterialInterface> ScopeLensMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Scope")
	int32 ScopeRenderTargetSize = 1024;

	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> ScopeRenderTarget;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ScopeLensMID;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> SniperScopeOverlayWidget;

	void ShowSniperScope();
	void HideSniperScope();

	// --- 인벤토리 캐릭터 프리뷰 (SceneCapture 렌더타겟) ---------------------

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|Preview", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneCaptureComponent2D> PreviewCapture;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Preview")
	TObjectPtr<UMaterialInterface> PreviewMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Preview")
	FVector PreviewCaptureOffset = FVector(320.f, 0.f, 5.f); // 캡슐 기준 (앞, 오른쪽, 위)

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Preview")
	float PreviewFOV = 18.f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Preview")
	FIntPoint PreviewRenderTargetSize = FIntPoint(512, 1024);

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Preview")
	float PreviewBrightness = 1.f; 

	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> PreviewRenderTarget;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> PreviewMID;

	void RefreshPreviewShowList();
	
	// --- 낙하산 캐노피 상태 ------------------------------------------
	
	float ParachuteDeployElapsed = -1.f;
	float LastYawForLean = 0.f;
	void ShowParachute();
	void HideParachute();
	void UpdateParachuteVisual(float Dt);
	
private:
	
	float RecoilAccumPitch = 0.f;
	float RecoilRecoverySpeed = 0.f;
	float LastRecoilTime = -1.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Recoil")
	float RecoilRecoveryDelay = 0.15f;
};
