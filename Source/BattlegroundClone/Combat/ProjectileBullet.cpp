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
	CollisionComp -> SetCollisionProfileName(TEXT("BlockAllDynamic")); // BlockAllDynamic / Custom Profile
	CollisionComp -> BodyInstance.bUseCCD = true;
	CollisionComp -> OnComponentHit.AddDynamic(this, &AProjectileBullet::OnHit);
	
	RootComponent = CollisionComp;
	
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
	ProjectileMovement -> UpdatedComponent = CollisionComp;
	ProjectileMovement -> InitialSpeed = 80000.f;
	ProjectileMovement -> MaxSpeed = 80000.f;
	ProjectileMovement -> bRotationFollowsVelocity = true;
	ProjectileMovement -> bShouldBounce  = false;
	ProjectileMovement -> ProjectileGravityScale = 0.5f;
	
	BulletMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BulletMesh"));
	BulletMesh->SetupAttachment(CollisionComp);
	BulletMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BulletMesh->SetGenerateOverlapEvents(false);
	
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);
	if (ProjectileMovement)
	{
		ProjectileMovement -> Deactivate();
	}

}

void AProjectileBullet::BeginPlay()
{
	Super::BeginPlay();
}

// ★
void AProjectileBullet::ActivateBullet(const FVector& InSpawnLocation, const FRotator& InSpawnRotation, float InDamage, float InRange, AActor* InOwner, APawn* InInstigator, const FText& InWeaponName)
{
	bInUse = true;
	Damage = InDamage;
	Range = InRange; // range 넘어서면 비활성화
	WeaponDisplayName = InWeaponName;
	SpawnLocation = InSpawnLocation;
	
	SetOwner(InOwner);
	SetInstigator(InInstigator);
	
	CollisionComp->ClearMoveIgnoreActors();
	
	SetActorLocationAndRotation(InSpawnLocation, InSpawnRotation);
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	
	if (InOwner)
	{
		CollisionComp ->IgnoreActorWhenMoving(InOwner, true);
	}
	if (ProjectileMovement)
	{
		ProjectileMovement->Activate(true);
		ProjectileMovement->Velocity = InSpawnRotation.Vector() * ProjectileMovement->InitialSpeed;
	}
}

void AProjectileBullet::DeactivateBullet()
{
	bInUse = false;
	
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	
	if (ProjectileMovement)
	{
		ProjectileMovement -> StopMovementImmediately();
		ProjectileMovement -> Deactivate();
	}
}



void AProjectileBullet::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	if (HasAuthority())
	{
		if (Range > 0.f)
		{
			const float DistanceTraveled = FVector::Dist(SpawnLocation, Hit.ImpactPoint);
			if (DistanceTraveled > Range) // 이동거리
			{
				DeactivateBullet();
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
		DeactivateBullet();
	}
}

