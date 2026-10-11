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
	
	/*
	자기장
	*/
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Zone")
	bool bZoneActive = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Zone")
	FVector2D ZoneFromCenter = FVector2D::ZeroVector;
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Zone")
	float ZoneFromRadius = 0.f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Zone")
	FVector2D ZoneToCenter = FVector2D::ZeroVector;
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Zone")
	float ZoneToRadius = 0.f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Zone")
	float ZoneShrinkStartTime = 0.f;
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Zone")
	float ZoneShrinkEndTime = 0.f;

	UFUNCTION(BlueprintPure, Category = "Zone")
	bool GetCurrentZone(FVector2D& OutCenter, float& OutRadius) const;
};
