#include "Core/GameModes/LobbyGameMode.h"
#include "Core/GameModes/LobbyGameState.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ALobbyGameMode::ALobbyGameMode()
{
	bUseSeamlessTravel = true;
	GameStateClass = ALobbyGameState::StaticClass();
}

void ALobbyGameMode::BeginPlay()
{
	Super::BeginPlay();
	if (ALobbyGameState* LobbyState = GetGameState<ALobbyGameState>())
	{
		LobbyState->StartCountdown(CountdownSeconds);
	}
	GetWorldTimerManager().SetTimer(CountdownHandle, this,
		&ALobbyGameMode::TravelToMatch, CountdownSeconds, false);
}

void ALobbyGameMode::TravelToMatch()
{
	if (TargetLevel.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("[LobbyGameMode] TargetLevel 미설정 - 트래블 취소"));
		return;
	}
	const FString PackageName = TargetLevel.ToSoftObjectPath().GetLongPackageName();
	GetWorld() -> ServerTravel(PackageName);
}