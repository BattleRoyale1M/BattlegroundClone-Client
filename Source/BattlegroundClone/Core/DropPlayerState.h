#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "DropPlayerState.generated.h"

UCLASS()
class BATTLEGROUNDCLONE_API ADropPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Map")
	FLinearColor MarkerColor = FLinearColor::White;
};
