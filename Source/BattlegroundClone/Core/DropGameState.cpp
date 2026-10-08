#include "Core/DropGameState.h"
#include "Net/UnrealNetwork.h"

void ADropGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADropGameState, AliveCount);
	DOREPLIFETIME(ADropGameState, TotalCount);
}
