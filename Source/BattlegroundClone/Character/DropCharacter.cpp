#include "Character/DropCharacter.h"

#include "Combat/HealthComponent.h"
#include "Weapon/WeaponBase.h"
#include "Weapon/WeaponInventoryComponent.h"
#include "Weapon/WeaponUserInterface.h"
#include "Drop/AirPlane.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Controller.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"

#include "Combat/ProjectileBullet.h"
#include "Core/DropPlayerController.h"
#include "Core/DropPlayerState.h"

#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Blueprint/UserWidget.h"
#include "Inventory/BagComponent.h"
#include "Input/InputBindingComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputActionValue.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Engine/DamageEvents.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/Image.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

ADropCharacter::ADropCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Don't rotate the mesh with the controller; the movement component turns it toward motion.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0.f, 500.f, 0.f);
	Move->JumpZVelocity = 500.f;
	Move->AirControl = 0.35f;
	Move->MaxWalkSpeed = 500.f;
	Move->BrakingDecelerationWalking = 2000.f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.f;
	CameraBoom->bUsePawnControlRotation = true; // boom follows controller (mouse) rotation

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false; // camera is fixed on the boom

	ScopeCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("ScopeCapture"));
	ScopeCapture->SetupAttachment(FollowCamera);
	ScopeCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	ScopeCapture->bCaptureEveryFrame = false; // 스코프 켤 때만 캡처 (성능)
	ScopeCapture->bCaptureOnMovement = false;
	ScopeCapture->FOVAngle = 8.f;
	ScopeCapture->SetActive(false);

	PreviewCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("PreviewCapture"));
	PreviewCapture->SetupAttachment(RootComponent);
	PreviewCapture->CaptureSource = ESceneCaptureSource::SCS_SceneColorHDR;
	PreviewCapture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
	PreviewCapture->bCaptureEveryFrame = false; // 인벤토리 열 때만 캡처 (성능)
	PreviewCapture->bCaptureOnMovement = false;
	PreviewCapture->ShowFlags.SetAtmosphere(false);
	PreviewCapture->ShowFlags.SetFog(false);
	PreviewCapture->SetActive(false);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> PreviewMaterialAsset(
		TEXT("/Script/Engine.Material'/Game/UI/M_CharacterPreview.M_CharacterPreview'"));
	if (PreviewMaterialAsset.Succeeded())
	{
		PreviewMaterial = PreviewMaterialAsset.Object;
	}

	ParachuteMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ParachuteMesh"));
	ParachuteMesh->SetupAttachment(GetCapsuleComponent());
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ParachuteMeshAsset(
		TEXT("/Script/Engine.StaticMesh'/Game/Fab/Parachute/parachute.parachute'"));
	if (ParachuteMeshAsset.Succeeded())
	{
		ParachuteMesh->SetStaticMesh(ParachuteMeshAsset.Object);
	}
	ParachuteMesh->SetRelativeLocation(ParachuteRelativeLocation);
	ParachuteMesh->SetRelativeRotation(ParachuteRelativeRotation);
	ParachuteMesh->SetRelativeScale3D(FVector(0.01f));           // 접힘
	ParachuteMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ParachuteMesh->SetCollisionProfileName(TEXT("NoCollision"));
	ParachuteMesh->SetGenerateOverlapEvents(false);
	ParachuteMesh->SetVisibility(false);
	ParachuteMesh->SetHiddenInGame(true);

	HealthComp = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComp"));
	HealthComp->OnDeath.AddDynamic(this, &ADropCharacter::HandleDeath);
	HealthComp->OnDeath.AddDynamic(this, &ADropCharacter::HandleOwnDeath);
	HealthComp->OnHit.AddDynamic(this, &ADropCharacter::HandleHit);

	WeaponInventory = CreateDefaultSubobject<UWeaponInventoryComponent>(TEXT("WeaponInventory"));
	BagComp = CreateDefaultSubobject<UBagComponent>(TEXT("BagComp"));
	InteractionComp = CreateDefaultSubobject<UInteractionComponent>(TEXT("InteractionComp"));
	InputBindingComp = CreateDefaultSubobject<UInputBindingComponent>(TEXT("InputBindingComp"));
}

/*
엔진 오버라이드
*/
void ADropCharacter::BeginPlay()
{
	Super::BeginPlay();

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->GetNavAgentPropertiesRef().bCanCrouch = true;
	Move->SetCrouchedHalfHeight(ProneCapsuleHalfHeight);
	Move->MaxWalkSpeedCrouched = ProneSpeed;

	if (WeaponInventory)
	{
		WeaponInventory->InitialEquip();
	}
}

void ADropCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (PC->IsLocalController())
		{
			PC->SetInputMode(FInputModeGameOnly());
			PC->SetShowMouseCursor(false);
		}
		if (InputBindingComp)
		{
			InputBindingComp->AddMappingContext(PC);
		}
	}
}

void ADropCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 인벤토리 열린 동안 무기 변경 즉시 반영
	if (PreviewCapture && PreviewCapture->bCaptureEveryFrame)
	{
		RefreshPreviewShowList();
	}

	switch (DropState)
	{
	case EDropState::Freefall:
		if (HasAuthority() || IsLocallyControlled()) UpdateFreefall(DeltaTime);
		if (HasAuthority() && GroundDistance() <= AutoDeployHeight) DeployParachute();
		break;
	case EDropState::Parachuting:
		if (HasAuthority() || IsLocallyControlled()) UpdateParachute(DeltaTime);
		if (HasAuthority() && GroundDistance() <=LandHeight) SetDropState(EDropState::Ground);
		break;
	case EDropState::Ground:
		if (IsLocallyControlled() || HasAuthority()) UpdateAimCamera(DeltaTime);
		break;
	default:
		break;
	}
	if (IsLocallyControlled())
	{
		UpdateRecoilRecovery(DeltaTime);
	}
	if (ParachuteDeployElapsed >= 0.f)
	{
		UpdateParachuteVisual(DeltaTime);
	}
}

void ADropCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (InputBindingComp)
	{
		InputBindingComp->SetupInput(PlayerInputComponent);
	}
}

void ADropCharacter::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(ADropCharacter, AimMode, COND_SkipOwner);
	DOREPLIFETIME(ADropCharacter, DropState);
}

float ADropCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	FVector ShotDirection = GetActorForwardVector();
	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		const FPointDamageEvent& PointDamageEvent = static_cast<const FPointDamageEvent&>(DamageEvent);
		ShotDirection = PointDamageEvent.ShotDirection;
	}
	if (HealthComp)
	{
		HealthComp -> ApplyDamage(DamageAmount, EventInstigator, DamageCauser, ShotDirection);
	}
	return Applied;
}

/*
IWeaponUserInterface
*/
void ADropCharacter::ReceiveWeaponRecoil_Implementation(float Pitch, float YawRange, float RecoverySpeed)
{
	AddRecoil(Pitch, YawRange, RecoverySpeed);
}
void ADropCharacter::NotifyWeaponFired_Implementation()
{
	if (WeaponInventory) WeaponInventory->PlayFireMontage();
}
void ADropCharacter::NotifyWeaponReloadStarted_Implementation(float Duration)
{
	if (WeaponInventory) WeaponInventory->HandleReloadStarted(Duration);
}
void ADropCharacter::RequestMeleeAttack_Implementation(UAnimMontage* AttackMontage)
{
	MeleeAttack(AttackMontage);
}
void ADropCharacter::NotifyAmmoEmpty_Implementation()
{
	if (ADropPlayerController* PC = Cast<ADropPlayerController>(GetController()))
		PC->ShowCenterNotification(FText::GetEmpty(), FText::FromString(TEXT("탄약 없음")), FLinearColor(1.f, 0.3f, 0.1f));
}

/*
입력 - 이동/점프
*/
void ADropCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	if (!Controller || Axis.IsNearlyZero())
	{
		return;
	}
	if (InteractionComp) InteractionComp->RequestCancelUseItem();

	const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
	const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(Forward, Axis.Y);
	AddMovementInput(Right, Axis.X);
}

void ADropCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(Axis.Y);

	const bool bIsPullingDown = (Axis.Y > 0.0f);
	const bool bHasAccumulatedRecoil = (RecoilAccumPitch > 0.0f);
	if (bIsPullingDown && bHasAccumulatedRecoil)
	{
		RecoilAccumPitch = FMath::Max(0.f, RecoilAccumPitch - Axis.Y);
	}
}

void ADropCharacter::OnJumpPressed()
{
	if (InteractionComp) InteractionComp->RequestCancelUseItem();
	Jump();
}

/*
포복
*/
void ADropCharacter::OnPronePressed(const FInputActionValue& Value)
{
	if (!CanActOnGround() || (InteractionComp && InteractionComp->IsUsingItem()))
	{
		return;
	}
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

void ADropCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	if (WeaponInventory)
	{
		WeaponInventory->RefreshWeaponAttach();
	}
}

void ADropCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	if (WeaponInventory)
	{
		WeaponInventory->RefreshWeaponAttach();
	}
}

/*
반동
*/
void ADropCharacter::AddRecoil(float Pitch, float YawRange, float RecoverySpeed)
{
	if (!IsLocallyControlled())
	{
		return;
	}
	AddControllerPitchInput(-Pitch);
	AddControllerYawInput(FMath::RandRange(-YawRange, YawRange));
	RecoilAccumPitch += Pitch;
	RecoilRecoverySpeed = RecoverySpeed;
	LastRecoilTime = GetWorld()->GetTimeSeconds();
}

void ADropCharacter::UpdateRecoilRecovery(float DeltaTime)
{
	if (RecoilAccumPitch <= 0.f)
	{
		return;
	}
	if (GetWorld()->GetTimeSeconds() - LastRecoilTime < RecoilRecoveryDelay)
	{
		return;
	}
	const float NewAccum = FMath::FInterpTo(RecoilAccumPitch, 0.f, DeltaTime, RecoilRecoverySpeed);
	AddControllerPitchInput(RecoilAccumPitch - NewAccum);
	RecoilAccumPitch = NewAccum < 0.01f ? 0.f : NewAccum;
}

/*
Drop 상태 머신
*/
void ADropCharacter::SetDropState(EDropState NewState)
{
	if (DropState == NewState)
	{
		return;
	}
	const EDropState Old = DropState;
	DropState = NewState; // ★ 서버에서만 호출됨 → 복제 → 클라 OnRep_DropState

	if (NewState != EDropState::Ground && AimMode != EDropAimMode::Hip)
	{
		SetAimMode(EDropAimMode::Hip);
		EndScopedWeapon();
	}
	ApplyDropState(Old, NewState);
	PrevDropState = NewState;
}

