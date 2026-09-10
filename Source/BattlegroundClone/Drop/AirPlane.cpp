#include "Drop/AirPlane.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"

#include "Character/DropCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/GameStateBase.h"
#include "Engine/Engine.h"


AAirPlane::AAirPlane()
{
	bReplicates = true;
	SetReplicateMovement(false); // 이동은 결정론적 lerp 로 각 머신이 계산 (AActor 이동 복제 안 씀)
	bAlwaysRelevant = true;      // 넷 릴러번시: 항상 복제 대상
	bNetLoadOnClient = false;    // 레벨(WP 셀)에서 로드하지 말고 서버가 클라로 동적 복제 → 큰 맵에서도 클라에 항상 존재

	PrimaryActorTick.bCanEverTick = true;
	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(RootScene);
	BodyMesh->SetRelativeRotation(MeshRotationOffset);
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Propeller = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Propeller"));
	Propeller->SetupAttachment(BodyMesh);
	Propeller->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	SeatPoint = CreateDefaultSubobject<USceneComponent>(TEXT("SeatPoint"));
	SeatPoint->SetupAttachment(RootScene);
	SeatPoint->SetRelativeLocation(FVector(-200.f, 0.f, -60.f)); // 에디터에서 위치 조정


}

void AAirPlane::BeginPlay()
{
	Super::BeginPlay();
	SetActorLocation(StartPoint);

	if (!StartPoint.Equals(EndPoint))
	{
		SetActorRotation((EndPoint - StartPoint).Rotation());
	}

	if (BodyMesh)
	{
		BodyMesh->SetRelativeRotation(MeshRotationOffset);
	}
	if (!HasAuthority())
	{
		return;
	}

	// 이륙은 첫 탑승 시점에 TryBoardAll 에서 시작 (여기선 탑승 재시도 타이머만 건다)
	GetWorldTimerManager().SetTimer(
		BoardTimerHandle, this, &AAirPlane::TryBoardAll, 0.25f, true, 0.f);
}

void AAirPlane::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AAirPlane, FlightAlpha);
	DOREPLIFETIME_CONDITION(AAirPlane, StartPoint, COND_InitialOnly);   // 스폰 시 1회만
	DOREPLIFETIME_CONDITION(AAirPlane, EndPoint,   COND_InitialOnly);
}

void AAirPlane::SetRoute(FVector InStart, FVector InEnd)
{
	StartPoint = InStart;
	EndPoint = InEnd;
}

void AAirPlane::TryBoardAll()
{
	bool bBoardedSomeone = false;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC) continue;

		ADropCharacter* Player = Cast<ADropCharacter>(PC->GetPawn());
		if (!Player) continue;

		if (Player->DropState == EDropState::Ground)   // 갓 스폰(지상 대기)인 사람만 태움. 뛰어내린 사람은 재탑승 X
		{
			BoardPassenger(Player);
			bBoardedSomeone = true;
		}
	}
	if (bBoardedSomeone && FlightStartTime < 0.f)
	{
		FlightStartTime = GetWorld()->GetTimeSeconds();   // 서버 로컬 시각 → 이륙
	}
	if (FlightAlpha > 0.5f)
	{
		GetWorldTimerManager().ClearTimer(BoardTimerHandle);
	}
}

void AAirPlane::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	TArray<UStaticMeshComponent*> MeshComps;
	GetComponents<UStaticMeshComponent>(MeshComps);
	for (UStaticMeshComponent* Comp : MeshComps)
	{
		if (Comp && Comp -> ComponentHasTag(TEXT("Propeller")))
		{
			Comp -> AddLocalRotation(FRotator(0.f, 0.f,  PropellerDegPerSec * DeltaSeconds));
		}
	}
	
	if (HasAuthority())
	{
		// 서버: 로컬 시각으로 진행률 계산 (이륙 전이면 FlightStartTime < 0 → 0 유지)
		if (FlightStartTime >= 0.f)
		{
			FlightAlpha = FMath::Clamp(
				(GetWorld()->GetTimeSeconds() - FlightStartTime) / FMath::Max(FlightDuration, 0.01f),
				0.f, 1.f);
		}
		SmoothAlpha = FlightAlpha;
	}
	else
	{
		// 클라: 매 프레임 전진 + 복제된 FlightAlpha 로 수렴 (시계 동기화 불필요)
		if (FlightAlpha <= 0.f)
		{
			SmoothAlpha = 0.f;   // 아직 이륙 전 → StartPoint 고정
		}
		else
		{
			SmoothAlpha = FMath::Clamp(SmoothAlpha + DeltaSeconds / FMath::Max(FlightDuration, 0.01f), 0.f, 1.f);
			SmoothAlpha = FMath::FInterpTo(SmoothAlpha, FlightAlpha, DeltaSeconds, 8.f);
		}
	}

	SetActorLocation(FMath::Lerp(StartPoint, EndPoint, SmoothAlpha));

	if (GEngine)
	{
		const int32 Key = HasAuthority() ? 8801 : 8802;
		const FColor Col = HasAuthority() ? FColor::Yellow : FColor::Cyan;
		GEngine->AddOnScreenDebugMessage(Key, 2.f, Col, FString::Printf(
			TEXT("[Plane] %s  loc=%s  repA=%.2f  smoothA=%.2f  dur=%.0f"),
			HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"),
			*GetActorLocation().ToCompactString(), FlightAlpha, SmoothAlpha, FlightDuration));
	}

	if (HasAuthority() && FlightAlpha >= 1.f && bDestroyOnArrival)
	{
		Destroy();
	}
}

void AAirPlane::BoardPassenger(ADropCharacter* Who)
{
	if (!HasAuthority()) return;
	if (Who && SeatPoint)
	{
		Who->EnterPlane(this, SeatPoint);
	}
}
