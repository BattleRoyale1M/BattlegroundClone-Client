#include "UI/MapGridWidget.h"

#include "Rendering/DrawElements.h"
#include "Widgets/SLeafWidget.h"

/*
실제 드로잉 담당 -> UMapGridWidget이 파라미터만 넘겨줌 
*/
class SMapGrid : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SMapGrid) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs) {}

	FLinearColor GridColor = FLinearColor::White;
	float GridOpacity = 0.4f;
	float CellWorldSize = 10000.f;
	FVector2D MapWorldSize = FVector2D(200000.0, 200000.0);
	int32 MajorEvery = 5;
	float LineThickness = 1.f;
	float MajorLineThickness = 2.f;

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override
	{
		const FVector2D Size = AllottedGeometry.GetLocalSize();
		if (Size.X <= 1.0 || Size.Y <= 1.0 || CellWorldSize <= 0.f)
		{
			return LayerId;
		}

		const double CellPxX = Size.X * (double(CellWorldSize) / FMath::Max(MapWorldSize.X, 1.0));
		const double CellPxY = Size.Y * (double(CellWorldSize) / FMath::Max(MapWorldSize.Y, 1.0));
		if (CellPxX < 2.0 || CellPxY < 2.0)
		{
			return LayerId;
		}

		FLinearColor Minor = GridColor;
		Minor.A *= GridOpacity;
		FLinearColor Major = GridColor;
		Major.A *= FMath::Min(1.f, GridOpacity * 1.8f);

		const FPaintGeometry PG = AllottedGeometry.ToPaintGeometry();
		const int32 MinorLayer = LayerId;
		const int32 MajorLayer = LayerId + 1;

		TArray<FVector2D> P;
		P.SetNum(2);

		int32 Index = 0;
		for (double X = 0.0; X <= Size.X + 0.5; X += CellPxX, ++Index)
		{
			const bool bMajor = (MajorEvery > 1) && (Index % MajorEvery == 0);
			P[0] = FVector2D(X, 0.0);
			P[1] = FVector2D(X, Size.Y);
			FSlateDrawElement::MakeLines(OutDrawElements, bMajor ? MajorLayer : MinorLayer, PG, P,
				ESlateDrawEffect::None, bMajor ? Major : Minor, true,
				bMajor ? MajorLineThickness : LineThickness);
		}

		Index = 0;
		for (double Y = 0.0; Y <= Size.Y + 0.5; Y += CellPxY, ++Index)
		{
			const bool bMajor = (MajorEvery > 1) && (Index % MajorEvery == 0);
			P[0] = FVector2D(0.0, Y);
			P[1] = FVector2D(Size.X, Y);
			FSlateDrawElement::MakeLines(OutDrawElements, bMajor ? MajorLayer : MinorLayer, PG, P,
				ESlateDrawEffect::None, bMajor ? Major : Minor, true,
				bMajor ? MajorLineThickness : LineThickness);
		}

		return MajorLayer + 1;
	}

	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(64.f, 64.f); }
};

// ---------------------------------------------------------------------------

TSharedRef<SWidget> UMapGridWidget::RebuildWidget()
{
	MyGrid = SNew(SMapGrid);
	return MyGrid.ToSharedRef();
}

void UMapGridWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	if (!MyGrid.IsValid())
	{
		return;
	}

	MyGrid->GridColor = GridColor;
	MyGrid->GridOpacity = GridOpacity;
	MyGrid->CellWorldSize = CellWorldSize;
	MyGrid->MapWorldSize = MapWorldSize;
	MyGrid->MajorEvery = MajorEvery;
	MyGrid->LineThickness = LineThickness;
	MyGrid->MajorLineThickness = MajorLineThickness;
	MyGrid->Invalidate(EInvalidateWidgetReason::Paint);
}

void UMapGridWidget::SetGrid(float InCellWorldSize, FVector2D InMapWorldSize)
{
	CellWorldSize = InCellWorldSize;
	MapWorldSize = InMapWorldSize;
	SynchronizeProperties();
}

void UMapGridWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MyGrid.Reset();
}

#if WITH_EDITOR
const FText UMapGridWidget::GetPaletteCategory()
{
	return NSLOCTEXT("BattlegroundClone", "MapUICategory", "Map UI");
}
#endif