void ADropCharacter::ApplyDropState(EDropState OldState, EDropState NewState)
{
	if (IsLocallyControlled())
	{
		if (ADropPlayerController* PC = Cast<ADropPlayerController>(GetController()))
		{
			bool bIsOnGround = (NewState == EDropState::Ground);
			PC->SetGameplayHUDVisible(bIsOnGround);
		}
	}

	if (UCharacterMovementComponent* M = GetCharacterMovement())
	{
		switch (NewState)
		{
		case EDropState::InPlane:
			M->DisableMovement();
			break;

		case EDropState::Freefall:
		case EDropState::Parachuting:
			M->SetMovementMode(MOVE_Flying);   // 속도 직접 제어, 중력/지면스냅 없음
			M->GravityScale = 0.f;
			M->AirControl = 1.f;
			M->bOrientRotationToMovement = false;
			bUseControllerRotationYaw = true;
			break;

		case EDropState::Ground:
		default:
			M->GravityScale = 1.f;
			M->AirControl = 0.35f;
			M->bOrientRotationToMovement = true;
			bUseControllerRotationYaw = false;
			M->SetMovementMode(MOVE_Walking);
			break;
		}
	}

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetVisibility(NewState != EDropState::InPlane);
	}

	if (IsLocallyControlled() && CameraBoom)
	{
		switch (NewState)
		{
		case EDropState::InPlane:
			CameraBoom->TargetArmLength  = InPlaneArmLength;
			CameraBoom->SocketOffset     = InPlaneSocketOffset;
			CameraBoom->bDoCollisionTest = false;
			break;
		case EDropState::Freefall:
		case EDropState::Parachuting:
			CameraBoom->TargetArmLength  = DescentArmLength;
			CameraBoom->SocketOffset     = DescentSocketOffset;
			CameraBoom->bDoCollisionTest = false;
			break;
		case EDropState::Ground:
			CameraBoom->TargetArmLength  = DefaultArmLength;
			CameraBoom->SocketOffset     = FVector::ZeroVector;
			CameraBoom->bDoCollisionTest = true;
			break;
		default:
			break;
		}
	}

	if (NewState == EDropState::Freefall)
	{
		ShowParachutePrompt();
	}
	else
	{
		HideParachutePrompt();
	}
	if (NewState == EDropState::Parachuting)
	{
		ShowParachute();
	}
	else if (OldState == EDropState::Parachuting)
	{
		HideParachute();
	}

	OnDropStateChanged(NewState, OldState);
}

void ADropCharacter::OnRep_DropState()
{
	ApplyDropState(PrevDropState, DropState);
	PrevDropState = DropState;
}

void ADropCharacter::EnterPlane(AAirPlane* Plane, USceneComponent* Seat)
{
	if (!HasAuthority()) return;
	if (!Plane || !Seat) return;

	BoardedPlane = Plane;
	AttachToComponent(Seat, FAttachmentTransformRules::SnapToTargetIncludingScale);
	SetDropState(EDropState::InPlane);

	if (AController* C = GetController())
	{
		C->SetControlRotation(FRotator(InPlaneCameraPitch, Plane->GetHeadingYaw() + InPlaneYawOffset, 0.f));
	}
}

/*
낙하산 - 자유낙하/전개
*/
void ADropCharacter::BeginFreefall()
{
	if (!HasAuthority())
	{
		return;
	}
	if (DropState != EDropState::InPlane) return;

	DetachFromActor(FDetachmentTransformRules(EDetachmentRule::KeepWorld, true));
	BoardedPlane = nullptr;

	SetDropState(EDropState::Freefall);

	if (UCharacterMovementComponent* M = GetCharacterMovement())
	{
		FVector Dir = GetControlRotation().Vector();
		Dir.Z = FMath::Min(Dir.Z, -0.4f);
		M->Velocity = Dir.GetSafeNormal() * FreefallMinSpeed;
	}
}

void ADropCharacter::UpdateFreefall(float Dt)
{
	float TargetSpeed = bIsFastFalling ? FreefallMaxSpeed : FreefallMinSpeed;

	UCharacterMovementComponent* M = GetCharacterMovement();
	if (!M) return;

	FVector Desired = GetControlRotation().Vector() * 800.f; // 조작감용 약간의 활강
	Desired.Z = -TargetSpeed;
	M->Velocity = FMath::VInterpTo(M->Velocity, Desired, Dt, FreefallAccel); // 바람저항
}

void ADropCharacter::DeployParachute()
{
	if (!HasAuthority())
	{
		return;
	}
	if (DropState != EDropState::Freefall)
	{
		return;
	}
	SetDropState(EDropState::Parachuting);
}

void ADropCharacter::UpdateParachute(float Dt)
{
	UCharacterMovementComponent* M = GetCharacterMovement();
	if (!M) return;

	/*
	낙하산 전개 상태: 일정한 하강 속도 + 바라보는 방향으로 전진
	*/
	const FRotator YawRot(0.f, GetControlRotation().Yaw, 0.f);
	const FVector Forward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);

	FVector Desired = Forward * ParachuteForwardSpeed;
	Desired.Z = -ParachuteDescentSpeed;

	M->Velocity = FMath::VInterpTo(M->Velocity, Desired, Dt, 2.f);
}

float ADropCharacter::GroundDistance() const
{
	const UWorld* World = GetWorld();
	if (!World) return TNumericLimits<float>::Max();

	const FVector Start = GetActorLocation();
	const FVector End = Start - FVector(0.f, 0.f, 200000.f);

	FHitResult Hit;
	FCollisionQueryParams P(SCENE_QUERY_STAT(DropGround), /*bTraceComplex=*/false, this);

	if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, P))
	{
		const float HalfHeight = GetCapsuleComponent()
			? GetCapsuleComponent()->GetScaledCapsuleHalfHeight()
			: 88.f;
		return FMath::Max(0.f, Hit.Distance - HalfHeight);
	}
	return TNumericLimits<float>::Max();
}

