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
	DOREPLIFETIME(AAirPlane, FlightAlpha);
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
	if (bBoardedSomeone && FlightStartTime < 0.f)
	{
		FlightStartTime = GetWorld()->GetTimeSeconds();
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
		if (FlightAlpha <= 0.f)
		{
			SmoothAlpha = 0.f;
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
