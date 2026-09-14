#pragma once

#include "CoreMinimal.h"
#include "EFireMode.generated.h"

UENUM(BlueprintType)
enum class EFireMode : uint8
{
	Single      UMETA(DisplayName = "단발"),
	Automatic   UMETA(DisplayName = "연사")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFireModeChanged, EFireMode, NewFireMode);