void ADropCharacter::OnFastFallPressed()
{
	if (DropState == EDropState::Freefall)
	{
		bIsFastFalling = true;
	}
}

void ADropCharacter::OnFastFallReleased()
{
	bIsFastFalling = false;
}

void ADropCharacter::OnParachutePressed()
{
	if (DropState == EDropState::InPlane)
	{
		ServerBeginFreefall();
	}
	else if (DropState == EDropState::Freefall)
	{
		ServerDeployParachute();
	}
}

void ADropCharacter::ServerBeginFreefall_Implementation()
{
	BeginFreefall();
}

void ADropCharacter::ServerDeployParachute_Implementation()
{
	DeployParachute();
}

/*
낙하산 - 비주얼
*/
void ADropCharacter::ShowParachute()
{
	if (!ParachuteMesh) return;
	ParachuteDeployElapsed = 0.f;
	LastYawForLean = GetActorRotation().Yaw;
	ParachuteMesh->SetRelativeLocation(ParachuteRelativeLocation);
	ParachuteMesh->SetRelativeRotation(ParachuteRelativeRotation);
	ParachuteMesh->SetRelativeScale3D(FVector(0.01f));
	ParachuteMesh->SetHiddenInGame(false, true);
	ParachuteMesh->SetVisibility(true, true);
}

void ADropCharacter::HideParachute()
{
	if (!ParachuteMesh) return;
	ParachuteDeployElapsed = -1.f;
	ParachuteMesh->SetVisibility(false, true);
	ParachuteMesh->SetHiddenInGame(true, true);
}

void ADropCharacter::UpdateParachuteVisual(float Dt)
{
	if (!ParachuteMesh) return;

	/*
	펼침 : 0 -> ParachuteOpenScale 로 ParachuteDeployTime 동안 ease-out
	*/
	ParachuteDeployElapsed += Dt;
	const float OpenAlpha = (ParachuteDeployTime > 0.f)
		? FMath::Clamp(ParachuteDeployElapsed / ParachuteDeployTime, 0.f, 1.f)
		:1.f;
	const float Eased = 1.f -FMath::Square(1.f - OpenAlpha);
	const float Scale = FMath::Max(ParachuteOpenScale*Eased, 0.01f);
	ParachuteMesh -> SetRelativeScale3D(FVector(Scale));

	// 단위 시간당 회전하는 각도의 크기
	const float Yaw = GetActorRotation().Yaw;
	const float YawRate = FMath::FindDeltaAngleDegrees(LastYawForLean, Yaw) // -180°~180° 경계 영역에서의 회전각 오차방지
		/ FMath::Max(Dt, KINDA_SMALL_NUMBER);
	LastYawForLean = Yaw;

	FRotator Rot = ParachuteRelativeRotation;
	const float SwayBlend = FMath::Clamp((OpenAlpha - 0.5f) * 2.f, 0.f, 1.f);
	if (SwayBlend > 0.f)
	{
		const float T = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
		// FMath::Sin(...) * ParachuteSwayAngle : 바람 흔들림
		Rot.Pitch += FMath::Sin(T*ParachuteSwaySpeed)*ParachuteSwayAngle*SwayBlend;
		Rot.Roll += FMath::Sin(T * ParachuteSwaySpeed * 0.73f + 1.3f)
			* ParachuteSwayAngle * 0.6f * SwayBlend;
		if (const UCharacterMovementComponent* M = GetCharacterMovement())
		{
			const FVector LocalVel =
				GetActorTransform().InverseTransformVectorNoScale(M->Velocity);
			const float Denom = FMath::Max(ParachuteForwardSpeed, 1.f);
			const float LeanPitch = FMath::Clamp(
				-LocalVel.X / Denom * ParachuteLeanScale, -ParachuteMaxLean, ParachuteMaxLean);
			const float LeanRoll = FMath::Clamp(
				 LocalVel.Y / Denom * ParachuteLeanScale, -ParachuteMaxLean, ParachuteMaxLean);
			Rot.Pitch += LeanPitch * SwayBlend;
			Rot.Roll  += LeanRoll  * SwayBlend;
		}
		Rot.Roll += FMath::Clamp(
			YawRate * ParachuteTurnLeanScale, -ParachuteMaxLean, ParachuteMaxLean) * SwayBlend;
	}
	ParachuteMesh->SetRelativeRotation(Rot);
}

void ADropCharacter::ShowParachutePrompt()
{
	if (!ParachutePromptWidgetClass || ParachutePromptWidget) return;
	if (!IsLocallyControlled()) return; // network
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		ParachutePromptWidget = CreateWidget<UUserWidget>(PC, ParachutePromptWidgetClass);
		if (ParachutePromptWidget)
		{
			ParachutePromptWidget->AddToViewport();
		}
	}
}

void ADropCharacter::HideParachutePrompt()
{
	if (!ParachutePromptWidget) return;
	ParachutePromptWidget->RemoveFromParent();
	ParachutePromptWidget = nullptr;
}

/*
Aim 상태
*/
void ADropCharacter::OnAimPressed()
{
	if (!CanActOnGround()) return;
	AimPressTime = GetWorld()->GetTimeSeconds();
	if (AimMode == EDropAimMode::Scoped) return; // 스코프 중 다시 누름
	SetAimMode(EDropAimMode::Shoulder);
}

