#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SessionApiSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FSessionInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString SessionId;

	UPROPERTY(BlueprintReadOnly)
	FString HostName;

	UPROPERTY(BlueprintReadOnly)
	FString Ip;

	UPROPERTY(BlueprintReadOnly)
	int32 Port = 0;

	UPROPERTY(BlueprintReadOnly)
	FString MapName;

	UPROPERTY(BlueprintReadOnly)
	int32 MaxPlayers = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 CurrentPlayers = 0;

	UPROPERTY(BlueprintReadOnly)
	FString CreatedAt;
};

USTRUCT()
struct FCreateSessionRequest
{
	GENERATED_BODY()

	UPROPERTY()
	FString HostName;

	UPROPERTY()
	FString Ip;

	UPROPERTY()
	int32 Port = 0;

	UPROPERTY()
	FString MapName;

	UPROPERTY()
	int32 MaxPlayers = 0;
};

DECLARE_DELEGATE_TwoParams(FOnSessionResult, bool, const FSessionInfo&);
DECLARE_DELEGATE_TwoParams(FOnSessionList, bool, const TArray<FSessionInfo>&);

UCLASS(Config = Game)
class BATTLEGROUNDCLONE_API USessionApiSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	void CreateSession(int32 Port, const FString& MapName, int32 MaxPlayers);
	void FetchSessions(FOnSessionList OnDone);
	void JoinSession(const FString& SessionId, FOnSessionResult OnDone);
	void LeaveSession();
	void DeleteSession();

	bool HasSession() const { return !CurrentSessionId.IsEmpty(); }

private:
	void SendRequest(const FString& Verb, const FString& Path, const FString& Body,
		TFunction<void(bool, const FString&)> OnDone);

	static FString GetLocalIp();

	UPROPERTY(Config)
	FString BaseUrl;

	FString CurrentSessionId;
};
