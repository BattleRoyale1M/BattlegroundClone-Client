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
	AProjectileBullet* NewBullet = SpawnPooledBullet(ProjectileClass, SpawnLocation, SpawnRotation);
	if (NewBullet)
	{
		NewBullet->ActivateBullet(SpawnLocation, SpawnRotation, Damage, Range, Owner, Instigator, WeaponName);
	}
	return NewBullet;
}

void UProjectilePoolSubsystem::WarmUpPool(TSubclassOf<AProjectileBullet> ProjectileClass, int32 PoolSize)
{
	if (!GetWorld() || !ProjectileClass) return;

	for (int32 i = 0; i < PoolSize; ++i)
	{
		if (AProjectileBullet* NewBullet = SpawnPooledBullet(ProjectileClass, FVector::ZeroVector, FRotator::ZeroRotator))
		{
			NewBullet->DeactivateBullet();
		}
	}
}

/*
풀생성시 파괴이벤트가 같이 연결
*/
AProjectileBullet* UProjectilePoolSubsystem::SpawnPooledBullet(TSubclassOf<AProjectileBullet> ProjectileClass,
	const FVector& SpawnLocation, const FRotator& SpawnRotation)
{
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
	AProjectileBullet* NewBullet = GetWorld()->SpawnActor<AProjectileBullet>(ProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);
	if (NewBullet)
	{
		NewBullet->OnDestroyed.AddDynamic(this, &UProjectilePoolSubsystem::HandleBulletDestroyed); // 핸들러 연결
		ProjectilePool.Add(NewBullet);
	}
	return NewBullet;
}

void UProjectilePoolSubsystem::HandleBulletDestroyed(AActor* DestroyedActor)
{
	ProjectilePool.RemoveSwap(Cast<AProjectileBullet>(DestroyedActor));
}

