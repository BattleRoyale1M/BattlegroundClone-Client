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

void ADropPlayerController::GetWorldMapView(float MapPixels, FVector2D& OutSelfPos, float& OutSelfAngle, bool& bMarkerValid, FVector2D& OutMarkerPos) const
{
	const FVector2D MapSize(MapPixels, MapPixels);
	const FVector SelfLoc = GetSelfMapLocation();
	const FVector2D NormSelf = UDropMapLibrary::WorldToNormalized(SelfLoc, WorldMin, WorldMax);
	OutSelfPos = UDropMapLibrary::NormalizedToWidget(FVector2D(NormSelf.Y, NormSelf.X), MapSize, true);
	OutSelfAngle = PlayerCameraManager ? PlayerCameraManager->GetCameraRotation().Yaw : 0.f;
	const FVector2D NormMarker = GetMarkerNormalized(bMarkerValid);
	OutMarkerPos = UDropMapLibrary::NormalizedToWidget(FVector2D(NormMarker.Y, NormMarker.X), MapSize, true);
}

void ADropPlayerController::GetFlightPathLine(float MapPixels, FVector2D& OutMid,
	float& OutLength, float& OutAngle, bool& bHasPath) const
{
	const FVector2D MapSize(MapPixels, MapPixels);
	FVector FS, FE;
	bHasPath = GetFlightPath(FS, FE);
	if (!bHasPath)
	{
		OutMid = FVector2D::ZeroVector;
		OutLength = 0.f;
		OutAngle = 0.f;
		return;
	}
	const FVector2D NS = UDropMapLibrary::WorldToNormalized(FS, WorldMin, WorldMax);
	const FVector2D NE = UDropMapLibrary::WorldToNormalized(FE, WorldMin, WorldMax);
	const FVector2D PS = UDropMapLibrary::NormalizedToWidget(FVector2D(NS.Y, NS.X), MapSize, true);
	const FVector2D PE = UDropMapLibrary::NormalizedToWidget(FVector2D(NE.Y, NE.X), MapSize, true);
	OutMid = (PS + PE) * 0.5f;
	const FVector2D D = PE - PS;
	OutLength = D.Size();
	OutAngle = FMath::RadiansToDegrees(FMath::Atan2(D.Y, D.X));
}