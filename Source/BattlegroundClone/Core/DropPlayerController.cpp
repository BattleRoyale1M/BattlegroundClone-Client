#include "Core/DropPlayerController.h"

#include "Core/DropGameMode.h"
#include "Drop/AirPlane.h"

#include "Blueprint/UserWidget.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/UserWidget.h"

#include "Core/DropPlayerState.h"
#include "Core/DropMapLibrary.h"
#include "GameFramework/GameStateBase.h"

ADropPlayerController::ADropPlayerController()
{
}

void ADropPlayerController::EnsureBounds() const
{
	if (bBoundsResolved || !GetWorld())
	{
		return;
	}

	const ADropGameMode* GM = GetWorld()->GetAuthGameMode<ADropGameMode>(); 
	if (!GM)
	{
		if (const AGameStateBase* GS = GetWorld()->GetGameState())
		{
			if (UClass* GMClass = GS->GameModeClass)
			{
				GM = GMClass->GetDefaultObject<ADropGameMode>();
			}
		}
	}
	if (GM)
	{
		GM->GetMapBounds(WorldMin, WorldMax);
		bBoundsResolved = true;
	}
}

void ADropPlayerController::BeginPlay()
{
	Super::BeginPlay();

	EnsureBounds();

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
	EnsureBounds();
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

void ADropPlayerController::GetPlayerMarkers(float MapPixels, bool bIncludeSelf,
	TArray<FVector2D>& OutPositions, TArray<FLinearColor>& OutColors) const
{
	OutPositions.Reset();
	OutColors.Reset();

	const AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (!GS)
	{
		return;
	}

	const FVector2D MapSize(MapPixels, MapPixels);

	for (APlayerState* PS : GS->PlayerArray)
	{
		const ADropPlayerState* DPS = Cast<ADropPlayerState>(PS);
		if (!DPS)
		{
			continue;
		}
		if (!bIncludeSelf && PS == PlayerState)
		{
			continue;
		}

		const APawn* Pawning = PS->GetPawn();
		if (!Pawning)
		{
			continue;
		}
		
		const FVector2D N = UDropMapLibrary::WorldToNormalized(Pawning->GetActorLocation(), WorldMin, WorldMax);
		OutPositions.Add(UDropMapLibrary::NormalizedToWidget(FVector2D(N.Y, N.X), MapSize, true));
		OutColors.Add(DPS->MarkerColor);
	}
}

void ADropPlayerController::RefreshPlayerMarkers(class UCanvasPanel* MarkerCanvas, TSubclassOf<UUserWidget> MarkerClass,
	float MapPixels, bool bIncludeSelf)
{
	if (!MarkerCanvas || !MarkerClass)
	{
		return;
	}
	MarkerCanvas -> ClearChildren();
	TArray<FVector2D> Positions;
	TArray<FLinearColor> Colors;
	GetPlayerMarkers(MapPixels, bIncludeSelf, Positions, Colors);
	for (int32 i = 0; i<Positions.Num(); ++i)
	{
		UUserWidget* Marker = CreateWidget<UUserWidget>(this, MarkerClass);
		if (!Marker)
		{
			continue;
		}
		Marker->SetColorAndOpacity(Colors[i]);
		if (UCanvasPanelSlot* Slot = MarkerCanvas -> AddChildToCanvas(Marker)) // 생성한 마커를 지도 캔버스 자식으로 붙인다.
		{
			Slot->SetAutoSize(true);
			Slot->SetAlignment(FVector2D(0.5f, 0.5f));
			Slot->SetPosition(Positions[i]); // 지도 캔버스 내의 해당 X, Y 좌표 위치로 마커를 이동
		}
	}
}
