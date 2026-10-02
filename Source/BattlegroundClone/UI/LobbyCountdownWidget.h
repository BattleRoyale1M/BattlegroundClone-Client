#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LobbyCountdownWidget.generated.h"

class UTextBlock;

/*
로비 중앙 카운트다운 텍스트 ("전투시작까지 n초 남았습니다.").
*/
UCLASS()
class BATTLEGROUNDCLONE_API ULobbyCountdownWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CountdownText;
};
