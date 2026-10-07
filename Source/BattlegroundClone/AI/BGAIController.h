#pragma once
#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "BGAIController.generated.h"
class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class ADropCharacter;

UCLASS()
class BATTLEGROUNDCLONE_API ABGAIController : public AAIController
{
	GENERATED_BODY()

public:
	ABGAIController();
	virtual void Tick(float DeltaTime) override;
	virtual FVector GetFocalPointOnActor(const AActor* Actor) const override;
	
protected:
	virtual void OnPossess(APawn* InPawn) override;
	
	UPROPERTY(VisibleAnywhere, Category = "AI")
	TObjectPtr<UAIPerceptionComponent> Perception;
	
	UPROPERTY()
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float EngageDistance = 1500.f;

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float FireInterval = 0.4f;

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float ReactionTime = 0.6f;

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float AimError = 60.f;

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float WanderRadius = 2000.f;

	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	UFUNCTION()
	void OnBotHit(AController* InstigatorController, AActor* DamageCauser, FVector ShotDirection);
	
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	bool bWaitForPlayerLanding = true;

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float BotEngageRange = 1200.f;

private:
	TWeakObjectPtr<ADropCharacter> Target;
	FVector AimOffset = FVector::ZeroVector;
	float FireCooldown = 0.f;
	float WanderCooldown = 0.f;

	ADropCharacter* GetBotCharacter() const;
	bool IsValidTarget(const ADropCharacter* Candidate) const;
	void SetTarget(ADropCharacter* NewTarget);
	void PickNewTarget();
	void Wander(float DeltaTime);
	void ShootTick(float DeltaTime);
	
	bool bActivated = false;
	bool HasAnyPlayerLanded() const;
};
