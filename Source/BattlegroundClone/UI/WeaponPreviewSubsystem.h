#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WeaponPreviewSubsystem.generated.h"

class AWeaponBase;
class UImage;
class USceneCaptureComponent2D;
class UStaticMeshComponent;
class UTextureRenderTarget2D;
class UMaterialInterface;
class UMaterialInstanceDynamic;

USTRUCT()
struct FWeaponPreviewEntry
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY()
	TObjectPtr<UTextureRenderTarget2D> RenderTarget;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> MID;

	float OrthoWidth = 100.f;
};

UCLASS()
class BATTLEGROUNDCLONE_API UWeaponPreviewSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "UI|WeaponPreview", meta = (WorldContext = "WorldContextObject"))
	static bool ApplyWeaponPreview(const UObject* WorldContextObject, UImage* TargetImage, TSubclassOf<AWeaponBase> WeaponClass);

protected:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Deinitialize() override;

private:
	UMaterialInstanceDynamic* GetOrCreatePreview(TSubclassOf<AWeaponBase> WeaponClass);
	void EnsureStudio();
	void Capture(FWeaponPreviewEntry& Entry);
	void RecaptureAll();

	UPROPERTY()
	TObjectPtr<AActor> StudioActor;

	UPROPERTY()
	TObjectPtr<USceneCaptureComponent2D> StudioCapture;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> PreviewMaterial;

	UPROPERTY()
	TMap<TSubclassOf<AWeaponBase>, FWeaponPreviewEntry> Cache;

	FTimerHandle RecaptureTimer;
	int32 RecapturesLeft = 0;
	
	static constexpr float StudioHeight = 200000.f;
	static constexpr int32 RenderWidth = 512;
	static constexpr int32 RenderHeight = 192;
};
