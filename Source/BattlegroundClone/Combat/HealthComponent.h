#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChanged, float, Health, float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDeath, AController*, Killer, AActor*, DamageCauser);

UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class BATTLEGROUNDCLONE_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHealthComponent();
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	void ApplyDamage(float Amount, AController* Instigator, AActor* DamageCauser);
	
	UFUNCTION(BlueprintPure, Category = "Health") float GetHealth() const { return Health; }
	UFUNCTION(BlueprintPure, Category = "Health") float GetMaxHealth() const { return MaxHealth; }
	UFUNCTION(BlueprintPure, Category = "Health") float GetHealthPercent() const { return MaxHealth > 0.f ? Health / MaxHealth : 0.f; }
	UFUNCTION(BlueprintPure, Category = "Health") bool IsDead() const { return bDead; }
	
	/*
	UI
	*/
	UPROPERTY(BlueprintAssignable, Category = "Health") FOnHealthChanged OnHealthChanged;
	UPROPERTY(BlueprintAssignable, Category = "Health") FOnDeath OnDeath;
	
protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(EditDefaultsOnly, Category = "Health")
	float MaxHealth = 100.f;
	
	UPROPERTY(ReplicatedUsing=OnRep_Health)
	float Health = 100.f;
	
	bool bDead = false;
	
	UFUNCTION()
	void OnRep_Health();
};
