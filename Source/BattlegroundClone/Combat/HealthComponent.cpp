#include "Combat/HealthComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Engine/World.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// 서버에서 초기 체력 설정
	if (GetOwner() && GetOwner() -> HasAuthority())
	{
		Health = MaxHealth;
	}
	OnHealthChanged.Broadcast(Health, MaxHealth);
}

void UHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	/*
	등록
	*/
	DOREPLIFETIME_CONDITION_NOTIFY(UHealthComponent, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME(UHealthComponent, bDead);
	DOREPLIFETIME(UHealthComponent, Boost);
}

void UHealthComponent::ApplyDamage(float Amount, AController* Instigator, AActor* DamageCauser, const FVector& ShotDirection)
{
	if (bDead || Amount <= 0.f || !GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	
	/*
	체력을 두 값 사이로 제한
	*/
	Health = FMath::Clamp(Health - Amount, 0.f, MaxHealth);
	OnHealthChanged.Broadcast(Health, MaxHealth);
	
	if (Health <= 0.f)
	{
		bDead = true;
		OnDeath.Broadcast(Instigator, DamageCauser); // [서버] 이벤트 발생
	}
	else
	{
		OnHit.Broadcast(Instigator, DamageCauser, ShotDirection);
	}
}

void UHealthComponent::Heal(float Amount, float Cap)
{
	if (bDead || Amount <= 0.f || !GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	const float Limit = FMath::Min(Cap, MaxHealth);
	if (Health >= Limit)
	{
		return;
	}
	Health = FMath::Min(Health + Amount, Limit);
	OnHealthChanged.Broadcast(Health, MaxHealth);
}

void UHealthComponent::AddBoost(float Amount)
{
	if (bDead || Amount <= 0.f || !GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	Boost = FMath::Clamp(Boost + Amount, 0.f, MaxBoost);
	OnBoostChanged.Broadcast(Boost);
	FTimerManager& TM = GetWorld()->GetTimerManager();
	if (!TM.IsTimerActive(BoostTimer))
	{
		TM.SetTimer(BoostTimer, this, &UHealthComponent::TickBoost, BoostTickInterval, true);
	}
}

void UHealthComponent::TickBoost()
{
	if (bDead || Boost <= 0.f)
	{
		Boost = 0.f;
		GetWorld()->GetTimerManager().ClearTimer(BoostTimer);
		OnBoostChanged.Broadcast(Boost);
		return;
	}
	Boost = FMath::Max(0.f, Boost - BoostDecayPerTick);
	Heal(BoostHealPerTick, MaxHealth);
	OnBoostChanged.Broadcast(Boost);
}

void UHealthComponent::OnRep_Boost()
{
	OnBoostChanged.Broadcast(Boost);
}

void UHealthComponent::OnRep_Health()
{
	OnHealthChanged.Broadcast(Health, MaxHealth);
	if (Health <= 0.f && !bDead)
	{
		bDead = true;
		OnDeath.Broadcast(nullptr, nullptr); // [클라이언트] 이벤트 발생
	}
}

void UHealthComponent::OnRep_Dead()
{
	// TODO : 다음 단계에서 사망 연출 트리거용
}

/*
첫 번째 함수와 두 번째 함수의 로직이 거의 똑같은 이유는 
멀티플레이어 네트워크 복제(Replication) 구조 때문

언리얼 엔진의 멀티플레이어 환경에서는 서버(Server)와 클라이언트(Client)가 
데미지를 처리하는 시점과 방식이 다르기 때문에 
두 곳에 각각 동일한 연출/UI 갱신 로직이 들어가야한다.
*/
