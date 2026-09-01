#pragma once
#include "CoreMinimal.h"
#include "DropTypes.generated.h"

UENUM(BlueprintType)
enum class EDropState : uint8
{
	Ground UMETA(DisplayName = "Ground"),
	InPlane UMETA(DisplayName = "InPlane"),
	Freefall UMETA(DisplayName = "Freefall"),
	Parachuting UMETA(DisplayName = "Parachuting"),
};