// aim 해제
void ADropCharacter::OnAimReleased()
{
	const float Held = GetWorld()->GetTimeSeconds() - AimPressTime;
	if (Held <= AimTapThreshold) // 누르고 있던 시간 < AimTapThreshold : 살짝 누르고 뗀 경우
	{
		SetAimMode(AimMode == EDropAimMode::Scoped ? EDropAimMode::Hip : EDropAimMode::Scoped);
	}
	else if (AimMode == EDropAimMode::Shoulder)
	{
		SetAimMode(EDropAimMode::Hip);
	}
}

void ADropCharacter::SetAimMode(EDropAimMode NewMode)
{
	AWeaponBase* Weapon = WeaponInventory ? WeaponInventory->GetEquippedWeapon() :  nullptr;

	if ((!Weapon || Weapon->GetWeaponType() == EWeaponType::Melee) && NewMode != EDropAimMode::Hip)
	{
		NewMode = EDropAimMode::Hip;
	}

	// ★ 스코프 상태일때는 카메라의 YAW에 따라 캐릭터도 움직여야함 ★
	if (AimMode == NewMode) return;
	const EDropAimMode OldMode = AimMode;
	AimMode = NewMode;
	bIsAiming = (AimMode != EDropAimMode::Hip);

	ApplyAimVisuals(OldMode, NewMode);
	if (!HasAuthority())
	{
		ServerSetAimMode(NewMode);
	}
}

void ADropCharacter::OnRep_AimMode()
{
	const EDropAimMode OldMode = PrevAimMode;
	bIsAiming = (AimMode != EDropAimMode::Hip);
	ApplyAimVisuals(OldMode, AimMode);
	PrevAimMode = AimMode;
}

void ADropCharacter::ServerSetAimMode_Implementation(EDropAimMode NewMode)
{
	if (AimMode == NewMode) return;
	const EDropAimMode OldMode = AimMode;
	AimMode   = NewMode;
	bIsAiming = (AimMode != EDropAimMode::Hip);
	ApplyAimVisuals(OldMode, NewMode);
}

void ADropCharacter::ApplyAimVisuals(EDropAimMode OldMode, EDropAimMode NewMode)
{
	// 1인칭 스코프 (Shoulder = 크로스헤어, Scoped = 저격 스코프 렌즈. 렌즈는 AWP 전용)
	if (NewMode == EDropAimMode::Shoulder)
	{
		ShowScopeOverlay();
	}
	else if (OldMode == EDropAimMode::Shoulder)
	{
		HideScopeOverlay();
	}

	AWeaponBase* Weapon = WeaponInventory ? WeaponInventory->GetEquippedWeapon() : nullptr;
	if (IsLocallyControlled() && Weapon && NewMode == EDropAimMode::Scoped)
	{
		BeginScopedWeapon(Weapon);
	}

	if (UCharacterMovementComponent* M = GetCharacterMovement())
	{
		M->bOrientRotationToMovement = !bIsAiming;
	}
	bUseControllerRotationYaw = bIsAiming;
	if (!bIsAiming && (HasAuthority() || IsLocallyControlled()))
	{
		StopFire();
	}
}

void ADropCharacter::UpdateAimCamera(float Dt)
{
	if (!CameraBoom || !FollowCamera) return;

	float   TargetArm    = HipArmLength;
	FVector TargetOffset = HipSocketOffset;
	float   TargetFOV    = HipFOV;

	if (AimMode == EDropAimMode::Shoulder)
	{
		TargetArm = ShoulderArmLength; TargetOffset = ShoulderSocketOffset; TargetFOV = ShoulderFOV;
	}
	else if (AimMode == EDropAimMode::Scoped)
	{
		TargetArm = ScopedArmLength; TargetOffset = ScopedSocketOffset; TargetFOV = ShoulderFOV;
	}

	AWeaponBase* Weapon = ScopedWeapon.Get();
	if (Weapon && WeaponInventory && WeaponInventory->GetEquippedWeapon() != Weapon)
	{
		EndScopedWeapon();
		Weapon = nullptr;
	}

	const bool bSniper = Weapon && Weapon->UsesScopedLens();
	if (!bSniper)
	{
		CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, TargetArm, Dt, AimInterpSpeed);
		CameraBoom->SocketOffset    = FMath::VInterpTo(CameraBoom->SocketOffset, TargetOffset, Dt, AimInterpSpeed);
		FollowCamera->SetFieldOfView(FMath::FInterpTo(FollowCamera->FieldOfView, TargetFOV, Dt, AimInterpSpeed));
		if (Weapon)
		{
			if (AimMode != EDropAimMode::Scoped)
			{
				EndScopedWeapon();
				return;
			}
			const FTransform Aimed = GetScopedWeaponTransform(Weapon);
			Weapon->SetActorRelativeLocation(Aimed.GetLocation());
			Weapon->SetActorRelativeRotation(Aimed.GetRotation());
		}
		return;
	}

	const bool bScoping = (AimMode == EDropAimMode::Scoped);
	ScopeAlpha = FMath::FInterpConstantTo(ScopeAlpha, bScoping ? 1.f : 0.f, Dt, 1.f / FMath::Max(ScopeTransitionTime, 0.01f));

	if (!bScoping)
	{
		ScopeFromArm = TargetArm;
		ScopeFromOffset = TargetOffset;
		ScopeFromFOV = TargetFOV;
		if (ScopeAlpha <= 0.f)
		{
			EndScopedWeapon();
			return;
		}
	}

	const float CamAlpha = FMath::InterpEaseInOut(0.f, 1.f, ScopeAlpha, 2.f);
	CameraBoom->TargetArmLength = FMath::Lerp(ScopeFromArm, ScopedArmLength, CamAlpha);
	CameraBoom->SocketOffset    = FMath::Lerp(ScopeFromOffset, ScopedSocketOffset, CamAlpha);
	FollowCamera->SetFieldOfView(FMath::Lerp(ScopeFromFOV, ScopedFOV, CamAlpha));

	const float T = ScopeAlpha - 1.f;
	const float Spring = 1.f + (ScopeSpringOvershoot + 1.f) * T * T * T + ScopeSpringOvershoot * T * T;
	const FTransform Aimed = GetScopedWeaponTransform(Weapon);
	Weapon->SetActorRelativeLocation(Aimed.GetLocation() + ScopedRaiseOffset * (1.f - Spring));
	Weapon->SetActorRelativeRotation((ScopedRaiseRotation * (1.f - Spring)).Quaternion() * Aimed.GetRotation());
}

