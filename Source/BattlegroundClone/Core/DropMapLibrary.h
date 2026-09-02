#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DropMapLibrary.generated.h"

UCLASS()
class BATTLEGROUNDCLONE_API UDropMapLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	
	UFUNCTION(BlueprintPure, Category = "DropMap")
	static FVector2D WorldToNormalized(const FVector& World, FVector2D WorldMin, FVector2D WorldMax);
	
	UFUNCTION(BlueprintPure, Category = "DropMap")
	static FVector NormalizedToWorld(FVector2D Normalized, FVector2D WorldMin, FVector2D WorldMax, float Z = 0.f);
	
	UFUNCTION(BlueprintPure, Category = "DropMap")
	static FVector2D NormalizedToWidget(FVector2D Normalized, FVector2D WidgetSize, bool bFlipY = true);
	
	UFUNCTION(BlueprintPure, Category = "DropMap")
	static float YawToCompass(float WorldYaw, float NorthYawOffset = 0.f);
	
	/*
	상단 나침반 위에 핑 아이콘을 정렬할 때 사용
	*/
	UFUNCTION(BlueprintPure, Category = "DropMap")
	static float BearingDeg(const FVector& From, const FVector& To, float NorthYawOffset = 0.f);
	
	UFUNCTION(BlueprintPure, Category = "DropMap")
	static float CmSToKmh(float CmPerSec);

	UFUNCTION(BlueprintPure, Category = "DropMap")
	static float KmhToCmS(float Kmh);
};
