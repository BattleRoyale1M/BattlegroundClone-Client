#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LobbyCountdownWidget.generated.h"

class UImage;
class UTextBlock;
class UMaterialInstanceDynamic;

/*
로비 중앙 원형 카운트다운. 링(CountdownRing)은 라디얼 머티리얼의 "Percent" 스칼라 파라미터로 채움을 제어한다.
*/
UCLASS()
class BATTLEGROUNDCLONE_API ULobbyCountdownWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CountdownText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> CountdownRing;

	UPROPERTY(EditDefaultsOnly, Category = "Lobby")
	FName RingPercentParamName = "Percent";

private:
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> RingMID;
};
