#include "Core/DropPlayerState.h"
#include "Net/UnrealNetwork.h"

void ADropPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADropPlayerState, MarkerColor);
	DOREPLIFETIME(ADropPlayerState, KillCount);

}
