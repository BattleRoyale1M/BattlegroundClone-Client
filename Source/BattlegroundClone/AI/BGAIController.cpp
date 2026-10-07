#include "AI/BGAIController.h"

#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Sight.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Character/DropCharacter.h"
#include "Combat/HealthComponent.h"
#include "Weapon/WeaponInventoryComponent.h"

ABGAIController::ABGAIController()
{
	PrimaryActorTick.bCanEverTick = true;

	Perception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("Perception"));
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));

	SightConfig->SightRadius = 5000.f;
	SightConfig->LoseSightRadius = 6000.f;
	SightConfig->PeripheralVisionAngleDegrees = 70.f;
	SightConfig->SetMaxAge(5.f);
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

	Perception->ConfigureSense(*SightConfig);
	Perception->SetDominantSense(SightConfig->GetSenseImplementation());
}

void ABGAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	Perception->OnTargetPerceptionUpdated.AddUniqueDynamic(this, &ABGAIController::OnTargetPerceptionUpdated);

	if (ADropCharacter* Bot = GetBotCharacter())
	{
		if (UHealthComponent* Health = Bot->GetHealthComp())
		{
			Health->OnHit.AddUniqueDynamic(this, &ABGAIController::OnBotHit);
		}
	}
}

ADropCharacter* ABGAIController::GetBotCharacter() const
{
	return Cast<ADropCharacter>(GetPawn());
}

bool ABGAIController::IsValidTarget(const ADropCharacter* Candidate) const
{
	return Candidate
		&& Candidate != GetPawn()
		&& Candidate->GetHealthComp()
		&& !Candidate->GetHealthComp()->IsDead()
		&& Candidate->DropState == EDropState::Ground;
}

void ABGAIController::SetTarget(ADropCharacter* NewTarget)
{
	Target = NewTarget;
	
	ADropCharacter* Bot = GetBotCharacter();
	const bool bCombat = Target.IsValid();
	
	if (Bot)
	{
		Bot->bUseControllerRotationYaw = bCombat;
		Bot->GetCharacterMovement()->bOrientRotationToMovement = !bCombat;
	}
	if (bCombat)
	{
		SetFocus(NewTarget);
		FireCooldown = ReactionTime;
	}
	else
	{
		ClearFocus(EAIFocusPriority::Gameplay);
		if (Bot && Bot->GetWeaponInventory())
		{
			Bot->GetWeaponInventory()->StopFire();
		}
	}
}

void ABGAIController::PickNewTarget()
{
	TArray<AActor*> Seen;
	Perception->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), Seen);

	TArray<ADropCharacter*> Candidates;
	for (AActor* Actor : Seen)
	{
		ADropCharacter* Candidate = Cast<ADropCharacter>(Actor);
		if (IsValidTarget(Candidate))
		{
			Candidates.Add(Candidate);
		}
	}

	SetTarget(Candidates.Num() > 0 ? Candidates[FMath::RandRange(0, Candidates.Num() - 1)] : nullptr);
}

void ABGAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!bActivated)
	{
		return;
	}

	ADropCharacter* Seen = Cast<ADropCharacter>(Actor);
	if (!Seen)
	{
		return;
	}

	if (Stimulus.WasSuccessfullySensed())
	{
		if (!Target.IsValid() && IsValidTarget(Seen))
		{
			SetTarget(Seen);
		}
	}
	else if (Target.Get() == Seen)
	{
		PickNewTarget();
	}
}

void ABGAIController::OnBotHit(AController* InstigatorController, AActor* DamageCauser, FVector ShotDirection)
{
	if (!bActivated)
	{
		return;
	}

	if (!InstigatorController)
	{
		return;
	}

	ADropCharacter* Attacker = Cast<ADropCharacter>(InstigatorController->GetPawn());
	if (Attacker != Target.Get() && IsValidTarget(Attacker)) // 나를 때린 녀석이 내가 주시하고 있던 타겟이 아니고 && 적절한 타겟인지
	{
		SetTarget(Attacker); // 타겟대상으로 삼음
	}
}

FVector ABGAIController::GetFocalPointOnActor(const AActor* Actor) const
{
	return Super::GetFocalPointOnActor(Actor) + AimOffset;
}

void ABGAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ADropCharacter* Bot = GetBotCharacter();
	if (!Bot || !Bot->GetHealthComp())
	{
		return;
	}

	if (Bot->GetHealthComp()->IsDead())
	{
		StopMovement();
		SetTarget(nullptr);
		SetActorTickEnabled(false);
		return;
	}
	
	if (!bActivated)
	{
		if (bWaitForPlayerLanding && !HasAnyPlayerLanded())
		{
			return;
		}
		bActivated = true;
		PickNewTarget();
	}

	if (Target.IsValid() && !IsValidTarget(Target.Get()))
	{
		PickNewTarget();
	}

	if (!Target.IsValid())
	{
		Wander(DeltaTime);
		return;
	}

	const float Distance = FVector::Dist(Bot->GetActorLocation(), Target->GetActorLocation());
	if (Distance > EngageDistance)
	{
		if (GetMoveStatus() == EPathFollowingStatus::Idle)
		{
			MoveToActor(Target.Get(), EngageDistance * 0.8f);
		}
	}
	else if (GetMoveStatus() != EPathFollowingStatus::Idle)
	{
		StopMovement();
	}

	ShootTick(DeltaTime);
}

void ABGAIController::ShootTick(float DeltaTime)
{
	FireCooldown -= DeltaTime;
	if (FireCooldown > 0.f || !LineOfSightTo(Target.Get()))
	{
		return;
	}

	ADropCharacter* Bot = GetBotCharacter();
	UWeaponInventoryComponent* Inventory = Bot ? Bot->GetWeaponInventory() : nullptr;
	if (!Inventory)
	{
		return;
	}

	Inventory->StartFire();
	Inventory->StopFire();

	FireCooldown = FireInterval * FMath::FRandRange(0.8f, 1.5f);
	AimOffset = FMath::VRand() * FMath::FRandRange(0.f, AimError);
}

void ABGAIController::Wander(float DeltaTime)
{
	WanderCooldown -= DeltaTime;
	if (WanderCooldown > 0.f || GetMoveStatus() != EPathFollowingStatus::Idle)
	{
		return;
	}

	if (UNavigationSystemV1* Nav = UNavigationSystemV1::GetCurrent(GetWorld()))
	{
		FNavLocation Destination;
		if (Nav->GetRandomReachablePointInRadius(GetPawn()->GetActorLocation(), WanderRadius, Destination))
		{
			MoveToLocation(Destination.Location);
		}
	}

	WanderCooldown = FMath::FRandRange(2.f, 5.f);
}

bool ABGAIController::HasAnyPlayerLanded() const
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PC = It->Get();
		const ADropCharacter* Player = PC ? Cast<ADropCharacter>(PC->GetPawn()) : nullptr;
		if (Player && Player->bHasLanded)
		{
			return true;
		}
	}
	return false;
}

