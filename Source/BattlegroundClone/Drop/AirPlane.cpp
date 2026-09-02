#include "Drop/AirPlane.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"

#include "Character/DropCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"


AAirPlane::AAirPlane()
{
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
	
	GetWorldTimerManager().SetTimerForNextTick([this]()
	{
		if (ADropCharacter* Player = Cast<ADropCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)))
		{
			BoardPassenger(Player);
		}
	});
	
}

void AAirPlane::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	Elapsed += DeltaSeconds;
	const float Alpha = FMath::Clamp(Elapsed / FlightDuration, 0.f, 1.f);
	SetActorLocation(FMath::Lerp(StartPoint, EndPoint, Alpha));
	
	TArray<UStaticMeshComponent*> MeshComps;
	GetComponents<UStaticMeshComponent>(MeshComps);
	for (UStaticMeshComponent* Comp : MeshComps)
	{
		if (Comp && Comp -> ComponentHasTag(TEXT("Propeller")))
		{
			Comp -> AddLocalRotation(FRotator(0.f, 0.f,  PropellerDegPerSec * DeltaSeconds));
		}
	}
	if (Alpha >= 1.f && bDestroyOnArrival)
	{
		Destroy();
	}
}

void AAirPlane::BoardPassenger(ADropCharacter* Who)
{
	if (Who && SeatPoint)
	{
		Who->EnterPlane(this, SeatPoint);
	}
}
