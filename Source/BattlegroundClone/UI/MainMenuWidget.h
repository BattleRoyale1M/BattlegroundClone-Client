#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/Network/SessionApiSubsystem.h"
#include "MainMenuWidget.generated.h"

class UMainMenuWidget;

UCLASS()
class USessionEntryHandler : public UObject
{
	GENERATED_BODY()

public:
	FSessionInfo Info;
	TWeakObjectPtr<UMainMenuWidget> Owner;

	UFUNCTION()
	void HandleClicked();
};

UCLASS()
class UMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void JoinSession(const FSessionInfo& Info);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	class UButton* HostButton;

	UPROPERTY(meta = (BindWidget))
	class UButton* JoinButton;

	UPROPERTY(meta = (BindWidgetOptional))
	class UEditableTextBox* IPAddressInput;

	UPROPERTY(meta = (BindWidgetOptional))
	class UScrollBox* SessionList;

	UFUNCTION()
	void OnHostClicked();

	UFUNCTION()
	void OnJoinClicked();

private:
	void RefreshSessions();
	void PopulateSessionList(const TArray<FSessionInfo>& Sessions);
	USessionApiSubsystem* GetSessionApi() const;

	UPROPERTY()
	TArray<TObjectPtr<USessionEntryHandler>> EntryHandlers;
};
