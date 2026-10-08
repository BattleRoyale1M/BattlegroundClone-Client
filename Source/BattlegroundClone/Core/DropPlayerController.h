#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Engine/DataTable.h"
#include "DropPlayerController.generated.h"

class UUserWidget;
class UInputAction;
class AAirPlane;
class UCanvasPanel;
class ALootContainer;

USTRUCT(BlueprintType)
struct FMapLocationRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DropMap")
	FText LocationName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DropMap")
	FVector WorldLocation = FVector::ZeroVector;
};

UCLASS()
class BATTLEGROUNDCLONE_API ADropPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ADropPlayerController();
	
	UFUNCTION(Exec, Category = "Cheat")
	void Cheat_Damage();

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
	
	UFUNCTION(BlueprintCallable, Category = "DropMap")
	void ToggleInventory();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void OpenLootScreen(ALootContainer* Container);

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<ALootContainer> ActiveLootContainer;

	UFUNCTION(BlueprintCallable, Category = "DropMap")
	void GetMinimapView(float MapPixels, float ViewportPixels,
		FVector2D& OutPan, float& OutSelfAngle,
		bool& bMarkerValid, FVector2D& OutMarkerPos) const;
	
	UFUNCTION(BlueprintCallable, Category = "DropMap")
	void GetWorldMapView(float MapPixels, 
		FVector2D& OutSelfPos, float& OutSelfAngle, 
		bool& bMarkerValid, FVector2D& OutMarkerPos) const;
	
	UFUNCTION(BlueprintCallable, Category = "DropMap")
	void GetFlightPathLine(float MapPixels, FVector2D& OutMid, float& OutLength,
						   float& OutAngle, bool& bHasPath) const;
	
	UFUNCTION(BlueprintCallable, Category = "DropMap")
	void GetPlayerMarkers(float MapPixels, bool bIncludeSelf,
		TArray<FVector2D>& OutPositions, TArray<FLinearColor>& OutColors) const;

	UFUNCTION(BlueprintCallable, Category = "DropMap")
	void RefreshPlayerMarkers(class UCanvasPanel* MarkerCanvas,
		TSubclassOf<UUserWidget> MarkerClass, float MapPixels, bool bIncludeSelf = false);

	UFUNCTION(BlueprintCallable, Category = "DropMap")
	void GetLocationLabels(float MapPixels, TArray<FVector2D>& OutPositions, TArray<FText>& OutNames) const;

	UFUNCTION(BlueprintCallable, Category = "DropMap")
	void RefreshLocationLabels(class UCanvasPanel* LabelCanvas, TSubclassOf<UUserWidget> LabelWidgetClass, float MapPixels);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ShowCenterNotification(const FText& Line1, const FText& Line2, FLinearColor Line2Color);
	// RPC 방향 문제대응
	UFUNCTION(Client, Reliable, Category = "HUD")
	void ClientShowCenterNotification(const FText& Line1, const FText& Line2, FLinearColor Line2Color);
	
	void SetGameplayHUDVisible(bool bVisible);
	void SetNavigationHUDVisible(bool bVisible);

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void ShowDeathUI();

	UFUNCTION(Client, Reliable)
	void ClientShowMatchResult(bool bVictory, int32 Placement, int32 TotalPlayers, int32 Kills);

	UFUNCTION(BlueprintCallable, Category = "Match")
	void GoToMainMenu();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	
	UPROPERTY(EditDefaultsOnly, Category = "DropMap|UI")
	TSubclassOf<UUserWidget> HUDWidgetClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "DropMap|UI")
	TSubclassOf<UUserWidget> WorldMapWidgetClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "DropMap|UI")
	TObjectPtr<UDataTable> LocationDataTable;

	UPROPERTY(EditDefaultsOnly, Category = "DropMap|Input")
	TObjectPtr<UInputAction> MapAction;
	
	UPROPERTY(EditDefaultsOnly, Category = "DropMap|UI")
	TSubclassOf<UUserWidget> InventoryWidgetClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "DropMap|Input")
	TObjectPtr<UInputAction> ToggleInventoryAction;
	
	UPROPERTY(EditDefaultsOnly, Category = "Combat|UI")
	TSubclassOf<UUserWidget> DeathUIWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Lobby|UI")
	TSubclassOf<UUserWidget> LobbyWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Match|UI")
	TSubclassOf<class UMatchResultWidget> GameOverWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Match|UI")
	TSubclassOf<class UMatchResultWidget> VictoryWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Match")
	TSoftObjectPtr<UWorld> MainMenuLevel;

private:
	AAirPlane* FindPlane() const;

	void EnsureBounds() const;

	void ShowInventoryWidget();
	void CloseInventoryWidget();

	/*
	실제로 생성된 위젯(인스턴스)을 메모리에 담아두고 관리하는 변수
	*/
	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> HUDWidget;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> WorldMapWidget;
	
	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> InventoryWidget;
	bool bInventoryOpen = false;
	
	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> DeathUIWidget;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> LobbyWidget;

	UPROPERTY(Transient)
	TObjectPtr<class UMatchResultWidget> MatchResultWidget;
	
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float DeathFadeDuration = 2.f;
	
	// --

	mutable FVector2D WorldMin = FVector2D(-100000.0, -100000.0);
	mutable FVector2D WorldMax = FVector2D(100000.0, 100000.0);
	mutable bool bBoundsResolved = false; // 경계값 구했는지 여부

	FVector2D MarkerNormalized = FVector2D::ZeroVector;
	bool bHasMarker = false;
	bool bWorldMapOpen = false;
};
