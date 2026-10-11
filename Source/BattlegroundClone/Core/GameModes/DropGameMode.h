#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DropGameMode.generated.h"

USTRUCT(BlueprintType)
struct FZonePhase
{
	GENERATED_BODY()

	FZonePhase() {}
	FZonePhase(float InWait, float InShrink, float InRadius, float InDps)
		: WaitTime(InWait), ShrinkTime(InShrink), Radius(InRadius), DamagePerSecond(InDps) {}

	UPROPERTY(EditAnywhere) 
	float WaitTime = 60.f;
	
	UPROPERTY(EditAnywhere) 
	float ShrinkTime = 60.f;
	
	UPROPERTY(EditAnywhere) 
	float Radius = 30000.f;
	
	UPROPERTY(EditAnywhere) 
	float DamagePerSecond = 1.f;
};

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
	
	UPROPERTY(EditAnywhere, Category = "Zone")
	FVector2D ZoneFinalCenter = FVector2D(116500.0, -164500.0);
	
	UPROPERTY(EditAnywhere, Category = "Zone")
	float ZoneStartDelay = 90.f;
	
	UPROPERTY(EditAnywhere, Category = "Zone")
	float ZoneDamageInterval = 1.f;
	
	UPROPERTY(EditAnywhere, Category = "Zone")
	TArray<FZonePhase> ZonePhases = {
		FZonePhase(30.f, 60.f, 60000.f, 1.f),
		FZonePhase(30.f, 45.f, 30000.f, 2.f),
		FZonePhase(20.f, 40.f, 12000.f, 4.f),
		FZonePhase(20.f, 30.f, 4000.f, 8.f)
	};

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
	
	int32 ZonePhaseIndex = -1;
	float ZoneDamagePerSecond = 0.f;
	FTimerHandle ZonePhaseTimerHandle;
	FTimerHandle ZoneDamageTimerHandle;
	void StartZone();
	void AdvanceZonePhase();
	void ApplyZoneDamage();
};
