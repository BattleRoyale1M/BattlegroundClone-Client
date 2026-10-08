#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DropGameMode.generated.h"

UCLASS()
class BATTLEGROUNDCLONE_API ADropGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	ADropGameMode();
	
	UFUNCTION(BluePrintPure, Category = "DropMap")
	void GetMapBounds(FVector2D& OutWorldMin, FVector2D& OutWorldMax) const;

	void RegisterCombatant(class ADropCharacter* Character);
	void NotifyCombatantDied(class ADropCharacter* Victim);
	void RemoveCombatant(class ADropCharacter* Character);

protected:
	virtual void OnPostLogin(AController* NewPlayer) override;
	virtual void BeginPlay() override;
	
	UPROPERTY(EditDefaultsOnly, Category = "Drop|Plane")
	TSubclassOf<class AAirPlane> PlaneClass;
	UPROPERTY(EditAnywhere, Category = "Drop|Plane")
	FVector PlaneRouteStart = FVector(-100000.f, 0.f, 20000.f);
	UPROPERTY(EditAnywhere, Category = "Drop|Plane")
	FVector PlaneRouteEnd = FVector(100000.f, 0.f, 20000.f);
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DropMap")
	FVector2D WorldMin = FVector2D(-100000.0, -100000.0);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DropMap")
	FVector2D WorldMax = FVector2D(100000.0, 100000.0);
	
	UPROPERTY(EditDefaultsOnly, Category = "Map")
	TArray<FLinearColor> MarkerPalette = {
		FLinearColor(1.f, 0.25f, 0.25f), FLinearColor(0.3f, 0.6f, 1.f),
		FLinearColor(0.4f, 1.f, 0.4f),   FLinearColor(1.f, 0.85f, 0.2f),
		FLinearColor(1.f, 0.5f, 0.1f),   FLinearColor(0.7f, 0.4f, 1.f)
	};
	
	UPROPERTY(EditDefaultsOnly, Category = "Match")
	float VictoryDelay = 2.f;

	UPROPERTY(EditDefaultsOnly, Category = "Match")
	int32 MaxBots = 5;

private:
	int32 NextMarkerIndex = 0;

	TArray<TWeakObjectPtr<class ADropCharacter>> AliveCombatants;
	TWeakObjectPtr<class ADropCharacter> Winner;
	bool bMatchOver = false;
	FTimerHandle VictoryTimerHandle;

	int32 RegisteredBots = 0;
	TArray<TWeakObjectPtr<class ADropCharacter>> ExtraBots;
	FTimerHandle ExtraBotsTimerHandle;
	void RemoveExtraBots();

	void SyncAliveCount();
	void CheckForWinner();
	void AnnounceWinner();
};
