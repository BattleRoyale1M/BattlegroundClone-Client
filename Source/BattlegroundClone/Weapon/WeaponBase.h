#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/Enums/EFireMode.h"
#include "WeaponBase.generated.h"

class UStaticMeshComponent;
class USceneComponent;
class UNiagaraSystem;
class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnReloadStarted, float, Duration);

UCLASS()
class BATTLEGROUNDCLONE_API AWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	AWeaponBase();
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(Server, Reliable)
	void ServerStartFire();
	UFUNCTION(Server, Reliable)
	void ServerStopFire();
	UFUNCTION(Server, Reliable)
	void ServerSetFireMode(EFireMode NewFireMode);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastFireFX(FVector TracerEnd, bool bHit);
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastReloadFX(float Duration);

	void StartFire();
	void StopFire();
	void StartReload();
	void SetFireMode(EFireMode NewFireMode);

	UFUNCTION(BlueprintPure, Category = "Weapon")
	EFireMode GetFireMode() const
	{
		return CurrentFireMode;
	}

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	FVector GetMuzzleLocation() const;

	UStaticMeshComponent* GetWeaponMesh() const { return WeaponMesh; }

	// 1인칭 스코프에서 카메라를 붙일 조준점. BP_WeaponBase 뷰포트에서 가늠자 뒤로 위치시킴.
	USceneComponent* GetAimPoint() const { return AimPoint; }

	UFUNCTION(BlueprintPure, Category = "Weapon")
	int32 GetCurrentAmmo() const { return CurrentAmmo; }

	UFUNCTION(BlueprintPure, Category = "Weapon")
	int32 GetReserveAmmo() const { return ReserveAmmo; }

	/*
	RPC Server
	*/
	UFUNCTION(Server, Reliable)
	void ServerStartReload();

	// 장전 시작 이벤트 (BP/UI용). 몽타주 재생은 MulticastReloadFX가 담당.
	UPROPERTY(BlueprintAssignable, Category = "Weapon")
	FOnReloadStarted OnReloadStarted;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<USceneComponent> RootScene;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	// 1인칭 스코프 카메라 부착점. BP_WeaponBase 에서 가늠자 뒤 조준선 위로 옮길 것.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<USceneComponent> AimPoint;

	// --- Stats : 무기별로 지정 ---------------------------------
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Stats")
	FText WeaponDisplayName = FText::FromString(TEXT("Weapon"));

	UFUNCTION(BlueprintPure, Category = "Weapon")
	FText GetWeaponDisplayName() const
	{
		return WeaponDisplayName;
	}
	
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Stats")
	int32 MagSize = 30;
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Stats")
	float RoundsPerMinute = 600.f;
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Stats")
	float ReloadTime = 2.2f;
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Stats")
	float Range = 15000.f;
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Stats")
	float Damage = 25.f;
	UPROPERTY(EditDefaultsOnly, Replicated)
	int32 ReserveAmmo = 90;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FName MuzzleSocketName = TEXT("Muzzle");

	// --- FX : 무기별로 지정 -----------------------------------
	// 총구 소켓(MuzzleSocketName)에 붙여 원샷 재생. NS_MuzzleFlash 지정.
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|FX")
	TObjectPtr<UNiagaraSystem> MuzzleFlashFX;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon|FX")
	TObjectPtr<USoundBase> FireSound;

	// 총구 섬광 라이트(0이면 스폰 안 함)
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|FX", meta = (ClampMin = "0.0"))
	float MuzzleLightIntensity = 1500.f;
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|FX")
	FLinearColor MuzzleLightColor = FLinearColor(1.f, 0.72f, 0.36f);
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|FX", meta = (ClampMin = "0.0"))
	float MuzzleLightRadius = 300.f;
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|FX", meta = (ClampMin = "0.0"))
	float MuzzleLightFadeTime = 0.05f;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Stats")
	TSubclassOf<class AProjectileBullet> ProjectileClass;
	
	// --- Runtime state ---------------------------------------
	UPROPERTY(Replicated)
	int32 CurrentAmmo = 0;
	
	UPROPERTY(Replicated)
	bool bReloading = false;
	bool bTriggerHeld = false; // only Server

	// B키로 캐릭터에서 토글되는 사격 모드. 서버가 권위를 가지며 StartFire()의 연사 타이머 여부를 결정.
	UPROPERTY(Replicated)
	EFireMode CurrentFireMode = EFireMode::Single;

	FTimerHandle FireTimerHandle;
	FTimerHandle ReloadTimerHandle;

	UFUNCTION(Client, Reliable)
	void ClientShowAmmoEmpty();
	
	void Fire();
	void FinishReload();
	void PlayFireFX();
	AController* GetOwningController() const;
	bool GetAimTrace(FVector& OutStart, FVector& OutEnd, FRotator& OutViewRot) const;
};

/*
RootScene가 루트. WeaponMesh는 그 자식이라 BP에서 Location/Rotation 조정 가능 (손 소켓 정렬용). 무기 액터를 캐릭터 손 소켓에 붙이면 메시가 딸려옴
스펙은 EditDefaultsOnly로 지정해 BP_AR4, BP_KA47 같은 자식 BP에서 값만 바꿔 무기 종류를 만듬 (C++ 재컴파일 필요x)
조준 트레이스는 무기가 "소유 폰 컨트롤러"에서 시점을 얻어 처리 → 판정은 여전히 카메라 기준. 트레이서 그리기만 GetMuzzleLocation()에서 시작.
*/
