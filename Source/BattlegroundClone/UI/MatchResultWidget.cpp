#include "UI/MatchResultWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Core/DropPlayerController.h"

void UMatchResultWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (MainMenuButton)
	{
		MainMenuButton->OnClicked.AddUniqueDynamic(this, &UMatchResultWidget::OnMainMenuClicked);
	}
	RemainingSeconds = AutoReturnSeconds;
	LastShownSeconds = -1;
	if (CountdownText)
	{
		CountdownText->SetVisibility(AutoReturnSeconds > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UMatchResultWidget::Setup(const FString& PlayerName, int32 Placement, int32 TotalPlayers, int32 Kills)
{
	if (PlayerNameText)
	{
		PlayerNameText->SetText(FText::FromString(PlayerName));
	}
	const FText Rank = FText::FromString(FString::Printf(TEXT("#%d"), Placement));
	if (RankText)
	{
		RankText->SetText(Rank);
	}
	if (PlacementText)
	{
		PlacementText->SetText(Rank);
	}
	if (TotalText)
	{
		TotalText->SetText(FText::FromString(FString::Printf(TEXT("/%d"), TotalPlayers)));
	}
	if (KillText)
	{
		KillText->SetText(FText::AsNumber(Kills));
	}
}

void UMatchResultWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (AutoReturnSeconds <= 0.f)
	{
		return;
	}
	RemainingSeconds -= InDeltaTime;
	const int32 Seconds = FMath::Max(0, FMath::CeilToInt(RemainingSeconds));
	if (Seconds != LastShownSeconds)
	{
		LastShownSeconds = Seconds;
		if (CountdownText)
		{
			CountdownText->SetText(FText::Format(
				NSLOCTEXT("Match", "AutoReturn", "{0}초 후에 메인 메뉴로 이동합니다."), FText::AsNumber(Seconds)));
		}
	}
	if (RemainingSeconds <= 0.f)
	{
		AutoReturnSeconds = 0.f;
		OnMainMenuClicked();
	}
}

void UMatchResultWidget::OnMainMenuClicked()
{
	if (ADropPlayerController* PC = Cast<ADropPlayerController>(GetOwningPlayer()))
	{
		PC->GoToMainMenu();
	}
}
