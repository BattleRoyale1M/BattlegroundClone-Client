#include "Drop/ZoneWall.h"

#include "Components/StaticMeshComponent.h"
#include "Core/DropGameState.h"

AZoneWall::AZoneWall()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false; // ★ 각자 GameState 값으로 그리기만 함

	WallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WallMesh"));
	RootComponent = WallMesh;
	WallMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WallMesh->SetCastShadow(false);
	WallMesh->SetHiddenInGame(true);
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
