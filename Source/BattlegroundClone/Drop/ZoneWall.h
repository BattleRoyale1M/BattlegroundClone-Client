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
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Zone")
	TObjectPtr<UStaticMeshComponent> WallMesh;

	// 생성자에서 로드하면 에디터에서 루트에 잡혀 머티리얼 편집 시 크래시 → 런타임 로드
	UPROPERTY(EditAnywhere, Category = "Zone")
	TSoftObjectPtr<UMaterialInterface> WallMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Materials/Zone/M_ZoneWall.M_ZoneWall")));

	UPROPERTY(EditAnywhere, Category = "Zone")
	float MeshRadius = 50.f;
	UPROPERTY(EditAnywhere, Category = "Zone")
	float MeshHeight = 100.f;

	UPROPERTY(EditAnywhere, Category = "Zone")
	float WallBottomZ = -10000.f;
	UPROPERTY(EditAnywhere, Category = "Zone")
	float WallHeight = 60000.f;
};