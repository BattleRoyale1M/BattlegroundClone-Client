#include "Core/DropPlayerController.h"

#include "Core/DropGameMode.h"
#include "Drop/AirPlane.h"

#include "Blueprint/UserWidget.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

ADropPlayerController::ADropPlayerController()
{
}

void ADropPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (const ADropGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ADropGameMode>() : nullptr)
	{
		GM->GetMapBounds(WorldMin, WorldMax);
	}

	if (!IsLocalController())
	{
		return;
	}

	if (HUDWidgetClass)
	{
		HUDWidget = CreateWidget<UUserWidget>(this, HUDWidgetClass);
		if (HUDWidget)
		{
			HUDWidget->AddToViewport(0);
		}
	}

	if (WorldMapWidgetClass)
	{
		WorldMapWidget = CreateWidget<UUserWidget>(this, WorldMapWidgetClass);
		if (WorldMapWidget)
		{
			WorldMapWidget->AddToViewport(10);
			WorldMapWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void ADropPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (MapAction)
		{
			EIC->BindAction(MapAction, ETriggerEvent::Started, this, &ADropPlayerController::ToggleWorldMap);
		}
	}
}

void ADropPlayerController::GetMapBounds(FVector2D& OutWorldMin, FVector2D& OutWorldMax) const
{
	OutWorldMin = WorldMin;
	OutWorldMax = WorldMax;
}

FVector ADropPlayerController::GetSelfMapLocation() const
{
	if (const APawn* P = GetPawn())
	{
		return P->GetActorLocation();
	}
	if (const AAirPlane* Plane = FindPlane())
	{
		return Plane->GetActorLocation();
	}
	return FVector::ZeroVector;
}

bool ADropPlayerController::GetFlightPath(FVector& OutStart, FVector& OutEnd) const
{
	if (const AAirPlane* Plane = FindPlane())
	{
		OutStart = Plane->GetFlightStart();
		OutEnd = Plane->GetFlightEnd();
		return true;
	}
	OutStart = FVector::ZeroVector;
	OutEnd = FVector::ZeroVector;
	return false;
}

FVector2D ADropPlayerController::GetMarkerNormalized(bool& bValid) const
{
	bValid = bHasMarker;
	return MarkerNormalized;
}

void ADropPlayerController::SetMarkerNormalized(FVector2D Normalized)
{
	MarkerNormalized = FVector2D(FMath::Clamp(Normalized.X, 0.0, 1.0),
	                             FMath::Clamp(Normalized.Y, 0.0, 1.0));
	bHasMarker = true;
}

void ADropPlayerController::ClearMarker()
{
	bHasMarker = false;
}

void ADropPlayerController::ToggleWorldMap()
{
	bWorldMapOpen = !bWorldMapOpen;

	if (WorldMapWidget)
	{
		WorldMapWidget->SetVisibility(bWorldMapOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	if (bWorldMapOpen)
	{
		FInputModeGameAndUI Mode;
		Mode.SetHideCursorDuringCapture(false);
		SetInputMode(Mode);
		SetShowMouseCursor(true);
	}
	else
	{
		SetInputMode(FInputModeGameOnly());
		SetShowMouseCursor(false);
	}
}

AAirPlane* ADropPlayerController::FindPlane() const
{
	return Cast<AAirPlane>(UGameplayStatics::GetActorOfClass(GetWorld(), AAirPlane::StaticClass()));
}
