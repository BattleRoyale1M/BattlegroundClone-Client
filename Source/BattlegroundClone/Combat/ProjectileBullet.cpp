#include "Combat/ProjectileBullet.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DamageEvents.h"

AProjectileBullet::AProjectileBullet()
{
 	
	PrimaryActorTick.bCanEverTick = false;
	
	bReplicates = true;
	
	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	CollisionComp -> InitSphereRadius(5.0f);
	CollisionComp -> SetCollisionProfileName(TEXT("Projectile")); // BlockAllDynamic / Custom Profile
	CollisionComp -> BodyInstance.bUseCCD = true;
	CollisionComp -> OnComponentHit.AddDynamic(this, &AProjectileBullet::OnHit);
	
	RootComponent = CollisionComp;
	
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
	ProjectileMovement -> UpdatedComponent = CollisionComp;
	ProjectileMovement -> InitialSpeed = 10000.f;
	ProjectileMovement -> MaxSpeed = 10000.f;
	ProjectileMovement -> bRotationFollowsVelocity = true;
	ProjectileMovement -> bShouldBounce  = false;
	ProjectileMovement -> ProjectileGravityScale = 0.5f;

}

void AProjectileBullet::BeginPlay()
{
	Super::BeginPlay();
	
	if (GetOwner())
	{
		CollisionComp -> IgnoreActorWhenMoving(GetOwner(), true);
	}
}

void AProjectileBullet::InitBullet(float InDamage, float InRange)
{
	Damage = InDamage;
	Range = InRange;
	SpawnLocation = GetActorLocation();
}

void AProjectileBullet::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	if (!HasAuthority())
	{
		if (Range > 0.f)
		{
			const float DistanceTraveled = FVector::Dist(SpawnLocation, Hit.ImpactPoint);
			if (DistanceTraveled > Range) // 이동거리
			{
				Destroy();
				return;
			}
		}
		
		// 발사한 캐릭터 본인과의 충돌은 무시되도록 처리
		AActor* MyOwner = GetOwner();
		if (OtherActor && OtherActor != this && OtherActor != MyOwner)
		{
			AController* MyOwnerInstigator = MyOwner ? MyOwner->GetInstigatorController() : nullptr;
			
			// https://meo-young.tistory.com/159
			UGameplayStatics::ApplyPointDamage(
				OtherActor,
				Damage,
				Hit.ImpactNormal,
				Hit,
				MyOwnerInstigator,
				this,
				UDamageType::StaticClass()
			);
		}
		Destroy();
	}
}

