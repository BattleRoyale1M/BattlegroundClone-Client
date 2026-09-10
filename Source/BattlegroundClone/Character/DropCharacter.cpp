#include "Character/DropCharacter.h"

#include "Combat/HealthComponent.h"
#include "Weapon/WeaponBase.h"
#include "Drop/AirPlane.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Controller.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"

#include "Net/UnrealNetwork.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
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

	ParachuteMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ParachuteMesh"));
	ParachuteMesh->SetupAttachment(GetCapsuleComponent());
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ParachuteMeshAsset(
		TEXT("/Script/Engine.StaticMesh'/Game/Fab/Parachute/SM_Parachute.SM_Parachute'"));
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
}

void ADropCharacter::BeginPlay()
{
	Super::BeginPlay();
	EquipDefaultWeapon();

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}
}

/*
낙하산
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

void ADropCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
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
		if (IsLocallyControlled()) UpdateAimCamera(DeltaTime);
		break;
	default:
		break;
	}
	if (ParachuteDeployElapsed >= 0.f)
	{
		UpdateParachuteVisual(DeltaTime);
	}
}

void ADropCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EIC)
	{
		return;
	}

	if (MoveAction)
	{
		EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ADropCharacter::Move);
	}
	if (LookAction)
	{
		EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &ADropCharacter::Look);
	}
	if (MouseLookAction)
	{
		EIC->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ADropCharacter::Look);
	}
	if (JumpAction)
	{
		EIC->BindAction(JumpAction, ETriggerEvent::Started,   this, &ADropCharacter::OnJumpPressed);
		EIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
	}
	if (ParachuteAction)
	{
		EIC->BindAction(ParachuteAction, ETriggerEvent::Started, this, &ADropCharacter::OnParachutePressed);
	}
	if (FireAction)
	{
		EIC->BindAction(FireAction, ETriggerEvent::Started, this, &ADropCharacter::StartFire);
		EIC->BindAction(FireAction, ETriggerEvent::Completed, this, &ADropCharacter::StopFire);
	}
	if (ReloadAction)
	{
		EIC->BindAction(ReloadAction, ETriggerEvent::Started, this, &ADropCharacter::OnReloadPressed);
	}
	if (AimAction)
	{
		EIC->BindAction(AimAction, ETriggerEvent::Started,   this, &ADropCharacter::OnAimPressed);
		EIC->BindAction(AimAction, ETriggerEvent::Completed, this, &ADropCharacter::OnAimReleased);
	}
}

void ADropCharacter::OnJumpPressed()
{
	
	if (DropState == EDropState::InPlane)
	{
		ServerBeginFreefall();
	}
	else
	{
		Jump();
	}
}

void ADropCharacter::OnParachutePressed()
{
	if (DropState == EDropState::Freefall)
	{
		ServerDeployParachute();
	}
}

void ADropCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	if (!Controller || Axis.IsNearlyZero())
	{
		return;
	}

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
}

/*
낙하산
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
	}
	ApplyDropState(Old, NewState);
	PrevDropState = NewState;
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
활강속도 
*/
void ADropCharacter::UpdateFreefall(float Dt)
{
	UCharacterMovementComponent* M = GetCharacterMovement();
	if (!M) return;

	FVector Desired = GetControlRotation().Vector() * 800.f; // 조작감용 약간의 활강
	Desired.Z = -FreefallMaxSpeed;                            // 항상 빠르게 수직 낙하
	M->Velocity = FMath::VInterpTo(M->Velocity, Desired, Dt, FreefallAccel); // 바람저항
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

/*
Aim
*/
void ADropCharacter::OnAimPressed()
{
	if (DropState != EDropState::Ground) return;
	AimPressTime = GetWorld()->GetTimeSeconds();
	if (AimMode == EDropAimMode::Scoped) return; // 스코프 중 다시 누름
	SetAimMode(EDropAimMode::Shoulder); 
}

// scope 해제
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

void ADropCharacter::ApplyAimVisuals(EDropAimMode OldMode, EDropAimMode NewMode)
{
	// 1인칭 스코프
	if (IsLocallyControlled() && EquippedWeapon)
	{
		if (NewMode == EDropAimMode::Scoped)
		{
			EquippedWeapon->AttachToComponent(
				FollowCamera,
				FAttachmentTransformRules::SnapToTargetIncludingScale);

			if (UStaticMeshComponent* WM = EquippedWeapon->GetWeaponMesh())
			{
				if (WM->DoesSocketExist(TEXT("Aim")))
				{
					// 역변환
					const FTransform Inv = WM->GetSocketTransform(TEXT("Aim"), RTS_Actor).Inverse();
					EquippedWeapon->SetActorRelativeLocation(Inv.GetLocation());
					EquippedWeapon->SetActorRelativeRotation(Inv.GetRotation().Rotator());
				}
			}

			if (GetMesh())
			{
				GetMesh()->HideBoneByName(TEXT("head"), PBO_None);
			}
		}
		else if (OldMode == EDropAimMode::Scoped)
		{
			EquippedWeapon->AttachToComponent(
				GetMesh(),
				FAttachmentTransformRules::SnapToTargetIncludingScale,
				WeaponAttachSocket);
			EquippedWeapon->SetActorRelativeLocation(FVector::ZeroVector);
			EquippedWeapon->SetActorRelativeRotation(FRotator::ZeroRotator);
			if (GetMesh())
			{
				GetMesh()->UnHideBoneByName(TEXT("head"));
			}
		}
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
	
	float   TargetArm;
	FVector TargetOffset;
	float   TargetFOV;

	switch (AimMode)
	{
	case EDropAimMode::Shoulder:
		TargetArm = ShoulderArmLength; TargetOffset = ShoulderSocketOffset; TargetFOV = ShoulderFOV;
		break;
	case EDropAimMode::Scoped:
		TargetArm = ScopedArmLength;   TargetOffset = ScopedSocketOffset;   TargetFOV = ScopedFOV;
		break;
	default:
		TargetArm = HipArmLength;      TargetOffset = HipSocketOffset;      TargetFOV = HipFOV;
		break;
	}

	// FMath::FInterpTo(현재값, 목표값, 시간, 보간된 속도) : 누적해서 점차 목표값에 가까워지게 하는 선형보간함수 중 하나
	CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, TargetArm,    Dt, AimInterpSpeed);
	CameraBoom->SocketOffset    = FMath::VInterpTo(CameraBoom->SocketOffset,    TargetOffset, Dt, AimInterpSpeed);
	FollowCamera->SetFieldOfView(FMath::FInterpTo(FollowCamera->FieldOfView,    TargetFOV,    Dt, AimInterpSpeed));
	
	if (AimMode == EDropAimMode::Scoped && EquippedWeapon && EquippedWeapon->GetRootComponent()
	&& EquippedWeapon->GetRootComponent()->GetAttachParent() == FollowCamera)
	{
		if (UStaticMeshComponent* WM = EquippedWeapon->GetWeaponMesh())
		{
			if (WM->DoesSocketExist(TEXT("Aim")))
			{
				const FTransform Inv = WM->GetSocketTransform(TEXT("Aim"), RTS_Actor).Inverse();
				EquippedWeapon->SetActorRelativeLocation(Inv.GetLocation());
				EquippedWeapon->SetActorRelativeRotation(Inv.GetRotation().Rotator());
			}
		}
	}

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

void ADropCharacter::StartFire()
{
	if (DropState != EDropState::Ground || !bIsAiming || !EquippedWeapon)
	{
		return;
	}
	EquippedWeapon->StartFire();
}

void ADropCharacter::StopFire()
{
	if (EquippedWeapon)
	{
		EquippedWeapon->StopFire();
	}
}

void ADropCharacter::OnReloadPressed()
{
	if (EquippedWeapon)
	{
		EquippedWeapon->StartReload();
	}
}

void ADropCharacter::OnRep_EquippedWeapon()
{
	if (!EquippedWeapon)
	{
		return;
	}
	EquippedWeapon->AttachToComponent(
		GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, WeaponAttachSocket);
}

void ADropCharacter::EquipDefaultWeapon()
{
	if (!HasAuthority())
	{
		return;
	}
	if (!DefaultWeaponClass || EquippedWeapon)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	EquippedWeapon = GetWorld()->SpawnActor<AWeaponBase>(DefaultWeaponClass, SpawnParams);
	if (EquippedWeapon)
	{
		EquippedWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, WeaponAttachSocket);
	}
}

/*
무기 재장전(Reload) 애니메이션 몽타주를 재장전 소요 시간(Duration)에 맞춰 재생 속도(Rate)를 동적으로 조절하여 실행
*/
void ADropCharacter::HandleReloadStarted(float Duration)
{
	if (!ReloadAnimMontage) return;
	UAnimInstance* Anim = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr; //몽타주가 등록되지 않았다면 조기 종료
	
	if (!Anim) return;
	const float MontageLen = ReloadAnimMontage->GetPlayLength();
	const float Rate = (Duration > 0.f && MontageLen > 0.f) ? (MontageLen / Duration) : 1.f;
	Anim->Montage_Play(ReloadAnimMontage, Rate);
}

void ADropCharacter::PlayFireMontage()
{
	if (!FireAnimMontage) return;
	UAnimInstance* Anim = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!Anim) return;
	Anim->Montage_Play(FireAnimMontage);
}


