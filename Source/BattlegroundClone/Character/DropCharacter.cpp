#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Controller.h"
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

	// 낙하/낙하산 중엔 카메라를 뒤로 빼서 캐노피까지 보이게, 착지 시 원상복귀
	if (CameraBoom)
	{
		switch (NewState)
		{
		case EDropState::Freefall:
		case EDropState::Parachuting:
			CameraBoom->TargetArmLength   = DescentArmLength;
			CameraBoom->SocketOffset      = DescentSocketOffset;
			CameraBoom->bDoCollisionTest  = false;
			break;
		case EDropState::Ground:
			CameraBoom->TargetArmLength   = DefaultArmLength;
			CameraBoom->SocketOffset      = FVector::ZeroVector;
			CameraBoom->bDoCollisionTest  = true;
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
	// 낙하산 생김새 분기
	if (NewState == EDropState::Parachuting)
	{
		ShowParachute();
	}
	else if (Old == EDropState::Parachuting)
	{
		HideParachute();
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
	if (DropState != EDropState::Ground || bReloading)
	{
		return;
	}
	bFireHeld = true;
	Fire();
	const float Interval = 60.f / FMath::Max(RoundsPerMinute, 1.f);
	
	// 사격 간격(Interval)마다 ADropCharacter 클래스의 Fire 함수를 계속(반복) 실행하도록 타이머를 ON
	GetWorldTimerManager().SetTimer(
		FireTimerHandle, this, &ADropCharacter::Fire, Interval, true);
}

void ADropCharacter::StopFire()
{
	bFireHeld = false;
	GetWorldTimerManager().ClearTimer(FireTimerHandle);
}

void ADropCharacter::Fire()
{
	// 발동 조건
	if (bReloading)
	{
		return;
	}
	if (CurrentAmmo <= 0)
	{
		StopFire();
		if (ReserveAmmo > 0)
		{
			Reload();
		}
		else if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, TEXT("*click*"));
		}
		return;
	}
	AController* C = GetController();
	if (!C)
	{
		return;
	}
	// 맞은 플레이어 처리
	FVector ViewLoc;
	FRotator ViewRot;
	C->GetPlayerViewPoint(ViewLoc, ViewRot);
	
	/*
	[시작점: Start] ---------------------------------------------> [끝점: End]
	(카메라 위치)                  (방향 * 15,000cm)               (최대 사거리 지점)
	*/
	const FVector Start = ViewLoc;
	const FVector End   = Start + ViewRot.Vector() * WeaponRange; // ★ ViewRot.Vector()는 무조건 1
	
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(WeaponFire), true, this);
	const bool bHit = GetWorld()->LineTraceSingleByChannel( // ★ 충돌 검사
		Hit, Start, End, ECC_Visibility, Params);
	const FVector ImpactPoint = bHit ? Hit.ImpactPoint : End;
	
	if (bHit && Hit.GetActor())
	{
		UGameplayStatics::ApplyPointDamage(
			Hit.GetActor(), WeaponDamage, ViewRot.Vector(), Hit, C, this, nullptr);
	}
	DrawDebugLine(GetWorld(), Start, ImpactPoint, FColor::Yellow, false, 0.5f, 0, 1.f);
	if (bHit)
	{
		DrawDebugPoint(GetWorld(), ImpactPoint, 10.f, FColor::Red, false, 0.5f);
	}
	--CurrentAmmo;
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			1, 1.f, FColor::Green,
			FString::Printf(TEXT("Ammo %d / %d"), CurrentAmmo, ReserveAmmo));
	}
	// TODO : 몽타주 재생
}

void ADropCharacter::OnReloadPressed()
{
	Reload();
}

void ADropCharacter::Reload()
{
	if (bReloading || CurrentAmmo >= MagSize || ReserveAmmo <= 0) // 장전이 안되는 경우의 수
	{
		return;
	}
	bReloading= true;
	StopFire();
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, ReloadTime, FColor::Cyan, TEXT("Reloading..."));
	}
	GetWorldTimerManager().SetTimer(
		ReloadTimerHandle, this, &ADropCharacter::FinishReload, ReloadTime, false);
}

void ADropCharacter::FinishReload()
{
	const int32 Move = FMath::Min(MagSize - CurrentAmmo, ReserveAmmo);
	CurrentAmmo += Move;
	ReserveAmmo -= Move;
	bReloading = false;
	if (bFireHeld)
	{
		StartFire();
	}
}
