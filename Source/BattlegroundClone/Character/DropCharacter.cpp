// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/DropCharacter.h"
#include "Drop/AirPlane.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "Components/SkeletalMeshComponent.h"

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
		EIC->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
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
		C->SetControlRotation(FRotator(InPlaneCameraPitch, Plane->GetHeadingYaw(), 0.f));
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
	
	FVector Desired = Aim * Speed;
	Desired.Z = FMath::Min(Desired.Z, -FreefallMinSpeed);
	M->Velocity = FMath::VInterpTo(M->Velocity, Desired, Dt, FreefallAccel);
}