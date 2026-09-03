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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DropMap")
	FVector2D WorldMin = FVector2D(-100000.0, -100000.0);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DropMap")
	FVector2D WorldMax = FVector2D(100000.0, 100000.0);
};
