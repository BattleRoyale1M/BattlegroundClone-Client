#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZoneWall.generated.h"

UCLASS()
class BATTLEGROUNDCLONE_API AZoneWall : public AActor
{
	GENERATED_BODY()

public:
	AZoneWall();
	virtual void Tick(float DeltaSeconds) override;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Zone")
	TObjectPtr<UStaticMeshComponent> WallMesh;
	
	UPROPERTY(EditAnywhere, Category = "Zone")
	float MeshRadius = 50.f;
	UPROPERTY(EditAnywhere, Category = "Zone")
	float MeshHeight = 100.f;

	UPROPERTY(EditAnywhere, Category = "Zone")
	float WallBottomZ = -10000.f;
	UPROPERTY(EditAnywhere, Category = "Zone")
	float WallHeight = 60000.f;
};