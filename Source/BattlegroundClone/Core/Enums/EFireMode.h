#pragma once

UENUM(BlueprintType)
enum class EFireMode : uint8
{
	Single      UMETA(DisplayName = "단발"),
	Burst       UMETA(DisplayName = "점사"),
	Automatic   UMETA(DisplayName = "연사")
};
