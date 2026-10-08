#include "Weapon/WeaponBase.h"

#include "Net/UnrealNetwork.h"
#include "Engine/DamageEvents.h"
#include "Components/SceneCaptureComponent2D.h"
#include "GameFramework/DamageType.h"
#include "Engine/Engine.h"

#include "Combat/ProjectilePoolSubsystem.h"

#include "Weapon/WeaponUserInterface.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Components/PointLightComponent.h"

AWeaponBase::AWeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);
	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(RootScene);

	// 1인칭 스코프 카메라가 붙는 지점. 위치는 BP_WeaponBase 뷰포트에서 잡음.
	AimPoint = CreateDefaultSubobject<USceneComponent>(TEXT("AimPoint"));
	AimPoint->SetupAttachment(WeaponMesh, TEXT("Aim"));

	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetCollisionProfileName(TEXT("NoCollision"));
	WeaponMesh->SetGenerateOverlapEvents(false);
	
	bReplicates = true;
	SetReplicatingMovement(true);
}

void AWeaponBase::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AWeaponBase, CurrentAmmo);
	DOREPLIFETIME(AWeaponBase, ReserveAmmo);
	DOREPLIFETIME(AWeaponBase, bReloading);
	DOREPLIFETIME(AWeaponBase, CurrentFireMode);
}

void AWeaponBase::ServerStartFire_Implementation()
{
	StartFire();
}

void AWeaponBase::ServerStopFire_Implementation()
{
	StopFire();
}

void AWeaponBase::ServerSetFireMode_Implementation(EFireMode NewFireMode)
{
	SetFireMode(NewFireMode);
}

void AWeaponBase::ServerStartReload_Implementation()
{
	StartReload();
}

void AWeaponBase::BeginPlay()
{
	Super::BeginPlay();
	CurrentAmmo = MagSize;
}

void AWeaponBase::MulticastFireFX_Implementation(FVector TracerEnd, bool bHit)
{
	PlayFireFX();

	if (AActor* OwnerActor = GetOwner(); OwnerActor && OwnerActor->Implements<UWeaponUserInterface>())
	{
		IWeaponUserInterface::Execute_ReceiveWeaponRecoil(OwnerActor, RecoilPitch, RecoilYawRange, RecoilRecoverySpeed);
		IWeaponUserInterface::Execute_NotifyWeaponFired(OwnerActor);
	}
}

void AWeaponBase::MulticastReloadFX_Implementation(float Duration)
{
	if (AActor* OwnerActor = GetOwner(); OwnerActor && OwnerActor->Implements<UWeaponUserInterface>())
	{
		IWeaponUserInterface::Execute_NotifyWeaponReloadStarted(OwnerActor, Duration);
	}
}

void AWeaponBase::StartFire()
{
	if (!HasAuthority())
	{
		ServerStartFire();
		return;
	}
	if (WeaponType == EWeaponType::Melee)
	{
		if (AActor* OwnerActor = GetOwner(); OwnerActor && OwnerActor->Implements<UWeaponUserInterface>())
		{
			IWeaponUserInterface::Execute_RequestMeleeAttack(OwnerActor, AttackMontage);
		}
		MeleeTrace();
		return;
	}
	if (bReloading)
	{
		return;
	}
	bTriggerHeld = true;
	Fire();

	// Single: 방금 Fire()로 한 발 나갔으니 타이머 없이 종료. Automatic일 때만 연사 타이머를 건다.
	if (CurrentFireMode == EFireMode::Automatic && bTriggerHeld && CurrentAmmo > 0)
	{
		const float Interval = 60.f / FMath::Max(RoundsPerMinute, 1.f);
		// 사격 간격(Interval)마다 ADropCharacter 클래스의 Fire 함수를 계속(반복) 실행하도록 타이머를 ON
		GetWorldTimerManager().SetTimer(
			FireTimerHandle, this, &AWeaponBase::Fire, Interval, true);
	}
}

void AWeaponBase::SetFireMode(EFireMode NewFireMode)
{
	if (!HasAuthority())
	{
		ServerSetFireMode(NewFireMode);
		return;
	}
	CurrentFireMode = NewFireMode;
}

