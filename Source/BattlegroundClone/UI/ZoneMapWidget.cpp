#include "UI/ZoneMapWidget.h"

#include "Rendering/DrawElements.h"
#include "Widgets/SLeafWidget.h"
#include "Core/DropGameState.h"
#include "Core/DropPlayerController.h"
#include "Core/DropMapLibrary.h"

class SZoneMap : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SZoneMap) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		ForceVolatile(true); // ★ 원이 계속 줄어드니 매 프레임 다시 그림
	}

	TWeakObjectPtr<UWidget> OwnerWidget;
	FLinearColor CurrentColor = FLinearColor(0.15f, 0.45f, 1.f, 1.f);
	FLinearColor NextColor = FLinearColor::White;
	float LineThickness = 2.f;

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override
	{
		UWorld* W = OwnerWidget.IsValid() ? OwnerWidget->GetWorld() : nullptr; // ★ travel 후에도 현재 월드 기준
		const ADropGameState* GS = W ? W->GetGameState<ADropGameState>() : nullptr;
		const ADropPlayerController* PC = W ? Cast<ADropPlayerController>(W->GetFirstPlayerController()) : nullptr;
		FVector2D Center;
		float Radius;
		if (!GS || !PC || !GS->GetCurrentZone(Center, Radius))
		{
			return LayerId;
		}

		FVector2D WorldMin, WorldMax;
		PC->GetMapBounds(WorldMin, WorldMax);
		const FVector2D Size = AllottedGeometry.GetLocalSize();
		const FPaintGeometry PG = AllottedGeometry.ToPaintGeometry();
		
		DrawCircle(OutDrawElements, LayerId, PG, Size, WorldMin, WorldMax, GS->ZoneToCenter, GS->ZoneToRadius, NextColor, LineThickness);
		
		FLinearColor Glow = CurrentColor;
		Glow.A *= 0.35f;
		DrawCircle(OutDrawElements, LayerId + 1, PG, Size, WorldMin, WorldMax, Center, Radius, Glow, LineThickness * 3.f);
		DrawCircle(OutDrawElements, LayerId + 2, PG, Size, WorldMin, WorldMax, Center, Radius, CurrentColor, LineThickness);
		return LayerId + 3;
	}

	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(64.f, 64.f); }

private:
	static void DrawCircle(FSlateWindowElementList& Out, int32 Layer, const FPaintGeometry& PG, const FVector2D& Size,
		const FVector2D& WorldMin, const FVector2D& WorldMax, const FVector2D& Center, float Radius,
		const FLinearColor& Color, float Thickness)
	{
		const FVector2D N = UDropMapLibrary::WorldToNormalized(FVector(Center.X, Center.Y, 0.f), WorldMin, WorldMax);
		const FVector2D CenterPx = UDropMapLibrary::NormalizedToWidget(FVector2D(N.Y, N.X), Size, true);
		const double RadiusPx = Radius / FMath::Max(WorldMax.Y - WorldMin.Y, 1.0) * Size.X;
		if (RadiusPx < 1.0)
		{
			return;
		}

		constexpr int32 Segments = 128;
		TArray<FVector2D> Points;
		Points.Reserve(Segments + 1);
		for (int32 i = 0; i <= Segments; ++i)
		{
			const double Angle = 2.0 * UE_DOUBLE_PI * i / Segments;
			Points.Add(CenterPx + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * RadiusPx);
		}
		FSlateDrawElement::MakeLines(Out, Layer, PG, Points, ESlateDrawEffect::None, Color, true, Thickness);
	}
};

TSharedRef<SWidget> UZoneMapWidget::RebuildWidget()
{
	MyZone = SNew(SZoneMap);
	return MyZone.ToSharedRef();
}

void UZoneMapWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	if (!MyZone.IsValid())
	{
		return;
	}
	MyZone->OwnerWidget = this;
	MyZone->CurrentColor = CurrentColor;
	MyZone->NextColor = NextColor;
	MyZone->LineThickness = LineThickness;
}

void UZoneMapWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MyZone.Reset();
}

#if WITH_EDITOR
const FText UZoneMapWidget::GetPaletteCategory()
{
	return NSLOCTEXT("BattlegroundClone", "MapUICategory", "Map UI");
}
#endif