/*
무기 발사/재장전 입력
*/
void ADropCharacter::StartFire()
{
	if (!CanAct())
	{
		return;
	}
	if (InteractionComp) InteractionComp->RequestCancelUseItem();
	if (DropState != EDropState::Ground)
	{
		return;
	}
	AWeaponBase* Weapon = GetEquippedWeapon();
	const bool bIsMelee = Weapon && Weapon->GetWeaponType() == EWeaponType::Melee;
	if (!bIsMelee && !bIsAiming)
	{
		return;
	}

	if (WeaponInventory)
	{
		WeaponInventory->StartFire();
	}
}

void ADropCharacter::StopFire()
{
	if (WeaponInventory)
	{
		WeaponInventory->StopFire();
	}
}

void ADropCharacter::OnReloadPressed()
{
	if (!CanAct())
	{
		return;
	}
	if (WeaponInventory)
	{
		WeaponInventory->OnReloadPressed();
	}
}

/*
스코프 - 숄더 크로스헤어
*/
void ADropCharacter::ShowScopeOverlay()
{
	if (!IsLocallyControlled())
	{
		return;
	}
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (!ScopeOverlayClass) // 클래스가 지정되어있지 않았을때
		{
			return;
		}
		if (!ScopeOverlayWidget) // 위젯이 없을때
		{
			ScopeOverlayWidget = CreateWidget<UUserWidget>(PC, ScopeOverlayClass);
			if (ScopeOverlayWidget)
			{
				ScopeOverlayWidget ->AddToViewport();
			}
		}
		if (ScopeOverlayWidget) // 위젯이 있을때
		{
			ScopeOverlayWidget -> SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}
}

