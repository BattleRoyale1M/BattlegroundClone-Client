#include "Character/DropCharacter.h"
#include "Drop/AirPlane.h"

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
#include "Components/CapsuleComponent.h"

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
}

void ADropCharacter::BeginPlay()
{
	Super::BeginPlay();

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
	if (DropState != EDropState::InPlane) return;

	DetachFromActor(FDetachmentTransformRules(EDetachmentRule::KeepWorld, true));
	BoardedPlane = nullptr;

	if (CameraBoom)
	{
		CameraBoom->TargetArmLength = DefaultArmLength;
		CameraBoom->SocketOffset    = FVector::ZeroVector;
		CameraBoom->bDoCollisionTest = true;
	}

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
		UpdateFreefall(DeltaTime);
		if (GroundDistance() <= AutoDeployHeight) DeployParachute();
		break;
	case EDropState::Parachuting:
		 UpdateParachute(DeltaTime);
		if (GroundDistance() <=LandHeight) SetDropState(EDropState::Ground);
		break;
	default:
		break;
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
}

void ADropCharacter::OnJumpPressed()
{
	
	if (DropState == EDropState::InPlane)
	{
		BeginFreefall();
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
		DeployParachute();
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
	DropState = NewState;
	
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
		MeshComp -> SetVisibility(NewState != EDropState::InPlane);
	}
	if (NewState == EDropState::Freefall)
	{
		ShowParachutePrompt();
	}
	else
	{
		HideParachutePrompt();
	}

	OnDropStateChanged(NewState, Old);
}

void ADropCharacter::EnterPlane(AAirPlane* Plane, USceneComponent* Seat)
{
	if (!Plane || !Seat)
	{
		return;
	}
	BoardedPlane = Plane;
	AttachToComponent(Seat, FAttachmentTransformRules::SnapToTargetIncludingScale);
	SetDropState(EDropState::InPlane);
	
	if (AController* C = GetController())
	{
		C->SetControlRotation(FRotator(InPlaneCameraPitch, Plane->GetHeadingYaw() + InPlaneYawOffset, 0.f));
	}
	
	/*
	비행기 전체가 보이도록 붐을 멀리 + 위로. 붐이 지형/메시에 튕겨 들어오지 않게 콜리전 테스트 끔.
	*/
	if (CameraBoom)
	{
		CameraBoom->TargetArmLength = InPlaneArmLength;
		CameraBoom->SocketOffset = InPlaneSocketOffset;
		CameraBoom->bDoCollisionTest = false;
	}
	
}

/*
낙하산
*/
void ADropCharacter::DeployParachute()
{
	if (DropState != EDropState::Freefall) return;
	SetDropState(EDropState::Parachuting);
	// TODO: (메시 준비되면)
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

void ADropCharacter::UpdateFreefall(float Dt)
{
	UCharacterMovementComponent* M = GetCharacterMovement();
	if (!M) return;
	
	/*
	자유낙하(Freefall) 속도를 가변적으로 조절
	*/
	const FRotator Ctrl = GetControlRotation(); // pawn 내장 함수
	const FVector Aim = Ctrl.Vector();
	const float PitchDeg = FRotator::NormalizeAxis(Ctrl.Pitch);
	const float DiveFrac = FMath::GetMappedRangeValueUnclamped(FVector2D(-10.f, -70.f), FVector2D(0.f, 1.f), PitchDeg);
	const float Speed = FMath::Lerp(FreefallMinSpeed, FreefallMaxSpeed, DiveFrac);
	
	FVector Desired = Aim * Speed; // camera가 바라보는 방향 벡터 * 구현 낙하 속도
	Desired.Z = FMath::Min(Desired.Z, -FreefallMinSpeed); // UE에서 Z축은 위가 (+), 아래가 (-)
	M->Velocity = FMath::VInterpTo(M->Velocity, Desired, Dt, FreefallAccel);
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