void AWeaponBase::SetScopeCaptureActive(bool bActive, USceneComponent* AimView)
{
	USceneCaptureComponent2D* Capture = FindComponentByClass<USceneCaptureComponent2D>();
	if (!Capture)
	{
		return;
	}
	if (bActive)
	{
		Capture->HiddenActors.Reset();
		Capture->HiddenActors.Add(this);
		if (AActor* OwnerActor = GetOwner())
		{
			Capture->HiddenActors.Add(OwnerActor);
		}
		if (AimView && !bScopeCaptureOnAimView)
		{
			ScopeCaptureHomeParent = Capture->GetAttachParent();
			ScopeCaptureHomeSocket = Capture->GetAttachSocketName();
			ScopeCaptureHomeRelative = Capture->GetRelativeTransform();
			Capture->AttachToComponent(AimView, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			bScopeCaptureOnAimView = true;
		}
	}
	else if (bScopeCaptureOnAimView)
	{
		if (USceneComponent* HomeParent = ScopeCaptureHomeParent.Get())
		{
			Capture->AttachToComponent(HomeParent, FAttachmentTransformRules::KeepRelativeTransform, ScopeCaptureHomeSocket);
			Capture->SetRelativeTransform(ScopeCaptureHomeRelative);
		}
		bScopeCaptureOnAimView = false;
	}
	Capture->bCaptureEveryFrame = bActive;
	Capture->SetActive(bActive);
	if (WeaponMesh)
	{
		WeaponMesh->SetScalarParameterValueOnMaterials(TEXT("ScopeActive"), bActive ? 1.f : 0.f);
	}
}

void AWeaponBase::ClientShowAmmoEmpty_Implementation()
{
	if (AActor* OwnerActor = GetOwner(); OwnerActor && OwnerActor->Implements<UWeaponUserInterface>())
	{
		IWeaponUserInterface::Execute_NotifyAmmoEmpty(OwnerActor);
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
		else
		{
			ClientShowAmmoEmpty();
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

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit, Start, End, ECC_Visibility, Params);

	FHitResult PawnHit;
	if (GetWorld()->LineTraceSingleByObjectType(PawnHit, Start, End, FCollisionObjectQueryParams(ECC_Pawn), Params)
		&& (!bHit || PawnHit.Distance < Hit.Distance))
	{
		Hit = PawnHit;
		bHit = true;
	}
	const FVector ImpactPoint = bHit ? Hit.ImpactPoint : End;
	
	const FVector MuzzleLocation = WeaponMesh ? WeaponMesh->GetSocketLocation(MuzzleSocketName) : GetActorLocation();
	const FRotator LaunchRotation = (ImpactPoint - MuzzleLocation).Rotation();
	
	/*
	서버 권한에서 오브젝트 풀을 통해 총알 발사
	*/
	if (HasAuthority() && ProjectileClass)
	{
		if (UProjectilePoolSubsystem* PoolSubsystem = GetWorld()->GetSubsystem<UProjectilePoolSubsystem>())
		{
			PoolSubsystem->GetProjectile(
				ProjectileClass,
				MuzzleLocation,
				LaunchRotation,
				Damage,
				Range,
				GetOwner(),
				Cast<APawn>(GetOwner()),
				WeaponDisplayName
			);
		}
	}

	--CurrentAmmo;

	MulticastFireFX(ImpactPoint, bHit);

	// TODO: 발사 몽타주
}

void AWeaponBase::MeleeTrace()
{
	AController* C = GetOwningController();
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!C || !OwnerPawn)
	{
		return;
	}

	FVector ViewLoc;
	FRotator ViewRot;
	C->GetPlayerViewPoint(ViewLoc, ViewRot);
	ViewRot.Pitch = 0.f;

	const FVector Start = OwnerPawn->GetActorLocation();
	const FVector End = Start + ViewRot.Vector() * Range;

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MeleeTrace), true, this);
	Params.AddIgnoredActor(GetOwner());

	const bool bHit = GetWorld()->SweepSingleByChannel(
		Hit, Start, End, FQuat::Identity, ECC_Pawn,
		FCollisionShape::MakeSphere(40.f), Params);

	if (!bHit)
	{
		return;
	}
	UGameplayStatics::ApplyPointDamage(
		Hit.GetActor(),
		Damage,
		Hit.ImpactNormal,
		Hit,
		GetOwningController(),
		this,
		UDamageType::StaticClass()
	);
}

void AWeaponBase::PlayFireFX()
{
	if (!WeaponMesh)
	{
		return;
	}

	// 소켓이 없으면 컴포넌트 원점에 붙임 (GetMuzzleLocation 폴백과 동일 정책)
	const bool bHasSocket = WeaponMesh->DoesSocketExist(MuzzleSocketName);
	const FName AttachPoint = bHasSocket ? MuzzleSocketName : NAME_None;

	if (MuzzleFlashFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAttached(
			MuzzleFlashFX, WeaponMesh, AttachPoint,
			FVector::ZeroVector, FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			/*bAutoDestroy=*/true, /*bAutoActivate=*/true);
	}

	if (FireSound)
	{
		UGameplayStatics::SpawnSoundAttached(
			FireSound, WeaponMesh, AttachPoint,
			FVector::ZeroVector, FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget, /*bStopWhenAttachedToDestroyed=*/false);
	}

	if (MuzzleLightIntensity > 0.f)
	{
		const FVector LightLoc = bHasSocket
			? WeaponMesh->GetSocketLocation(MuzzleSocketName)
			: WeaponMesh->GetComponentLocation();

		UPointLightComponent* Light = NewObject<UPointLightComponent>(this);
		Light->SetMobility(EComponentMobility::Movable);
		Light->SetIntensity(MuzzleLightIntensity);
		Light->SetLightColor(MuzzleLightColor);
		Light->SetAttenuationRadius(MuzzleLightRadius);
		Light->SetCastShadows(false);
		Light->RegisterComponent();
		Light->AttachToComponent(
			WeaponMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, AttachPoint);
		Light->SetWorldLocation(LightLoc);

		// 짧게 번쩍이고 스스로 정리
		FTimerHandle LightTimer;
		TWeakObjectPtr<UPointLightComponent> WeakLight(Light);
		GetWorldTimerManager().SetTimer(LightTimer, [WeakLight]()
		{
			if (WeakLight.IsValid())
			{
				WeakLight->DestroyComponent();
			}
		}, FMath::Max(MuzzleLightFadeTime, 0.01f), false);
	}
}

void AWeaponBase::StopFire()
{
	if (!HasAuthority())
	{
		ServerStopFire();
		return;
	}
	bTriggerHeld = false;
	GetWorldTimerManager().ClearTimer(FireTimerHandle);
}

void AWeaponBase::StartReload()
{
	if (!HasAuthority())
	{
		ServerStartReload();
		return;
	}
	if (bReloading || CurrentAmmo >= MagSize || ReserveAmmo <= 0)
	{
		return;
	}
	bReloading = true;
	MulticastReloadFX(ReloadTime); // 서버 <-> 클라 동기화용
	OnReloadStarted.Broadcast(ReloadTime); // BP/UI용 이벤트
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

