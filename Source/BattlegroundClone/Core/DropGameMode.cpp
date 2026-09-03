#include "Core/DropGameMode.h"
#include "Character/DropCharacter.h"
#include "Core/DropPlayerController.h"

ADropGameMode::ADropGameMode()
{
	DefaultPawnClass = ADropCharacter::StaticClass();
	PlayerControllerClass = ADropPlayerController::StaticClass();

}

void ADropGameMode::GetMapBounds(FVector2D& OutWorldMin, FVector2D& OutWorldMax) const
{
	OutWorldMin = WorldMin;
	OutWorldMax = WorldMax;
}
