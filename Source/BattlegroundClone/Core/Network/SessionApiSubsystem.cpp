#include "Core/Network/SessionApiSubsystem.h"

#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "JsonObjectConverter.h"
#include "SocketSubsystem.h"
#include "IPAddress.h"

void USessionApiSubsystem::CreateSession(int32 Port, const FString& MapName, int32 MaxPlayers)
{
	FCreateSessionRequest Payload;
	Payload.HostName = FPlatformProcess::ComputerName();
	Payload.Ip = GetLocalIp();
	Payload.Port = Port;
	Payload.MapName = MapName;
	Payload.MaxPlayers = MaxPlayers;

	FString Body;
	FJsonObjectConverter::UStructToJsonObjectString(Payload, Body);

	SendRequest(TEXT("POST"), TEXT(""), Body, [this](bool bOk, const FString& Content)
	{
		FSessionInfo Info;
		if (bOk && FJsonObjectConverter::JsonObjectStringToUStruct(Content, &Info))
		{
			CurrentSessionId = Info.SessionId;
			UE_LOG(LogTemp, Log, TEXT("[SessionApi] 세션 등록 %s (%s:%d)"), *Info.SessionId, *Info.Ip, Info.Port);
		}
	});
}

void USessionApiSubsystem::FetchSessions(FOnSessionList OnDone)
{
	SendRequest(TEXT("GET"), TEXT(""), FString(), [OnDone](bool bOk, const FString& Content)
	{
		TArray<FSessionInfo> Sessions;
		if (bOk)
		{
			bOk = FJsonObjectConverter::JsonArrayStringToUStruct(Content, &Sessions);
		}
		OnDone.ExecuteIfBound(bOk, Sessions);
	});
}

void USessionApiSubsystem::JoinSession(const FString& SessionId, FOnSessionResult OnDone)
{
	SendRequest(TEXT("POST"), FString::Printf(TEXT("/%s/join"), *SessionId), FString(),
		[OnDone](bool bOk, const FString& Content)
	{
		FSessionInfo Info;
		if (bOk)
		{
			bOk = FJsonObjectConverter::JsonObjectStringToUStruct(Content, &Info);
		}
		OnDone.ExecuteIfBound(bOk, Info);
	});
}

void USessionApiSubsystem::LeaveSession()
{
	if (!HasSession()) return;
	SendRequest(TEXT("POST"), FString::Printf(TEXT("/%s/leave"), *CurrentSessionId), FString(),
		[](bool, const FString&) {});
}

void USessionApiSubsystem::DeleteSession()
{
	if (!HasSession()) return;
	SendRequest(TEXT("DELETE"), FString::Printf(TEXT("/%s"), *CurrentSessionId), FString(),
		[](bool, const FString&) {});
	CurrentSessionId.Reset();
}

void USessionApiSubsystem::SendRequest(const FString& Verb, const FString& Path, const FString& Body,
	TFunction<void(bool, const FString&)> OnDone)
{
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(BaseUrl + TEXT("/api/v0/sessions") + Path);
	Request->SetVerb(Verb);
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	if (!Body.IsEmpty())
	{
		Request->SetContentAsString(Body);
	}

	Request->OnProcessRequestComplete().BindWeakLambda(this,
		[Verb, OnDone = MoveTemp(OnDone)](FHttpRequestPtr Req, FHttpResponsePtr Response, bool bConnected)
	{
		const bool bOk = bConnected && Response.IsValid() && EHttpResponseCodes::IsOk(Response->GetResponseCode());
		const FString Content = Response.IsValid() ? Response->GetContentAsString() : FString();
		if (!bOk)
		{
			UE_LOG(LogTemp, Warning, TEXT("[SessionApi] %s %s 실패 (%d) %s"), *Verb, *Req->GetURL(),
				Response.IsValid() ? Response->GetResponseCode() : 0, *Content);
		}
		OnDone(bOk, Content);
	});
	Request->ProcessRequest();
}

FString USessionApiSubsystem::GetLocalIp()
{
	bool bCanBindAll = false;
	TSharedPtr<FInternetAddr> Addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLocalHostAddr(*GLog, bCanBindAll);
	return Addr.IsValid() ? Addr->ToString(false) : TEXT("127.0.0.1");
}
