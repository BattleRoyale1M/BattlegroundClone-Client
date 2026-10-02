#include "LobbyCountdownWidget.h"
#include "Core/GameModes/LobbyGameState.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Materials/MaterialInstanceDynamic.h"

void ULobbyCountdownWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (CountdownRing)
	{
		RingMID = CountdownRing->GetDynamicMaterial();
	}
}

void ULobbyCountdownWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const ALobbyGameState* LobbyState = GetWorld() ? GetWorld()->GetGameState<ALobbyGameState>() : nullptr;
	if (!LobbyState) return;

	if (CountdownText)
	{
		const int32 SecondsLeft = FMath::CeilToInt(LobbyState->GetRemainingSeconds());
		CountdownText->SetText(FText::AsNumber(SecondsLeft));
	}
	if (RingMID)
	{
		RingMID->SetScalarParameterValue(RingPercentParamName, LobbyState->GetRemainingFraction());
	}
}
