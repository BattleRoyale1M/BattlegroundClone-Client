#include "Core/GameModes/LobbyGameMode.h"
#include "Core/GameModes/LobbyGameState.h"
#include "Core/Network/SessionApiSubsystem.h"
#include "GameFramework/GameSession.h"
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

	if (GetNetMode() == NM_ListenServer || GetNetMode() == NM_DedicatedServer)
	{
		if (USessionApiSubsystem* SessionApi = GetSessionApi())
		{
			SessionApi->CreateSession(GetWorld()->URL.Port, TargetLevel.GetAssetName(),
				GameSession ? GameSession->MaxPlayers : 0);
		}
	}
}

void ALobbyGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (USessionApiSubsystem* SessionApi = GetSessionApi())
	{
		SessionApi->DeleteSession();
	}
	Super::EndPlay(EndPlayReason);
}

void ALobbyGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);
	if (Exiting && !Exiting->IsLocalController())
	{
		if (USessionApiSubsystem* SessionApi = GetSessionApi())
		{
			SessionApi->LeaveSession();
		}
	}
}

void ALobbyGameMode::TravelToMatch()
{
	if (TargetLevel.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("[LobbyGameMode] TargetLevel 미설정 - 트래블 취소"));
		return;
	}
	if (USessionApiSubsystem* SessionApi = GetSessionApi())
	{
		SessionApi->DeleteSession();
	}
	const FString PackageName = TargetLevel.ToSoftObjectPath().GetLongPackageName();
	GetWorld() -> ServerTravel(PackageName);
}

USessionApiSubsystem* ALobbyGameMode::GetSessionApi() const
{
	const UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<USessionApiSubsystem>() : nullptr;
}
