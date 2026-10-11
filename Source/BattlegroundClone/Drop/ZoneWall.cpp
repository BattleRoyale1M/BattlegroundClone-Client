#include "Drop/ZoneWall.h"

#include "Components/StaticMeshComponent.h"
#include "Core/DropGameState.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialInterface.h"

AZoneWall::AZoneWall()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false; // ★ 각자 GameState 값으로 그리기만 함

	WallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WallMesh"));
	RootComponent = WallMesh;
	WallMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WallMesh->SetCastShadow(false);
	WallMesh->SetHiddenInGame(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded()) WallMesh->SetStaticMesh(CylinderMesh.Object);
}

void AZoneWall::BeginPlay()
{
	Super::BeginPlay();

	if (UMaterialInterface* Mat = WallMaterial.LoadSynchronous())
	{
		WallMesh->SetMaterial(0, Mat);
	}
}

void AZoneWall::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const ADropGameState* GS = GetWorld()->GetGameState<ADropGameState>();
	FVector2D Center;
	float Radius;
	if (!GS || !GS->GetCurrentZone(Center, Radius))
	{
		WallMesh->SetHiddenInGame(true);
		return;
	}
	WallMesh->SetHiddenInGame(false);

	SetActorLocation(FVector(Center.X, Center.Y, WallBottomZ + WallHeight * 0.5f));
	const float XY = Radius / MeshRadius;
	SetActorScale3D(FVector(XY, XY, WallHeight / MeshHeight));
}
