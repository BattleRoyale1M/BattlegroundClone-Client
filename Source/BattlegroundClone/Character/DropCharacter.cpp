#include "Character/DropCharacter.h"

#include "Combat/HealthComponent.h"
#include "Weapon/WeaponBase.h"
#include "Weapon/WeaponInventoryComponent.h"
#include "Interaction/Interactable/InteractableInterface.h"
#include "Drop/AirPlane.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Controller.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"

#include "Combat/ProjectileBullet.h"
#include "Core/DropPlayerController.h"
#include "Core/DropPlayerState.h"

#include "Net/UnrealNetwork.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
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
	ScopeCapture->FOVAngle = 8.f; // ShowSniperScope에서 무기별 값으로 덮어씀
	ScopeCapture->SetActive(false);

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
}

void ADropCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (WeaponInventory)
	{
		WeaponInventory->InitialEquip();
	}

	GetCapsuleComponent()->OnComponentBeginOverlap.AddDynamic(this, &ADropCharacter::OnInteractableBeginOverlap);
	GetCapsuleComponent()->OnComponentEndOverlap.AddDynamic(this, &ADropCharacter::OnInteractableEndOverlap);
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
		if (IsLocallyControlled() || HasAuthority()) UpdateAimCamera(DeltaTime);
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
	if (ChangeFireModeAction && WeaponInventory)
	{
		EIC->BindAction(ChangeFireModeAction, ETriggerEvent::Started, WeaponInventory.Get(), &UWeaponInventoryComponent::ChangeFireMode);
	}
	if (WeaponInventory)
	{
		for (int32 i = 0; i < WeaponSlotActions.Num(); ++i)
		{
			if (WeaponSlotActions[i])
			{
				EIC->BindAction(WeaponSlotActions[i], ETriggerEvent::Started, WeaponInventory.Get(), &UWeaponInventoryComponent::SwitchWeaponSlot, i);
			}
		}
	}
	if (InteractAction)
	{
		EIC->BindAction(InteractAction, ETriggerEvent::Started, this, &ADropCharacter::OnInteractPressed);
	}
}