/*
RPC Server
*/
void ADropCharacter::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADropCharacter, EquippedWeapon);
	DOREPLIFETIME_CONDITION(ADropCharacter, AimMode, COND_SkipOwner);
	DOREPLIFETIME(ADropCharacter, DropState);
}

void ADropCharacter::ApplyDropState(EDropState OldState, EDropState NewState)
{
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
	
	// 메시 가시성 (전원)
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetVisibility(NewState != EDropState::InPlane);
	}
	// 카메라 (내 화면 전용)
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

	// 낙하산 프롬프트 (내부에서 이미 IsLocallyControlled 체크)
	if (NewState == EDropState::Freefall)
	{
		ShowParachutePrompt();
	}
	else
	{
		HideParachutePrompt();
	}
	// 낙하산 생김새 분기
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
낙하산
*/
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

void ADropCharacter::OnRep_DropState()
{
	ApplyDropState(PrevDropState, DropState);
	PrevDropState = DropState;
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
HP
*/
float ADropCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (HealthComp)
	{
		HealthComp -> ApplyDamage(DamageAmount);
	}
	return Applied;
}

/*
 DEBUG
*/
void ADropCharacter::DbgHurt(float Amt)
{
	if (HasAuthority() && HealthComp) HealthComp->ApplyDamage(Amt);
}

