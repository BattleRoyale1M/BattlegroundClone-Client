#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DynamicCrosshairWidget.generated.h"

class AWeaponBase;

UCLASS()
class BATTLEGROUNDCLONE_API UDynamicCrosshairWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> LineTop;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> LineBottom;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> LineLeft;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> LineRight;

	UPROPERTY(EditAnywhere, Category = "Crosshair", meta = (ClampMin = "0.0"))
	float BaseGap = 6.f;

	UPROPERTY(EditAnywhere, Category = "Crosshair", meta = (ClampMin = "0.0"))
	float MaxSpread = 60.f;

	UPROPERTY(EditAnywhere, Category = "Crosshair", meta = (ClampMin = "0.0"))
	float MoveSpread = 18.f;

	UPROPERTY(EditAnywhere, Category = "Crosshair", meta = (ClampMin = "1.0"))
	float MoveSpeedForFullSpread = 500.f;

	UPROPERTY(EditAnywhere, Category = "Crosshair", meta = (ClampMin = "0.0"))
	float AirSpread = 25.f;

	UPROPERTY(EditAnywhere, Category = "Crosshair", meta = (ClampMin = "0.0"))
	float FireKick = 8.f;

	UPROPERTY(EditAnywhere, Category = "Crosshair", meta = (ClampMin = "0.0"))
	float FireKickMax = 30.f;

	UPROPERTY(EditAnywhere, Category = "Crosshair", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ProneMultiplier = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Crosshair", meta = (ClampMin = "0.0"))
	float ExpandSpeed = 20.f;

	UPROPERTY(EditAnywhere, Category = "Crosshair", meta = (ClampMin = "0.0"))
	float ContractSpeed = 8.f;

	UPROPERTY(EditAnywhere, Category = "Crosshair", meta = (ClampMin = "0.0"))
	float FireRecoverSpeed = 6.f;

private:
	float CurrentSpread = 0.f;
	float FireSpread = 0.f;
	int32 LastAmmo = -1;
	TWeakObjectPtr<AWeaponBase> LastWeapon;

	void ApplySpread(float Spread);
};