void ADropCharacter::OnJumpPressed()
{
	Jump();
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
스코프
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

/*
저격 스코프 렌즈: SceneCapture로 좁은 FOV를 렌더타겟에 찍고, 원형 마스크 머티리얼로 화면 중앙에 표시.
메인 카메라는 줌하지 않고, 화면 전체가 아니라 렌즈 원 안에서만 확대되어 보임.
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
				const FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(this);
				if (ViewportSize.Y > 0.f)
				{
					// 원형 마스크가 화면비에 관계없이 항상 동그랗게 보이도록
					ScopeLensMID->SetScalarParameterValue(TEXT("AspectRatio"), ViewportSize.X / ViewportSize.Y);
				}
				if (UImage* LensImage = Cast<UImage>(SniperScopeOverlayWidget->GetWidgetFromName(TEXT("LensImage"))))
				{
					LensImage->SetBrushFromMaterial(ScopeLensMID);
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
	const bool bIsAWPEquipped = Weapon && Weapon->GetClass()->GetName().Contains(TEXT("AWP"));

	if (NewMode == EDropAimMode::Scoped && bIsAWPEquipped)
	{
		ShowSniperScope();
	}
	else if (OldMode == EDropAimMode::Scoped)
	{
		HideSniperScope();
	}

	if (IsLocallyControlled() && Weapon)
	{
		if (NewMode == EDropAimMode::Scoped)
		{
			Weapon->AttachToComponent(
			   FollowCamera,
			   FAttachmentTransformRules::SnapToTargetIncludingScale);

			if (bIsAWPEquipped)
			{
				Weapon->SetActorRelativeLocation(ScopedWeaponOffset);
				Weapon->SetActorRelativeRotation(ScopedWeaponRotation);
				Weapon->SetActorHiddenInGame(true);
			}
			else if (UStaticMeshComponent* WM = Weapon->GetWeaponMesh())
			{
				if (WM->DoesSocketExist(TEXT("Aim")))
				{
					const FTransform SocketTransform = WM->GetSocketTransform(TEXT("Aim"), RTS_Actor);
					FTransform Inv = SocketTransform.Inverse();

					const FRotator AxisCorrection = Weapon->GetAimCameraRotationOffset();
					Inv.ConcatenateRotation(AxisCorrection.Quaternion());

					Weapon->SetActorRelativeLocation(Inv.GetLocation());
					Weapon->SetActorRelativeRotation(Inv.GetRotation().Rotator());
				}
			}

			if (GetMesh())
			{
				GetMesh()->HideBoneByName(TEXT("head"), PBO_None);
			}
		}
		else if (OldMode == EDropAimMode::Scoped)
		{
			Weapon->AttachToComponent(
				GetMesh(),
				FAttachmentTransformRules::SnapToTargetIncludingScale,
				WeaponInventory->GetWeaponAttachSocket());
			Weapon->SetActorRelativeLocation(FVector::ZeroVector);
			Weapon->SetActorRelativeRotation(FRotator::ZeroRotator);
			Weapon->SetActorHiddenInGame(false); // AWP가 아니었으면 애초에 숨긴 적 없으니 안전한 no-op
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
		// 메인 카메라는 줌하지 않음 - 확대는 ScopeCapture 렌즈(원형 UI)가 담당
		TargetArm = ScopedArmLength; TargetOffset = ScopedSocketOffset; TargetFOV = ShoulderFOV;
		break;
	default:
		TargetArm = HipArmLength;      TargetOffset = HipSocketOffset;      TargetFOV = HipFOV;
		break;
	}

	// FMath::FInterpTo(현재값, 목표값, 시간, 보간된 속도) : 누적해서 점차 목표값에 가까워지게 하는 선형보간함수 중 하나
	CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, TargetArm,    Dt, AimInterpSpeed);
	CameraBoom->SocketOffset    = FMath::VInterpTo(CameraBoom->SocketOffset,    TargetOffset, Dt, AimInterpSpeed);
	FollowCamera->SetFieldOfView(FMath::FInterpTo(FollowCamera->FieldOfView,    TargetFOV,    Dt, AimInterpSpeed));
	
	AWeaponBase* Weapon = WeaponInventory ? WeaponInventory->GetEquippedWeapon() : nullptr;
	if (AimMode == EDropAimMode::Scoped && Weapon && Weapon->GetRootComponent()
	&& Weapon->GetRootComponent()->GetAttachParent() == FollowCamera
	&& !Weapon->GetClass()->GetName().Contains(TEXT("AWP"))) // AWP는 ApplyAimVisuals에서 고정 오프셋으로 한 번만 세팅하면 됨
	{
		if (UStaticMeshComponent* WM = Weapon->GetWeaponMesh())
		{
			if (WM->DoesSocketExist(TEXT("Aim")))
			{
				const FTransform SocketTransform = WM->GetSocketTransform(TEXT("Aim"), RTS_Actor);
				FTransform Inv = SocketTransform.Inverse();

				const FRotator AxisCorrection = Weapon->GetAimCameraRotationOffset();
				Inv.ConcatenateRotation(AxisCorrection.Quaternion());

				Weapon->SetActorRelativeLocation(Inv.GetLocation());
				Weapon->SetActorRelativeRotation(Inv.GetRotation().Rotator());
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
	if (DropState != EDropState::Ground || !bIsAiming)
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
	if (WeaponInventory)
	{
		WeaponInventory->OnReloadPressed();
	}
}

void ADropCharacter::OnInteractableBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor || !OtherActor->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()))
	{
		return;
	}
	NearbyInteractables.AddUnique(OtherActor);
	UpdateCurrentInteractable();
}

void ADropCharacter::OnInteractableEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	NearbyInteractables.Remove(OtherActor);
	UpdateCurrentInteractable();
}

void ADropCharacter::UpdateCurrentInteractable()
{
	AActor* Best = NearbyInteractables.Num() > 0 ? NearbyInteractables[0] : nullptr;
	if (Best == CurrentInteractable)
	{
		return;
	}
	CurrentInteractable = Best;

	if (IsLocallyControlled())
	{
		const FText Prompt = CurrentInteractable
			? IInteractableInterface::Execute_GetInteractionPromptText(CurrentInteractable)
			: FText::GetEmpty();
		OnInteractableChanged.Broadcast(Prompt);
	}
}

void ADropCharacter::OnInteractPressed()
{
	if (DropState != EDropState::Ground || !CurrentInteractable)
	{
		return;
	}
	ServerInteract(CurrentInteractable);
}

void ADropCharacter::ServerInteract_Implementation(AActor* InteractActor)
{
	if (InteractActor && InteractActor->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()))
	{
		IInteractableInterface::Execute_Interact(InteractActor, this);
	}
}

AWeaponBase* ADropCharacter::GetEquippedWeapon() const
{
	return WeaponInventory ? WeaponInventory->GetEquippedWeapon() : nullptr;
}

EFireMode ADropCharacter::GetCurrentFireMode() const
{
	return WeaponInventory ? WeaponInventory->GetCurrentFireMode() : EFireMode::Single;
}

/*
RPC Server
*/
void ADropCharacter::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(ADropCharacter, AimMode, COND_SkipOwner);
	DOREPLIFETIME(ADropCharacter, DropState);
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

void ADropCharacter::HandleDeath(AController* Killer, AActor* DamageCauser)
{
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

void ADropCharacter::DestroySelf()
{
	Destroy();
}

void ADropCharacter::HandleHit(AController* InstigatorController, AActor* DamageCauser, FVector ShotDirection)
{
	if (!HasAuthority() || bIsDead)
	{
		return;
	}
	// https://developer-bing-gu.tistory.com/entry/UnrealC-Launch-Character-%EB%8F%99%EC%9E%91-%ED%95%98%EC%A7%80-%EC%95%8A%EB%8A%94-%EC%9D%B4%EC%9C%A0
	LaunchCharacter(-ShotDirection * KnockbackPower, true, true);
	Multicast_HitReact(ShotDirection);
}

void ADropCharacter::Multicast_HitReact_Implementation(FVector ShotDirection)
{
	if (bIsDead || HitReactMontages.Num() == 0)
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
}

/*
 DEBUG
*/
void ADropCharacter::DbgHurt(float Amt)
{
	if (HasAuthority())
	{
		if (HealthComp) HealthComp->ApplyDamage(Amt, nullptr, nullptr, FVector::ZeroVector);
	}
	else
	{
		Server_DbgHurt(Amt);
	}
}

void ADropCharacter::Server_DbgHurt_Implementation(float Amt)
{
	if (HealthComp) HealthComp->ApplyDamage(Amt, nullptr, nullptr, FVector::ZeroVector);
}

