#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MatchResultWidget.generated.h"

class UTextBlock;
class UButton;

UCLASS()
class BATTLEGROUNDCLONE_API UMatchResultWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(const FString& PlayerName, int32 Placement, int32 TotalPlayers, int32 Kills);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PlayerNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> RankText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TotalText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PlacementText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> KillText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> MainMenuButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CountdownText;

	UPROPERTY(EditAnywhere, Category = "Match")
	float AutoReturnSeconds = 0.f;

private:
	UFUNCTION()
	void OnMainMenuClicked();

	float RemainingSeconds = 0.f;
	int32 LastShownSeconds = -1;
};
