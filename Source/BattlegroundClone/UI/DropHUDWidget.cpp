#include "DropHUDWidget.h"
#include "Components/Widget.h"

void UDropHUDWidget::SetGameplayHUDVisible(bool bVisible)
{
	const ESlateVisibility Vis = bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
	if (AmmoBox) AmmoBox->SetVisibility(Vis);
	if (ItemSlotsBox) ItemSlotsBox->SetVisibility(Vis);
	if (HealthBar) HealthBar->SetVisibility(Vis);
}

void UDropHUDWidget::SetNavigationHUDVisible(bool bVisible)
{
	const ESlateVisibility Vis = bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
	if (CompassClip) CompassClip->SetVisibility(Vis);
	if (CompassCenter) CompassCenter->SetVisibility(Vis);
	if (MinimapRoot) MinimapRoot->SetVisibility(Vis);
}
