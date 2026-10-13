#include "Character/CMC/DropMovementComponent.h"
#include "Character/DropCharacter.h"
#include "GameFramework/Character.h"

ADropCharacter* UDropMovementComponent::GetDropOwner() const
{
	return Cast<ADropCharacter>(CharacterOwner);
}

void UDropMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);
	bWantsFastFall = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;
	bWantsDeploy = (Flags & FSavedMove_Character::FLAG_Custom_1) != 0;
}

FNetworkPredictionData_Client* UDropMovementComponent::GetPredictionData_Client() const
{
	if (!ClientPredictionData)
	{
		UDropMovementComponent* Mutable = const_cast<UDropMovementComponent*>(this);
		Mutable->ClientPredictionData = new FNetworkPredictionData_Client_Drop(*this);
	}
	return ClientPredictionData;
}

void UDropMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
	if (!IsInCustomMode(CMOVE_Freefall))
	{
		bWantsDeploy = false;
		bWantsFastFall = false;
	}
}

void UDropMovementComponent::PhysCustom(float DeltaTime, int32 Iterations)
{
	if (DeltaTime < MIN_TICK_TIME)
	{
		return;
	}
	ADropCharacter* C = GetDropOwner();
	if (!C)
	{
		return;
	}
	if (CharacterOwner->GetLocalRole() != ROLE_SimulatedProxy)
	{
		if (CustomMovementMode == CMOVE_Freefall)
        	{
        		if (bWantsDeploy || C->GroundDistance() <= C->AutoDeployHeight)
        		{
        			SetMovementMode(MOVE_Custom, CMOVE_Parachute);
        		}
        		else
        		{
        			FVector Desired = C->GetControlRotation().Vector() * 800.f;
        			Desired.Z = -(bWantsFastFall ? C->FreefallMaxSpeed : C->FreefallMinSpeed);
        			Velocity = FMath::VInterpTo(Velocity, Desired, DeltaTime, C->FreefallAccel);
        		}
        	}
        	if (CustomMovementMode == CMOVE_Parachute)
        	{
        		if (C->GroundDistance() <= C->LandHeight)
        		{
        			SetMovementMode(MOVE_Falling);
        			StartNewPhysics(DeltaTime, Iterations);
        			return;
        		}
        		const FVector Forward = FRotationMatrix(FRotator(0.f, C->GetControlRotation().Yaw, 0.f)).GetUnitAxis(EAxis::X);
        		FVector Desired = Forward * C->ParachuteForwardSpeed;
        		Desired.Z = -C->ParachuteDescentSpeed;
        		Velocity = FMath::VInterpTo(Velocity, Desired, DeltaTime, 2.f);
        	}
	}
	Iterations++;
	
	const FVector Delta = Velocity * DeltaTime;
	FHitResult Hit;
	SafeMoveUpdatedComponent(Delta, UpdatedComponent->GetComponentQuat(), true, Hit);
	if (Hit.IsValidBlockingHit())
	{
		SlideAlongSurface(Delta, 1.f - Hit.Time, Hit.Normal, Hit, true);
	}
}

void FSavedMove_Drop::Clear()
{
	Super::Clear();
	bSavedWantsFastFall = false;
	bSavedWantsDeploy = false;
}

uint8 FSavedMove_Drop::GetCompressedFlags() const
{
	uint8 Result = Super::GetCompressedFlags();
	if (bSavedWantsFastFall) Result |= FLAG_Custom_0;
	if (bSavedWantsDeploy) Result |= FLAG_Custom_1;
	return Result;
}

bool FSavedMove_Drop::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const
{
	const FSavedMove_Drop* Other = static_cast<const FSavedMove_Drop*>(NewMove.Get());
	if (bSavedWantsFastFall != Other->bSavedWantsFastFall || bSavedWantsDeploy != Other->bSavedWantsDeploy)
	{
		return false;
	}
	return Super::CanCombineWith(NewMove, InCharacter, MaxDelta);
}

void FSavedMove_Drop::SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData)
{
	Super::SetMoveFor(C, InDeltaTime, NewAccel, ClientData);
	if (const UDropMovementComponent* M = Cast<UDropMovementComponent>(C->GetCharacterMovement()))
	{
		bSavedWantsFastFall = M->bWantsFastFall;
		bSavedWantsDeploy = M->bWantsDeploy;
	}
}

void FSavedMove_Drop::PrepMoveFor(ACharacter* C)
{
	Super::PrepMoveFor(C);
	if (UDropMovementComponent* M = Cast<UDropMovementComponent>(C->GetCharacterMovement()))
	{
		M->bWantsFastFall = bSavedWantsFastFall;
		M->bWantsDeploy = bSavedWantsDeploy;
	}
}
