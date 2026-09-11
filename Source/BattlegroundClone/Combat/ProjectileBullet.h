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

	// 시각적 메시(3배 스케일)보다 판정이 너무 박하지 않도록 BP 디폴트에서 조정
	UPROPERTY(EditDefaultsOnly, Category = "Components", meta = (ClampMin = "0.0"))
	float CollisionRadius = 15.f;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> BulletMesh; // 총알 실체

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
