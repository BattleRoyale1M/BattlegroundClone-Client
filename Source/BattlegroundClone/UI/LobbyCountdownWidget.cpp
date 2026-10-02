#include "LobbyCountdownWidget.h"
#include "Core/GameModes/LobbyGameState.h"
#include "Components/TextBlock.h"
#include "Internationalization/Text.h"

void ULobbyCountdownWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const ALobbyGameState* LobbyState = GetWorld() ? GetWorld()->GetGameState<ALobbyGameState>() : nullptr;
	if (!LobbyState || !CountdownText) return;

	const int32 SecondsLeft = FMath::CeilToInt(LobbyState->GetRemainingSeconds());
	CountdownText->SetText(FText::Format(
		NSLOCTEXT("Lobby", "CountdownFormat", "전투시작까지 {0}초 남았습니다."),
		FText::AsNumber(SecondsLeft)));
}
