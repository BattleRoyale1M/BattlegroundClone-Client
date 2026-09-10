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
	
	/*
	Weapon에서 스폰 직후 호출하여 데미지 및 사거리 주입 ★
	*/
	void InitBullet(float InDamage, float InRange);
	
protected:
	virtual void BeginPlay() override;
	
	/* 
	 총알 충돌체 
	*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> CollisionComp;
	
	/* 
	 투사체 이동컴포넌트
	*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;
	
	/* 
	 기본 데미지
	*/
	float Damage = 0.f;
	float Range = 0.f;
	
	/* 
	 사거리 계산을 위한 스폰 위치
	*/
	FVector SpawnLocation = FVector::ZeroVector;
	
	/* 
	 충돌 콜백 함수
	*/
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
};
