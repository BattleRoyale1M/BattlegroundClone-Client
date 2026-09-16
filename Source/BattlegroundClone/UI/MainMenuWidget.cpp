#include "MainMenuWidget.h"

#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "Components/EditableTextBox.h"


void UMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	HostButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnHostClicked);
	JoinButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnJoinClicked);
}

void UMainMenuWidget::OnHostClicked()
{
	UGameplayStatics::OpenLevel(this,  FName("L_TestRange"), true, TEXT("listen"));
}

void UMainMenuWidget::OnJoinClicked()
{
	FString IP = IPAddressInput->GetText().ToString();
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC -> ClientTravel(IP, ETravelType::TRAVEL_Absolute);
	}
}