void ADropCharacter::HideScopeOverlay()
{
	if (ScopeOverlayWidget)
	{
		ScopeOverlayWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void ADropCharacter::BeginScopedWeapon(AWeaponBase* Weapon)
{
	if (!Weapon || ScopedWeapon == Weapon)
	{
		return;
	}
	ScopedWeapon = Weapon;
	ScopeAlpha = 0.f;
	ScopeFromArm = CameraBoom->TargetArmLength;
	ScopeFromOffset = CameraBoom->SocketOffset;
	ScopeFromFOV = FollowCamera->FieldOfView;
	Weapon->AttachToComponent(FollowCamera, FAttachmentTransformRules::SnapToTargetIncludingScale);
	Weapon->SetScopeCaptureActive(true);
	if (GetMesh())
	{
		GetMesh()->HideBoneByName(TEXT("head"), PBO_None);
	}
}

void ADropCharacter::EndScopedWeapon()
{
	AWeaponBase* Weapon = ScopedWeapon.Get();
	ScopedWeapon = nullptr;
	ScopeAlpha = 0.f;
	if (GetMesh())
	{
		GetMesh()->UnHideBoneByName(TEXT("head"));
	}
	if (!Weapon)
	{
		return;
	}
	Weapon->SetScopeCaptureActive(false);
	Weapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, WeaponInventory->GetCurrentAttachSocket());
	Weapon->SetActorRelativeLocation(FVector::ZeroVector);
	Weapon->SetActorRelativeRotation(FRotator::ZeroRotator);
}

FTransform ADropCharacter::GetScopedWeaponTransform(const AWeaponBase* Weapon) const
{
	if (Weapon->UsesScopedLens())
	{
		return FTransform(ScopedWeaponRotation, ScopedWeaponOffset);
	}
	const UStaticMeshComponent* WM = Weapon->GetWeaponMesh();
	if (WM && WM->DoesSocketExist(TEXT("Aim")))
	{
		FTransform Inv = WM->GetSocketTransform(TEXT("Aim"), RTS_Actor).Inverse();
		Inv.ConcatenateRotation(Weapon->GetAimCameraRotationOffset().Quaternion());
		return FTransform(Inv.GetRotation(), Inv.GetLocation());
	}
	return FTransform::Identity;
}

/*
스코프 - 저격 렌즈
*/
void ADropCharacter::ShowSniperScope()
{
	if (!IsLocallyControlled() || !ScopeCapture)
	{
		return;
	}

	if (!ScopeRenderTarget)
	{
		ScopeRenderTarget = NewObject<UTextureRenderTarget2D>(this);
		ScopeRenderTarget->InitAutoFormat(ScopeRenderTargetSize, ScopeRenderTargetSize);
		ScopeRenderTarget->UpdateResourceImmediate(true);
		ScopeCapture->TextureTarget = ScopeRenderTarget;
	}

	if (const AWeaponBase* Weapon = WeaponInventory ? WeaponInventory->GetEquippedWeapon() : nullptr)
	{
		ScopeCapture->FOVAngle = Weapon->GetScopedFOV();
	}
	ScopeCapture->bCaptureEveryFrame = true;
	ScopeCapture->SetActive(true);

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (!SniperScopeOverlayClass)
		{
			return;
		}
		if (!SniperScopeOverlayWidget)
		{
			SniperScopeOverlayWidget = CreateWidget<UUserWidget>(PC, SniperScopeOverlayClass);
			if (SniperScopeOverlayWidget)
			{
				SniperScopeOverlayWidget->AddToViewport(10); // 크로스헤어보다 위에
			}
		}
		if (SniperScopeOverlayWidget)
		{
			if (!ScopeLensMID && ScopeLensMaterial)
			{
				ScopeLensMID = UMaterialInstanceDynamic::Create(ScopeLensMaterial, this);
			}
			if (ScopeLensMID)
			{
				ScopeLensMID->SetTextureParameterValue(TEXT("ScopeTexture"), ScopeRenderTarget);
				if (UImage* LensImage = Cast<UImage>(SniperScopeOverlayWidget->GetWidgetFromName(TEXT("LensImage"))))
				{
					LensImage->SetBrushFromMaterial(ScopeLensMID);

					const FVector2D LensSize = LensImage->GetCachedGeometry().GetLocalSize();
					if (LensSize.Y > 0.f)
					{
						ScopeLensMID->SetScalarParameterValue(TEXT("AspectRatio"), LensSize.X / LensSize.Y);
					}
				}
			}
			SniperScopeOverlayWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}
}

void ADropCharacter::HideSniperScope()
{
	if (ScopeCapture)
	{
		ScopeCapture->SetActive(false);
		ScopeCapture->bCaptureEveryFrame = false;
	}
	if (SniperScopeOverlayWidget)
	{
		SniperScopeOverlayWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

/*
무기
*/
AWeaponBase* ADropCharacter::GetEquippedWeapon() const
{
	return WeaponInventory ? WeaponInventory->GetEquippedWeapon() : nullptr;
}

EFireMode ADropCharacter::GetCurrentFireMode() const
{
	return WeaponInventory ? WeaponInventory->GetCurrentFireMode() : EFireMode::Single;
}

void ADropCharacter::MeleeAttack(UAnimMontage* AttackMontage)
{
	if (!AttackMontage)
	{
		return;
	}
	if (HasAuthority())
	{
		MulticastPlayMeleeMontage(AttackMontage);
	}
	else
	{
		ServerMeleeAttack(AttackMontage);
	}
}

void ADropCharacter::MulticastPlayMeleeMontage_Implementation(UAnimMontage* AttackMontage)
{
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		if (AttackMontage && !AnimInstance->IsAnyMontagePlaying())
		{
			AnimInstance->Montage_Play(AttackMontage, 1.2f);
			AnimInstance->Montage_JumpToSection(TEXT("Attack"), AttackMontage);
		}
	}
}

void ADropCharacter::ServerMeleeAttack_Implementation(UAnimMontage* AttackMontage)
{
	MulticastPlayMeleeMontage(AttackMontage);
}

/*
사망/피격
*/
void ADropCharacter::HandleDeath(AController* Killer, AActor* DamageCauser)
{
	if (InteractionComp) InteractionComp->CancelUseItem();
	if (!HasAuthority() || !Killer)
	{
		return;
	}
	ADropPlayerController* KillerPC = Cast<ADropPlayerController>(Killer);
	if (!KillerPC)
	{
		return;
	}
	// 1. 공격자의 PlayerState(점수/킬수 정보 저장소)를 가져옴
	ADropPlayerState* KillerPS = Killer->GetPlayerState<ADropPlayerState>();

	// 2. 킬 카운트를 1 증가시킴 (PlayerState가 있으면 기존 킬수+1)
	const int32 NewKillCount = KillerPS ? ++KillerPS->KillCount : 1;

	// 3. 나를 죽인 원인(DamageCauser)이 총알(AProjectileBullet)인지 확인
	const AProjectileBullet* Bullet = Cast<AProjectileBullet>(DamageCauser);

	// 4. 총알 정보가 있으면 해당 총기 이름을 가져오고, 없으면 기본값 사용
	const FText WeaponName = Bullet ? Bullet->GetWeaponDisplayName() : FText::FromString(TEXT("무기"));

	// 5. 당신의 [GetWeaponDisplayName]로 인해 상대플레이어가 사망했습니다
	const FText Line1 = FText::Format(
		NSLOCTEXT("Combat", "KillFeed", "당신의 {0}로 인해 상대플레이어가 사망했습니다"), WeaponName);

	// 6. "1 킬" (또는 "2 킬", "3 킬" 등)
	const FText Line2 = FText::FromString(FString::Printf(TEXT("%d 킬"), NewKillCount));

	// 7. 킬러의 PlayerController를 통해 화면 중앙에 주황색(FLinearColor) 알림 텍스트를 띄움!
	KillerPC->ClientShowCenterNotification(Line1, Line2, FLinearColor(1.f, 0.55f, 0.1f));
}

void ADropCharacter::HandleOwnDeath(AController* Killer, AActor* DamageCauser)
{
	if (!HasAuthority())
	{
		return;
	}
	Multicast_Die();
	GetWorldTimerManager().SetTimer(DeathDestroyTimerHandle, this, &ADropCharacter::DestroySelf, DeathDestroyDelay, false);
}

void ADropCharacter::ForceStopAim()
{
	if (AimMode == EDropAimMode::Hip)
	{
		return;
	}
	const EDropAimMode OldMode = AimMode;
	AimMode = EDropAimMode::Hip;
	bIsAiming = false;
	ApplyAimVisuals(OldMode, EDropAimMode::Hip);
}

void ADropCharacter::DestroySelf()
{
	Destroy();
}

void ADropCharacter::HandleHit(AController* InstigatorController, AActor* DamageCauser, FVector ShotDirection)
{
	if (!HasAuthority() || !CanAct())
	{
		return;
	}
	// https://developer-bing-gu.tistory.com/entry/UnrealC-Launch-Character-%EB%8F%99%EC%9E%91-%ED%95%98%EC%A7%80-%EC%95%8A%EB%8A%94-%EC%9D%B4%EC%9C%A0
	LaunchCharacter(-ShotDirection * KnockbackPower, true, true);
	Multicast_HitReact(ShotDirection);
}

void ADropCharacter::Multicast_HitReact_Implementation(FVector ShotDirection)
{
	if (!CanAct() || HitReactMontages.Num() == 0)
	{
		return;
	}
	// 총알이 날아온 방향(ShotDirection)을 계산해서 몇 번 피격 모션을 틀지 인덱스를 구함 (예: 0번=전방, 1번=후방, 2번=좌측, 3번=우측 피격 몽타주)
	const int32 Index = GetHitDirectionIndex(ShotDirection);
	if (!HitReactMontages.IsValidIndex(Index) || !HitReactMontages[Index])
	{
		return;
	}
	// 캐릭터 메쉬에서 애니메이션 인스턴스 불러오기
	if (UAnimInstance* AnimInst = GetMesh()->GetAnimInstance())
	{
		AnimInst->Montage_Play(HitReactMontages[Index]);
	}
}

int32 ADropCharacter::GetHitDirectionIndex(const FVector& ShotDirection) const
{
	const FVector ToAttacker = ShotDirection;
	const float ForwardDot = FVector::DotProduct(GetActorForwardVector(), ToAttacker);
	const float RightDot = FVector::DotProduct(GetActorRightVector(), ToAttacker);
	if (FMath::Abs(ForwardDot) >= FMath::Abs(RightDot))
	{
		return ForwardDot >= 0.f ? 0 : 1;
	}
	return RightDot >= 0.f ? 3 : 2;
}

void ADropCharacter::Multicast_Die_Implementation()
{
	if (bIsDead)
	{
		return;
	}
	bIsDead = true;
	ForceStopAim();

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->SetMovementMode(MOVE_None);
	GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
	GetMesh()->SetSimulatePhysics(true);
	if (DeathMontages.Num() > 0)
	{
		UAnimMontage* Montage =  DeathMontages[FMath::RandHelper(DeathMontages.Num())];
		if (UAnimInstance* AnimInst = GetMesh()->GetAnimInstance())
		{
			AnimInst->Montage_Play(Montage);
		}
	}
	if (IsLocallyControlled())
	{
		if (ADropPlayerController* PC = Cast<ADropPlayerController>(GetController()))
		{
			PC->ShowDeathUI();
		}
	}
}

/*
인벤토리 캐릭터 프리뷰 (자신 + 부착 무기만 캡처, 배경 투명)
*/
void ADropCharacter::StartInventoryPreview(UImage* TargetImage)
{
	if (!IsLocallyControlled() || !PreviewCapture)
	{
		return;
	}

	if (!PreviewRenderTarget)
	{
		PreviewRenderTarget = NewObject<UTextureRenderTarget2D>(this);
		PreviewRenderTarget->ClearColor = FLinearColor(0.f, 0.f, 0.f, 1.f); // 알파 1 = 빈 배경
		PreviewRenderTarget->InitCustomFormat(PreviewRenderTargetSize.X, PreviewRenderTargetSize.Y, PF_FloatRGBA, true);
		PreviewRenderTarget->UpdateResourceImmediate(true);
		PreviewCapture->TextureTarget = PreviewRenderTarget;
	}

	PreviewCapture->SetRelativeLocationAndRotation(PreviewCaptureOffset, FRotator(0.f, 180.f, 0.f));
	PreviewCapture->FOVAngle = PreviewFOV;
	RefreshPreviewShowList();
	PreviewCapture->bCaptureEveryFrame = true;
	PreviewCapture->SetActive(true);

	if (!TargetImage || !PreviewMaterial)
	{
		return;
	}
	if (!PreviewMID)
	{
		PreviewMID = UMaterialInstanceDynamic::Create(PreviewMaterial, this);
	}
	PreviewMID->SetTextureParameterValue(TEXT("PreviewTexture"), PreviewRenderTarget);
	PreviewMID->SetScalarParameterValue(TEXT("Brightness"), PreviewBrightness);
	TargetImage->SetBrushFromMaterial(PreviewMID);
	TargetImage->SetColorAndOpacity(FLinearColor::White); // WBP 기본 알파 0
}

void ADropCharacter::StopInventoryPreview()
{
	if (PreviewCapture)
	{
		PreviewCapture->SetActive(false);
		PreviewCapture->bCaptureEveryFrame = false;
	}
}

void ADropCharacter::RefreshPreviewShowList()
{
	PreviewCapture->ShowOnlyActors.Reset();
	PreviewCapture->ShowOnlyActors.Add(this);

	TArray<AActor*> Attached;
	GetAttachedActors(Attached, true, true);
	for (AActor* Actor : Attached)
	{
		PreviewCapture->ShowOnlyActors.Add(Actor);
	}
}
