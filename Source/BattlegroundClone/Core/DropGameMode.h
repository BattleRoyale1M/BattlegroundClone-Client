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
	
protected:
	virtual void OnPostLogin(AController* NewPlayer) override;
	
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
	
private:
	int32 NextMarkerIndex = 0;
};
