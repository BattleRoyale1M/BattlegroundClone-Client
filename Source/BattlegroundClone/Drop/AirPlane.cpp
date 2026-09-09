#include "Drop/AirPlane.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"

#include "Character/DropCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"


AAirPlane::AAirPlane()
{
	bReplicates = true;
	SetReplicateMovement(true);
	bAlwaysRelevant = true;   // 클라가 폰 부착정보보다 비행기를 먼저 받도록 (부착 복제 레이스 방지)

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
	
	/*
	경과 시간 
	*/
	Elapsed = 0.f;
	if(!StartPoint.Equals(EndPoint))
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
	GetWorldTimerManager().SetTimerForNextTick([this]()
	{
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			if (APlayerController* PC = It->Get())
			{
				if (ADropCharacter* Player = Cast<ADropCharacter>(PC->GetPawn()))
				{
					BoardPassenger(Player);
				}
			}
		}
	});
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
	if (!HasAuthority()) return;
	Elapsed += DeltaSeconds;
	const float Alpha = FMath::Clamp(Elapsed / FlightDuration, 0.f, 1.f);
	SetActorLocation(FMath::Lerp(StartPoint, EndPoint, Alpha));
	
	if (Alpha >= 1.f && bDestroyOnArrival)
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
