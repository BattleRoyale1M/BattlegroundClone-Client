#include "Core/DropGameMode.h"

#include "DropMapLibrary.h"
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

void ADropPlayerController::GetMinimapView(float MapPixels, float ViewportPixels, FVector2D& OutPan, float& OutSelfAngle, bool& bMarkerValid, FVector2D& OutMarkerPos) const
{
	const FVector2D MapSize(MapPixels, MapPixels);
	const FVector SelfLoc = GetSelfMapLocation();
	const FVector2D NormSelf = UDropMapLibrary::WorldToNormalized(SelfLoc, WorldMin, WorldMax);
	const FVector2D SelfPx = UDropMapLibrary::NormalizedToWidget(FVector2D(NormSelf.Y, NormSelf.X), MapSize, true);
	OutPan = FVector2D(ViewportPixels * 0.5f, ViewportPixels * 0.5f) - SelfPx;
	OutSelfAngle = PlayerCameraManager ? PlayerCameraManager->GetCameraRotation().Yaw : 0.f;
	const FVector2D NormMarker = GetMarkerNormalized(bMarkerValid);
	OutMarkerPos = UDropMapLibrary::NormalizedToWidget(FVector2D(NormMarker.Y, NormMarker.X), MapSize, true);
}