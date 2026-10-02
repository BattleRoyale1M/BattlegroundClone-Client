#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChanged, float, Health, float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDeath, AController*, Killer, AActor*, DamageCauser);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnHit, AController*, InstigatorController, AActor*, DamageCauser, FVector, ShotDirection);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBoostChanged, float, Boost);

UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class BATTLEGROUNDCLONE_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHealthComponent();
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	void ApplyDamage(float Amount, AController* Instigator, AActor* DamageCauser, const FVector& ShotDirection);
	
	void Heal(float Amount, float Cap); 
	void AddBoost(float Amount);
	UFUNCTION(BlueprintPure, Category = "Health") float GetBoost() const { return Boost; }
	UPROPERTY(BlueprintAssignable, Category = "Health") FOnBoostChanged OnBoostChanged;

	UFUNCTION(BlueprintPure, Category = "Health") float GetHealth() const { return Health; }
	UFUNCTION(BlueprintPure, Category = "Health") float GetMaxHealth() const { return MaxHealth; }
	UFUNCTION(BlueprintPure, Category = "Health") float GetHealthPercent() const { return MaxHealth > 0.f ? Health / MaxHealth : 0.f; }
	UFUNCTION(BlueprintPure, Category = "Health") bool IsDead() const { return bDead; }
	
	/*
	UI
	*/
	UPROPERTY(BlueprintAssignable, Category = "Health") FOnHealthChanged OnHealthChanged;
	UPROPERTY(BlueprintAssignable, Category = "Health") FOnDeath OnDeath;
	UPROPERTY(BlueprintAssignable, Category = "Health") FOnHit OnHit;
	
protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(EditDefaultsOnly, Category = "Health")
	float MaxHealth = 100.f;
	
	UPROPERTY(ReplicatedUsing=OnRep_Health)
	float Health = 100.f;
	
	UPROPERTY(Replicated)
	bool bDead = false;

	UFUNCTION()
	void OnRep_Health();


	UPROPERTY(ReplicatedUsing = OnRep_Boost)
	float Boost = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Health|Boost")
	float MaxBoost = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "Health|Boost")
	float BoostTickInterval = 2.f; 

	UPROPERTY(EditDefaultsOnly, Category = "Health|Boost")
	float BoostDecayPerTick = 4.f;

	UPROPERTY(EditDefaultsOnly, Category = "Health|Boost")
	float BoostHealPerTick = 1.f;

	FTimerHandle BoostTimer;
	void TickBoost();

	UFUNCTION()
	void OnRep_Boost();

};
