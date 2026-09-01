#include "Core/LobbyGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

void ALobbyGameMode::BeginPlay()
{
	Super::BeginPlay();
	GetWorldTimerManager().SetTimer(CountdownHandle, this,
		&ALobbyGameMode::TravelToMatch, CountdownSeconds, false);
}

void ALobbyGameMode::TravelToMatch()
{
	if (TargetLevel.IsNull()) return;
	UGameplayStatics::OpenLevel(this,
		FName(*TargetLevel.ToSoftObjectPath().GetLongPackageName()));
}