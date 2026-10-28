#include "Core/Network/SessionApiSubsystem.h"

#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "JsonObjectConverter.h"
#include "SocketSubsystem.h"
#include "IPAddress.h"
#include "Misc/Paths.h"

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
			
			if (GEngine)
			{
			   GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("[네트워크 오류] 세션 서버가 꺼져있거나 Tailscale 연결을 확인해주세요!"));
			}
		}
		OnDone(bOk, Content);
	});
	Request->ProcessRequest();
}

FString USessionApiSubsystem::GetLocalIp() const
{
	if (!AdvertiseIp.IsEmpty())
	{
		return AdvertiseIp;
	}

	auto IsTailscaleIp = [](const FString& Ip)
	{
		TArray<FString> Octets;
		if (Ip.ParseIntoArray(Octets, TEXT(".")) != 4 || Octets[0] != TEXT("100")) return false;
		const int32 Second = FCString::Atoi(*Octets[1]);
		return Second >= 64 && Second <= 127;
	};

	TArray<FString> Candidates;
	Candidates.Add(FPaths::Combine(FPlatformMisc::GetEnvironmentVariable(TEXT("ProgramFiles")), TEXT("Tailscale/tailscale.exe")));
	Candidates.Add(TEXT("tailscale"));

	for (const FString& Candidate : Candidates)
	{
		// 후보가 .exe로 끝나는 경로인데 FileExists가 false
		if (Candidate.EndsWith(TEXT(".exe")) && !FPaths::FileExists(Candidate))
		{
			continue;
		}
		int32 Code = -1;
		FString StdOut, StdErr;
		if (FPlatformProcess::ExecProcess(*Candidate, TEXT("ip -4"), &Code, &StdOut, &StdErr) && Code == 0)
		{
			FString Ip = StdOut.TrimStartAndEnd();
			if (IsTailscaleIp(Ip))
			{
				return Ip;
			}
		}
	}
	
	ISocketSubsystem* Sockets = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!Sockets)
	{
		return TEXT("127.0.0.1");
	}

	TArray<TSharedPtr<FInternetAddr>> Addrs;
	if (Sockets->GetLocalAdapterAddresses(Addrs))
	{
		for (const TSharedPtr<FInternetAddr>& A : Addrs)
		{
			if (!A.IsValid()) continue; // TSharedPtr = nullptr방지(다음 A->에서 크래시 방지)
			FString Ip = A->ToString(false);
			Ip.RemoveFromStart(TEXT("::ffff:"));
			if (IsTailscaleIp(Ip))
			{
				return Ip;
			}
		}
	}
	bool bCanBindAll = false;
	TSharedPtr<FInternetAddr> Addr = Sockets->GetLocalHostAddr(*GLog, bCanBindAll);
	if (!Addr.IsValid())
	{
		return TEXT("127.0.0.1");
	}
	FString Ip = Addr->ToString(false);
	Ip.RemoveFromStart(TEXT("::ffff:"));
	return Ip == TEXT("0.0.0.0") ? TEXT("127.0.0.1") : Ip;
}
