#include "Combat/HealthComponent.h"
#include "Net/UnrealNetwork.h"

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
	
	DOREPLIFETIME_CONDITION_NOTIFY(UHealthComponent, Health, COND_None, REPNOTIFY_Always);
}

void UHealthComponent::ApplyDamage(float Amount, AController* Instigator, AActor* DamageCauser)
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

/*
첫 번째 함수와 두 번째 함수의 로직이 거의 똑같은 이유는 
멀티플레이어 네트워크 복제(Replication) 구조 때문

언리얼 엔진의 멀티플레이어 환경에서는 서버(Server)와 클라이언트(Client)가 
데미지를 처리하는 시점과 방식이 다르기 때문에 
두 곳에 각각 동일한 연출/UI 갱신 로직이 들어가야한다.
*/
