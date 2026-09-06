#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "MapGridWidget.generated.h"

/*
미니맵 & 전체지도 위에 얹는 격자 그리드 레이어
*/
UCLASS()
class BATTLEGROUNDCLONE_API UMapGridWidget : public UWidget
{
	GENERATED_BODY()

public:
	/** 격자선 색. 알파는 GridOpacity 로 곱해진다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Grid")
	FLinearColor GridColor = FLinearColor::White;

	/** 격자선 불투명도 (0~1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Grid", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float GridOpacity = 0.4f;

	/** 한 칸이 커버하는 월드 거리(uu). 예: 10000 = 100m. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Grid", meta = (ClampMin = "1.0"))
	float CellWorldSize = 10000.f;

	/** 맵 전체가 커버하는 월드 크기(uu) = (WorldMax - WorldMin). GameMode/PlayerController 경계와 맞출 것. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Grid")
	FVector2D MapWorldSize = FVector2D(200000.0, 200000.0);

	/** N칸마다 굵은 주(主)격자선. 0 또는 1이면 비활성. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Grid", meta = (ClampMin = "0"))
	int32 MajorEvery = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Grid", meta = (ClampMin = "0.0"))
	float LineThickness = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Grid", meta = (ClampMin = "0.0"))
	float MajorLineThickness = 2.f;

	/** 런타임에 격자 파라미터 갱신 후 다시 그리기. */
	UFUNCTION(BlueprintCallable, Category = "Map Grid")
	void SetGrid(float InCellWorldSize, FVector2D InMapWorldSize);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void SynchronizeProperties() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif

private:
	TSharedPtr<class SMapGrid> MyGrid;
};
