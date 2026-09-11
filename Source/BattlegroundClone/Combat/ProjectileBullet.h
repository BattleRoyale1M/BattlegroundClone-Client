#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectileBullet.generated.h"

class USphereComponent;
class UProjectileMovementComponent;

UCLASS()
class BATTLEGROUNDCLONE_API AProjectileBullet : public AActor
{
	GENERATED_BODY()

public:	
	AProjectileBullet();
	
	void ActivateBullet(const FVector& SpawnLocation, const FRotator& SpawnRotation, float InDamage, float InRange, AActor* InOwner, APawn* InInstigator);
	
	void DeactivateBullet();

	bool IsInUse() const { return bInUse; }
	
protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> CollisionComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	float Damage = 0.f;
	float Range = 0.f;
	FVector SpawnLocation = FVector::ZeroVector;
	bool bInUse = false;
	
	/* 
	 충돌 콜백 함수
	*/
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
};
