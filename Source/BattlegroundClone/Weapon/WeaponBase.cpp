#include "Weapon/WeaponBase.h"

#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "TimerManager.h"

AWeaponBase::AWeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);
	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(RootScene);
	
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetCollisionProfileName(TEXT("NoCollision"));
	WeaponMesh->SetGenerateOverlapEvents(false);
}

void AWeaponBase::BeginPlay()
{
	Super::BeginPlay();
	CurrentAmmo = MagSize;
}

void AWeaponBase::StartFire()
{
	if (bReloading)
	{
		return;
	}
	bTriggerHeld = true;
	Fire();
	
	if (bTriggerHeld && CurrentAmmo > 0)
	{
		const float Interval = 60.f / FMath::Max(RoundsPerMinute, 1.f);
		// 사격 간격(Interval)마다 ADropCharacter 클래스의 Fire 함수를 계속(반복) 실행하도록 타이머를 ON
		GetWorldTimerManager().SetTimer(
			FireTimerHandle, this, &AWeaponBase::Fire, Interval, true);
	}
}

void AWeaponBase::Fire()
{
	if (bReloading)
	{
		return;
	}
	if (CurrentAmmo <= 0)
	{
		StopFire();
		if (ReserveAmmo > 0)
		{
			StartReload();
		}
		else if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, TEXT("*click*"));
		}
		return;
	}

	FVector Start, End;
	FRotator ViewRot;
	if (!GetAimTrace(Start, End, ViewRot))
	{
		return;
	}

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(WeaponFire), true, this);
	Params.AddIgnoredActor(GetOwner());

	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit, Start, End, ECC_Visibility, Params);
	const FVector ImpactPoint = bHit ? Hit.ImpactPoint : End;

	if (bHit && Hit.GetActor())
	{
		UGameplayStatics::ApplyPointDamage(
			Hit.GetActor(), Damage, ViewRot.Vector(), Hit,
			GetOwningController(), this, nullptr);
	}
	
	const FVector MuzzleLoc = GetMuzzleLocation();
	DrawDebugLine(GetWorld(), MuzzleLoc, ImpactPoint, FColor::Yellow, false, 0.5f, 0, 1.f);
	if (bHit)
	{
		DrawDebugPoint(GetWorld(), ImpactPoint, 10.f, FColor::Red, false, 0.5f);
	}

	--CurrentAmmo;
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			1, 1.f, FColor::Green,
			FString::Printf(TEXT("Ammo %d / %d"), CurrentAmmo, ReserveAmmo));
	}

	// TODO: 머즐플래시 파티클 / 사격 사운드 / 발사 몽타주
}

void AWeaponBase::StopFire()
{
	bTriggerHeld = false;
	GetWorldTimerManager().ClearTimer(FireTimerHandle);
}

void AWeaponBase::StartReload()
{
	if (bReloading || CurrentAmmo >= MagSize || ReserveAmmo <= 0)
	{
		return;
	}
	bReloading = true;
	StopFire();
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, ReloadTime, FColor::Cyan, TEXT("Reloading..."));
	}
	GetWorldTimerManager().SetTimer(
		ReloadTimerHandle, this, &AWeaponBase::FinishReload, ReloadTime, false);
}

void AWeaponBase::FinishReload()
{
	const int32 Moved = FMath::Min(MagSize - CurrentAmmo, ReserveAmmo);
	CurrentAmmo += Moved;
	ReserveAmmo -= Moved;
	bReloading = false;

	if (bTriggerHeld)
	{
		StartFire();
	}
}

FVector AWeaponBase::GetMuzzleLocation() const
{
	if (WeaponMesh && WeaponMesh->DoesSocketExist(MuzzleSocketName))
	{
		return WeaponMesh->GetSocketLocation(MuzzleSocketName);
	}
	return WeaponMesh ? WeaponMesh->GetComponentLocation() : GetActorLocation();
}

AController* AWeaponBase::GetOwningController() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	return OwnerPawn ? OwnerPawn->GetController() : nullptr;
}

bool AWeaponBase::GetAimTrace(FVector& OutStart, FVector& OutEnd, FRotator& OutViewRot) const
{
	AController* C = GetOwningController();
	if (!C)
	{
		return false;
	}
	FVector ViewLoc;
	C->GetPlayerViewPoint(ViewLoc, OutViewRot);
	OutStart = ViewLoc;
	OutEnd   = ViewLoc + OutViewRot.Vector() * Range;
	return true;
}

