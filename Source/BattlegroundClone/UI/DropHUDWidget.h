#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DropHUDWidget.generated.h"

/*
화면 중앙 알림 (킬로그/탄창부족 등 공용). 실제 텍스트 세팅+페이드는 WBP_DropHUD 그래프에서 구현.
*/
UCLASS()
class BATTLEGROUNDCLONE_API UDropHUDWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void ShowCenterNotification(const FText& Line1, const FText& Line2, FLinearColor Line2Color);

	UPROPERTY(meta = (BindWidget))
	class UWidget* AmmoBox;

	UPROPERTY(meta = (BindWidget))
	class UWidget* ItemSlotsBox;

	UPROPERTY(meta = (BindWidget))
	class UWidget* HealthBar;

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetGameplayHUDVisible(bool bVisible);
};
