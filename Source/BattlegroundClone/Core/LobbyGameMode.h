// LobbyGameMode.h
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LobbyGameMode.generated.h"

UCLASS()
class BATTLEGROUNDCLONE_API ALobbyGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	virtual void BeginPlay() override;
protected:
	UPROPERTY(EditDefaultsOnly, Category="Match") float CountdownSeconds = 5.f;
	UPROPERTY(EditDefaultsOnly, Category="Match") TSoftObjectPtr<UWorld> TargetLevel;
	void TravelToMatch();
private:
	FTimerHandle CountdownHandle;
};
