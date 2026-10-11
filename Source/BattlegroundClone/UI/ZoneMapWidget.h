#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "ZoneMapWidget.generated.h"

UCLASS()
class BATTLEGROUNDCLONE_API UZoneMapWidget : public UWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone")
	FLinearColor CurrentColor = FLinearColor(0.15f, 0.45f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone")
	FLinearColor NextColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone", meta = (ClampMin = "0.0"))
	float LineThickness = 2.f;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void SynchronizeProperties() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif

private:
	TSharedPtr<class SZoneMap> MyZone;
};
