// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BattlegroundClonePlayerController.generated.h"

class UInputMappingContext;

/**
 *  Basic PlayerController class for a third person game
 *  Manages input mappings
 */
UCLASS(abstract)
class ABattlegroundClonePlayerController : public APlayerController
{
	GENERATED_BODY()

protected:

	/** Input Mapping Contexts added for the local player on input setup */
	UPROPERTY(EditAnywhere, Category ="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

};
