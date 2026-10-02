#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "LobbyGameState.generated.h"

UCLASS()
class BATTLEGROUNDCLONE_API ALobbyGameState : public AGameStateBase
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintPure, Category = "Lobby")
	float GetRemainingSeconds() const;

	UFUNCTION(BlueprintPure, Category = "Lobby")
	float GetRemainingFraction() const;

	void StartCountdown(float InDuration);

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(Replicated)
	double CountdownEndServerTime = 0.0;

	UPROPERTY(Replicated)
	float CountdownDuration = 0.f;
};
