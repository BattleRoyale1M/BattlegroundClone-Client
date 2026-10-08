#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "DropGameState.generated.h"

UCLASS()
class BATTLEGROUNDCLONE_API ADropGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
	int32 AliveCount = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
	int32 TotalCount = 0;
};
