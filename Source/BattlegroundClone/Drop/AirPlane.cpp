#include "Drop/AirPlane.h"
#include "GameFramework/GameStateBase.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"

#include "Character/DropCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"


AAirPlane::AAirPlane()
{
	bReplicates = true;
	SetReplicateMovement(false);
	bAlwaysRelevant = true;
	bNetLoadOnClient = false;

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
	
	TArray<UStaticMeshComponent*> MeshComps;
	GetComponents<UStaticMeshComponent>(MeshComps);
	for (UStaticMeshComponent* Comp : MeshComps)
	{
		if (Comp && Comp->ComponentHasTag(TEXT("Propeller")))
		{
			PropellerComps.Add(Comp);
		}
	}
	
	if (!HasAuthority())
	{
		return;
	}

	GetWorldTimerManager().SetTimer(
		BoardTimerHandle, this, &AAirPlane::TryBoardAll, 0.25f, true, 0.f);
}

void AAirPlane::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AAirPlane, FlightStartServerTime);
	DOREPLIFETIME_CONDITION(AAirPlane, StartPoint, COND_InitialOnly);
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

		if (Player->DropState == EDropState::Ground)
		{
			BoardPassenger(Player);
			bBoardedSomeone = true;
		}
	}
	if (bBoardedSomeone && FlightStartServerTime < 0.f)
	{
		FlightStartServerTime = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
	}
	if (FlightStartServerTime >= 0.f &&
	GetWorld()->GetGameState()->GetServerWorldTimeSeconds() - FlightStartServerTime > FlightDuration * 0.5f)
	{
		GetWorldTimerManager().ClearTimer(BoardTimerHandle);
	}
}

void AAirPlane::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	for (UStaticMeshComponent* Comp : PropellerComps)
	{
		Comp->AddLocalRotation(FRotator(0.f, 0.f, PropellerDegPerSec * DeltaSeconds));
	}

	const AGameStateBase* GS = GetWorld()->GetGameState();
	const float TargetTime = GS ? GS->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
	if (HasAuthority() || SmoothedServerTime < 0.f || FMath::Abs(TargetTime - SmoothedServerTime) > 1.f)
	{
		SmoothedServerTime = TargetTime;
	}
	else
	{
		SmoothedServerTime = FMath::FInterpTo(SmoothedServerTime + DeltaSeconds, TargetTime, DeltaSeconds, 1.f);
	}
	const float Now = SmoothedServerTime;
	const float Alpha = FlightStartServerTime < 0.f ? 0.f
		: FMath::Clamp((Now - FlightStartServerTime) / FlightDuration, 0.f, 1.f);
	SetActorLocation(FMath::Lerp(StartPoint, EndPoint, Alpha));

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
