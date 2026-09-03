#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "DropPlayerController.generated.h"

class UUserWidget;
class UInputAction;
class AAirPlane;

UCLASS()
class BATTLEGROUNDCLONE_API ADropPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ADropPlayerController();

	// --- 위젯이 읽을데이터 --------------------------------------------------
	
	UFUNCTION(BlueprintPure, Category = "DropMap")
	void GetMapBounds(FVector2D& OutWorldMin, FVector2D& OutWorldMax) const;
	
	UFUNCTION(BlueprintPure, Category = "DropMap")
	FVector GetSelfMapLocation() const;
	
	UFUNCTION(BlueprintPure, Category = "DropMap")
	bool GetFlightPath(FVector& OutStart, FVector& OutEnd) const;

	UFUNCTION(BlueprintPure, Category = "DropMap")
	FVector2D GetMarkerNormalized(bool& bValid) const;
	
	UFUNCTION(BlueprintPure, Category = "DropMap")
	bool IsWorldMapOpen() const { return bWorldMapOpen; }
    	
	UFUNCTION(BlueprintCallable, Category = "DropMap")
	void SetMarkerNormalized(FVector2D Normalized);

	UFUNCTION(BlueprintCallable, Category = "DropMap")
	void ClearMarker();
	
	UFUNCTION(BlueprintCallable, Category = "DropMap")
	void ToggleWorldMap();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	
	UPROPERTY(EditDefaultsOnly, Category = "DropMap|UI")
	TSubclassOf<UUserWidget> HUDWidgetClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "DropMap|UI")
	TSubclassOf<UUserWidget> WorldMapWidgetClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "DropMap|Input")
	TObjectPtr<UInputAction> MapAction;

private:
	AAirPlane* FindPlane() const;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> HUDWidget;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> WorldMapWidget;

	FVector2D WorldMin = FVector2D(-100000.0, -100000.0);
	FVector2D WorldMax = FVector2D(100000.0, 100000.0);

	FVector2D MarkerNormalized = FVector2D::ZeroVector;
	bool bHasMarker = false;
	bool bWorldMapOpen = false;
};
