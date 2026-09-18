#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DropMapLabelWidget.generated.h"

UCLASS()
class BATTLEGROUNDCLONE_API UDropMapLabelWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "DropMap")
	void SetLabelText(const FText& Text);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UTextBlock> LabelText;
};
