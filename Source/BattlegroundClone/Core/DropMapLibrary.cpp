#include "Core/DropMapLibrary.h"

FVector2D UDropMapLibrary::WorldToNormalized(const FVector& World, FVector2D WorldMin, FVector2D WorldMax)
{
	return FVector2D
	(
	(World.X - WorldMin.X) / FMath::Max(WorldMax.X - WorldMin.X, 1.0),
	(World.Y - WorldMin.Y) / FMath::Max(WorldMax.Y - WorldMin.Y, 1.0)
	);
}

FVector UDropMapLibrary::NormalizedToWorld(FVector2D Normalized, FVector2D WorldMin, FVector2D WorldMax, float Z)
{
	return FVector
	(
		FMath::Lerp(WorldMin.X, WorldMax.X, Normalized.X),
		FMath::Lerp(WorldMin.Y, WorldMax.Y, Normalized.Y),
		Z
	);
}

FVector2D UDropMapLibrary::NormalizedToWidget(FVector2D Normalized, FVector2D WidgetSize, bool bFlipY)
{
	const double Y = bFlipY ? (1.0 - Normalized.Y) : Normalized.Y;
	return FVector2D
	(
		Normalized.X * WidgetSize.X,
		Y*WidgetSize.Y
	);
}

float UDropMapLibrary::YawToCompass(float WorldYaw, float NorthYawOffset)
{
	return FRotator::ClampAxis(WorldYaw - NorthYawOffset);
}

// .Rotation().Yaw : 지평선 기준으로 몇 도 회전되어 있는가를 언리얼 좌표계로 뽑
float UDropMapLibrary::BearingDeg(const FVector& From, const FVector& To, float NorthYawOffset)
{
	FVector Direction = To - From;
	return FRotator::ClampAxis(Direction.Rotation().Yaw - NorthYawOffset);
}

float UDropMapLibrary::CmSToKmh(float CmPerSec)
{
	return CmPerSec * 0.036;
}

float UDropMapLibrary::KmhToCmS(float Kmh)
{
	return Kmh * 27.7778f;
}

