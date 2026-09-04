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

	// 비행 경로를 맵 경계(WorldMin~WorldMax)까지 연장해 항상 지도를 끝에서 끝까지 관통시킨다.
	{
		const FVector2D Origin(FS.X, FS.Y);
		const FVector2D Dir = FVector2D(FE.X - FS.X, FE.Y - FS.Y).GetSafeNormal();
		if (!Dir.IsNearlyZero())
		{
			double TMin = -TNumericLimits<double>::Max();
			double TMax =  TNumericLimits<double>::Max();
			const double MinA[2]    = { WorldMin.X, WorldMin.Y };
			const double MaxA[2]    = { WorldMax.X, WorldMax.Y };
			const double OriginA[2] = { Origin.X, Origin.Y };
			const double DirA[2]    = { Dir.X, Dir.Y };
			for (int32 Axis = 0; Axis < 2; ++Axis)
			{
				if (FMath::Abs(DirA[Axis]) < UE_KINDA_SMALL_NUMBER)
				{
					continue;
				}
				double T1 = (MinA[Axis] - OriginA[Axis]) / DirA[Axis];
				double T2 = (MaxA[Axis] - OriginA[Axis]) / DirA[Axis];
				if (T1 > T2)
				{
					Swap(T1, T2);
				}
				TMin = FMath::Max(TMin, T1);
				TMax = FMath::Min(TMax, T2);
			}
			if (TMax > TMin)
			{
				FS = FVector(Origin + Dir * TMin, FS.Z);
				FE = FVector(Origin + Dir * TMax, FE.Z);
			}
		}
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