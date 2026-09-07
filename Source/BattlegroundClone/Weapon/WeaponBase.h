#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponBase.generated.h"
class UStaticMeshComponent;

UCLASS()
class BATTLEGROUNDCLONE_API AWeaponBase : public AActor
{
	GENERATED_BODY()
	
public:	
	AWeaponBase();
	
	void StartFire();
	void StopFire();
	void StartReload();
	
	UFUNCTION(BlueprintCallable, Category="Weapon")
	FVector GetMuzzleLocation() const;
	UFUNCTION(BlueprintPure, Category = "Weapon")
	int32 GetCurrentAmmo() const { return CurrentAmmo; }
	UFUNCTION(BlueprintPure, Category = "Weapon")
	int32 GetReserveAmmo() const { return ReserveAmmo; }

protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;
	
	// --- Stats : 무기별로 지정 ------------------------------
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
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Stats")
	int32 ReserveAmmo = 90;
	
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FName MuzzleSocketName = TEXT("Muzzle");
	
	//  --- Runtime state -------------------------------------
	int32 CurrentAmmo = 0;
	bool  bReloading   = false;
	bool  bTriggerHeld = false;
	FTimerHandle FireTimerHandle;
	FTimerHandle ReloadTimerHandle;
	
	void Fire();
	void FinishReload();
	AController* GetOwningController() const;
	bool GetAimTrace(FVector& OutStart, FVector& OutEnd, FRotator& OutViewRot) const;
};

/*
WeaponMesh가 루트가 되어 무기 액터를 캐릭터 손 소켓에 붙이면 메시가 딸려옴
스펙은 EditDefaultsOnly로 지정해 BP_AR4, BP_KA47 같은 자식 BP에서 값만 바꿔 무기 종류를 만듬 (C++ 재컴파일 필요x)
조준 트레이스는 무기가 "소유 폰 컨트롤러"에서 시점을 얻어 처리 → 판정은 여전히 카메라 기준. 트레이서 그리기만 GetMuzzleLocation()에서 시작. 
*/