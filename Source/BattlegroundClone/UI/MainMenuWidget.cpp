#include "MainMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Components/VerticalBox.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/Border.h"
#include "Components/VerticalBoxSlot.h"

void USessionEntryHandler::HandleClicked()
{
	if (Owner.IsValid())
	{
		Owner->JoinSession(Info);
	}
}

void UMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	HostButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnHostClicked);
	JoinButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnJoinClicked);

	if (SessionList)
	{
		RefreshSessions();
	}
}

void UMainMenuWidget::OnHostClicked()
{
	if (UWorld* World = GetWorld())
	{
		World->ServerTravel(TEXT("/Game/Maps/L_Lobby?listen"));
	}
}

void UMainMenuWidget::OnJoinClicked()
{
	const FString IP = IPAddressInput ? IPAddressInput->GetText().ToString().TrimStartAndEnd() : FString();
	if (!IP.IsEmpty())
	{
		if (IP.Len() < 7) 
		{
			ShowErrorPopup(TEXT("올바른 IP 주소를 입력해주세요."));
			return;
		}
		if (APlayerController* PC = GetOwningPlayer())
		{
			PC -> ClientTravel(IP, ETravelType::TRAVEL_Absolute);
		}
		return;
	}
	RefreshSessions();
}

void UMainMenuWidget::RefreshSessions()
{
	USessionApiSubsystem* SessionApi = GetSessionApi();
	if (!SessionApi) return;

	SessionApi->FetchSessions(FOnSessionList::CreateWeakLambda(this,
		[this](bool bOk, const TArray<FSessionInfo>& Sessions)
	{
		if (!bOk)
		{
			ShowErrorPopup(TEXT("세션 서버와 통신할 수 없습니다. 네트워크 연결을 확인해주세요."));
			return;
		}
		if (SessionList)
		{
			PopulateSessionList(Sessions);
		}
		else if (Sessions.Num() > 0)
		{
			JoinSession(Sessions[0]);
		}
	}));
}

void UMainMenuWidget::PopulateSessionList(const TArray<FSessionInfo>& Sessions)
{
	SessionList->ClearChildren();
	EntryHandlers.Reset();

	for (const FSessionInfo& Info : Sessions)
	{
		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
		Label->SetText(FText::FromString(FString::Printf(TEXT("%s  (%d/%d)  %s"),
			*Info.HostName, Info.CurrentPlayers, Info.MaxPlayers, *Info.MapName)));

		UButton* Button = WidgetTree->ConstructWidget<UButton>();
		Button->AddChild(Label);

		USessionEntryHandler* Handler = NewObject<USessionEntryHandler>(this);
		Handler->Info = Info;
		Handler->Owner = this;
		Button->OnClicked.AddDynamic(Handler, &USessionEntryHandler::HandleClicked);

		SessionList->AddChild(Button);
		EntryHandlers.Add(Handler);
	}
}

void UMainMenuWidget::JoinSession(const FSessionInfo& Info)
{
	USessionApiSubsystem* SessionApi = GetSessionApi();
	if (!SessionApi) return;

	SessionApi->JoinSession(Info.SessionId, FOnSessionResult::CreateWeakLambda(this,
		[this](bool bOk, const FSessionInfo& Joined)
	{
		if (!bOk) return;
		if (APlayerController* PC = GetOwningPlayer())
		{
			PC -> ClientTravel(FString::Printf(TEXT("%s:%d"), *Joined.Ip, Joined.Port), ETravelType::TRAVEL_Absolute);
		}
	}));
}

USessionApiSubsystem* UMainMenuWidget::GetSessionApi() const
{
	const UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<USessionApiSubsystem>() : nullptr;
}

void UMainMenuWidget::ShowErrorPopup(const FString& Message)
{
	if (!WidgetTree) return;

	UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>();
	if (!RootOverlay) return;

	UBorder* BackgroundBorder = WidgetTree->ConstructWidget<UBorder>();
	if (!BackgroundBorder) return;
	BackgroundBorder->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.7f));

	UVerticalBox* DialogBox = WidgetTree->ConstructWidget<UVerticalBox>();

	UTextBlock* ErrorText = WidgetTree->ConstructWidget<UTextBlock>();
	ErrorText->SetText(FText::FromString(Message));
	ErrorText->SetColorAndOpacity(FSlateColor(FLinearColor::Red));
	ErrorText->SetJustification(ETextJustify::Center);

	UButton* ConfirmButton = WidgetTree->ConstructWidget<UButton>();
	UTextBlock* ButtonText = WidgetTree->ConstructWidget<UTextBlock>();
	ButtonText->SetText(FText::FromString(TEXT("확인")));
	ConfirmButton->AddChild(ButtonText);

	ConfirmButton->OnClicked.AddDynamic(RootOverlay, &UWidget::RemoveFromParent);

	DialogBox->AddChildToVerticalBox(ErrorText);
	DialogBox->AddChildToVerticalBox(ConfirmButton);
    
	BackgroundBorder->AddChild(DialogBox);

	UOverlaySlot* Slot = RootOverlay->AddChildToOverlay(BackgroundBorder);
	if (Slot)
	{
		Slot->SetHorizontalAlignment(HAlign_Fill);
		Slot->SetVerticalAlignment(VAlign_Fill);
	}

	UPanelWidget* RootPanel = Cast<UPanelWidget>(WidgetTree->RootWidget);
	if (RootPanel)
	{
		RootPanel->AddChild(BackgroundBorder);
	}
}
