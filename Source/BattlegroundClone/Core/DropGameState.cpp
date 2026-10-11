#include "Core/DropGameState.h"
#include "Net/UnrealNetwork.h"
#include "EngineUtils.h"
#include "Drop/ZoneWall.h"

void ADropGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADropGameState, AliveCount);
	DOREPLIFETIME(ADropGameState, TotalCount);
	DOREPLIFETIME(ADropGameState, bZoneActive);
	DOREPLIFETIME(ADropGameState, ZoneFromCenter);
	DOREPLIFETIME(ADropGameState, ZoneFromRadius);
	DOREPLIFETIME(ADropGameState, ZoneToCenter);
	DOREPLIFETIME(ADropGameState, ZoneToRadius);
	DOREPLIFETIME(ADropGameState, ZoneShrinkStartTime);
	DOREPLIFETIME(ADropGameState, ZoneShrinkEndTime);
}

bool ADropGameState::GetCurrentZone(FVector2D& OutCenter, float& OutRadius) const
{
	if (!bZoneActive)
	{
		return false;
	}
	const float Duration = ZoneShrinkEndTime - ZoneShrinkStartTime;
	const float Now = static_cast<float>(GetServerWorldTimeSeconds());
	const float Alpha = Duration > 0.f ? FMath::Clamp((Now - ZoneShrinkStartTime) / Duration, 0.f, 1.f) : 1.f;
	OutCenter = FMath::Lerp(ZoneFromCenter, ZoneToCenter, Alpha);
	OutRadius = FMath::Lerp(ZoneFromRadius, ZoneToRadius, Alpha);
	return true;
}

void ADropGameState::BeginPlay()
{
	Super::BeginPlay();

	// ★ 맵에 배치 안 돼 있어도 서버/클라 각자 로컬로 자기장 벽 생성
	if (GetNetMode() == NM_DedicatedServer || TActorIterator<AZoneWall>(GetWorld()))
	{
		return;
	}
	GetWorld()->SpawnActor<AZoneWall>();
}
