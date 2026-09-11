#include "Combat/ProjectilePoolSubsystem.h"
#include "Engine/World.h"

AProjectileBullet* UProjectilePoolSubsystem::GetProjectile(TSubclassOf<AProjectileBullet> ProjectileClass,
	const FVector& SpawnLocation, const FRotator& SpawnRotation, float Damage, float Range, AActor* Owner,
	APawn* Instigator, const FText& WeaponName)
{
	UWorld* World = GetWorld();
	if (!World || !ProjectileClass) return nullptr;

	// 1. 기존 풀에서 비활성화 된 총알 찾기
	for (AProjectileBullet* Bullet : ProjectilePool)
	{
		if (IsValid(Bullet) && !Bullet->IsInUse())
		{
			Bullet->ActivateBullet(SpawnLocation, SpawnRotation, Damage, Range, Owner, Instigator, WeaponName);
			return Bullet;
		}
	}

	// 2. 풀에 이용 가능한 총알이 없으면 새로 동적 생성(Pool Expansion) 후 활성화
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AProjectileBullet* NewBullet = World->SpawnActor<AProjectileBullet>(ProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);
	if (NewBullet)
	{
		ProjectilePool.Add(NewBullet);
		NewBullet->ActivateBullet(SpawnLocation, SpawnRotation, Damage, Range, Owner, Instigator, WeaponName);
		return NewBullet;
	}

	return nullptr;
}

void UProjectilePoolSubsystem::WarmUpPool(TSubclassOf<AProjectileBullet> ProjectileClass, int32 PoolSize)
{
	UWorld* World = GetWorld();
	if (!World || !ProjectileClass) return;
	
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
	for (int32 i = 0; i < PoolSize; ++i) 
	{
		AProjectileBullet* NewBullet = GetWorld()->SpawnActor<AProjectileBullet>(ProjectileClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	
		if (NewBullet)
		{
			NewBullet->DeactivateBullet();
			ProjectilePool.Add(NewBullet);
		}
	}
}
