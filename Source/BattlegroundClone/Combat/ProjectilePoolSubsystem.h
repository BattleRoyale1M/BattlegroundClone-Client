#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectilePoolSubsystem.generated.h"

UCLASS()
class BATTLEGROUNDCLONE_API AProjectilePoolSubsystem : public AActor
{
	GENERATED_BODY()
	
public:
	AProjectilePoolSubsystem();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

};
