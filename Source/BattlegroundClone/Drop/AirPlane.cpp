#include "Drop/AirPlane.h"
#include "Components/StaticMeshComponent.h"


AAirPlane::AAirPlane()
{
	PrimaryActorTick.bCanEverTick = true;
	
	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	SetRootComponent(BodyMesh);
	BodyMesh -> SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	Propeller = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Propeller"));
	Propeller -> SetupAttachment(BodyMesh);
	Propeller -> SetCollisionEnabled(ECollisionEnabled::NoCollision);
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

