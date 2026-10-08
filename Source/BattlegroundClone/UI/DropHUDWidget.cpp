#include "DropHUDWidget.h"
#include "Components/Widget.h"
#include "Components/TextBlock.h"
#include "Core/DropGameState.h"
#include "Core/DropPlayerState.h"
#include "GameFramework/PlayerController.h"

void UDropHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const UWorld* World = GetWorld();
	const ADropGameState* GS = World ? World->GetGameState<ADropGameState>() : nullptr;
	if (GS && AliveCountText && GS->AliveCount != CachedAliveCount)
	{
		CachedAliveCount = GS->AliveCount;
		AliveCountText->SetText(FText::AsNumber(CachedAliveCount));
	}

	const APlayerController* PC = GetOwningPlayer();
	const ADropPlayerState* PS = PC ? PC->GetPlayerState<ADropPlayerState>() : nullptr;
	if (PS && KillCountText && PS->KillCount != CachedKillCount)
	{
		CachedKillCount = PS->KillCount;
		KillCountText->SetText(FText::AsNumber(CachedKillCount));
	}
}

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
