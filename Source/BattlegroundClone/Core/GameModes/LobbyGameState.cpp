#include "Core/GameModes/LobbyGameState.h"
#include "Net/UnrealNetwork.h"

void ALobbyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALobbyGameState, CountdownEndServerTime);
	DOREPLIFETIME(ALobbyGameState, CountdownDuration);
}

void ALobbyGameState::StartCountdown(float InDuration)
{
	CountdownDuration = InDuration;
	CountdownEndServerTime = GetServerWorldTimeSeconds() + InDuration;
}

float ALobbyGameState::GetRemainingSeconds() const
{
	return FMath::Max(0.f, static_cast<float>(CountdownEndServerTime - GetServerWorldTimeSeconds()));
}

float ALobbyGameState::GetRemainingFraction() const
{
	if (CountdownDuration <= 0.f) return 0.f;
	return FMath::Clamp(GetRemainingSeconds() / CountdownDuration, 0.f, 1.f);
}
