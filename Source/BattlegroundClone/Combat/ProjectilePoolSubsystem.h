#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Combat/ProjectileBullet.h"
#include "ProjectilePoolSubsystem.generated.h"

UCLASS()
class BATTLEGROUNDCLONE_API UProjectilePoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	/*
	풀에서 사용 가능한 총알을 가져오거나 없으면 확장하여 가져온다 
	*/
	AProjectileBullet* GetProjectile(TSubclassOf<AProjectileBullet> ProjectileClass, const FVector& SpawnLocation, const FRotator& SpawnRotation, float Damage, float Range, AActor* Owner, APawn* Instigator, const FText& WeaponName);
	
	/*
	 초기 지정 수량만큼 총알을 미리 스폰하여 풀 생성
	*/
	void WarmUpPool(TSubclassOf<AProjectileBullet> ProjectileClass, int32 PoolSize = 100);

protected:
	UPROPERTY()
	TArray<TObjectPtr<AProjectileBullet>> ProjectilePool;
};
