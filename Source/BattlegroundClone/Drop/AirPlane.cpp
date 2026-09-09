#include "Drop/AirPlane.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"

#include "Character/DropCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/GameStateBase.h"


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

	// 서버: 비행 시작 시각 기록 (복제되어 클라도 같은 타임라인 사용)
	if (const AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr)
	{
		FlightStartServerTime = GS->GetServerWorldTimeSeconds();
	}

	GetWorldTimerManager().SetTimer(
		BoardTimerHandle, this, &AAirPlane::TryBoardAll, 0.25f, true, 0.f);
}

void AAirPlane::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AAirPlane, FlightStartServerTime);
}

float AAirPlane::GetFlightAlpha() const
{
	const AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (!GS || FlightStartServerTime < 0.f)
	{
		return 0.f;
	}
	const float FlightElapsed = GS->GetServerWorldTimeSeconds() - FlightStartServerTime;
	return FMath::Clamp(FlightElapsed / FMath::Max(FlightDuration, 0.01f), 0.f, 1.f);
}

void AAirPlane::TryBoardAll()
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC) continue;

		ADropCharacter* Player = Cast<ADropCharacter>(PC->GetPawn());
		if (!Player) continue;   // 아직 possess 안 됨 → 다음 틱에 다시

		if (Player->DropState == EDropState::Ground)   // 갓 스폰(지상 대기)인 사람만 태움. 뛰어내린 사람은 재탑승 X
		{
			BoardPassenger(Player);
		}
	}

	// 비행 절반 지나면 탑승 마감 → 재시도 중단
	if (GetFlightAlpha() > 0.5f)
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
	// 이동: 전 머신이 복제된 시작 시각 기준으로 동일하게 계산
	const float Alpha = GetFlightAlpha();
	SetActorLocation(FMath::Lerp(StartPoint, EndPoint, Alpha));

	// 도착 처리는 서버만
	if (HasAuthority() && Alpha >= 1.f && bDestroyOnArrival)
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
