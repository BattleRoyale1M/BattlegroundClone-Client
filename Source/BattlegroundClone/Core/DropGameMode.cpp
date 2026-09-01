#include "Core/DropGameMode.h"
#include "Character/DropCharacter.h"

ADropGameMode::ADropGameMode()
{
	DefaultPawnClass = ADropCharacter::StaticClass();
}