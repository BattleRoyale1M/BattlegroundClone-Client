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

UENUM(BlueprintType)
enum class EDropAimMode : uint8
{
	
	Hip      UMETA(DisplayName = "Hip"),        // 비조준 (3인칭 기본)
	Shoulder UMETA(DisplayName = "Shoulder"),   // 우클릭 홀드 (3인칭 어깨너머)
	Scoped   UMETA(DisplayName = "Scoped"),     // 우클릭 탭 (1인칭 정조준)
};