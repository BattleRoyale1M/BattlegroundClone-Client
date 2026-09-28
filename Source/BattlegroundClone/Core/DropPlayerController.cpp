#include "Core/DropPlayerController.h"

#include "UI/DropHUDWidget.h"
#include "UI/DropMapLabelWidget.h"

#include "Core/GameModes/DropGameMode.h"
#include "Drop/AirPlane.h"
#include "Character/DropCharacter.h"
#include "Interaction/Item/LootContainer.h"

#include "Blueprint/UserWidget.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
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
				if (GMClass->IsChildOf(ADropGameMode::StaticClass()))
				{
					GM = GMClass->GetDefaultObject<ADropGameMode>();
				}
			}
		}
	}
	if (GM)
	{
		GM->GetMapBounds(WorldMin, WorldMax);
		bBoundsResolved = true;
	}
}

void ADropPlayerController::SetGameplayHUDVisible(bool bVisible)
{
	if (UDropHUDWidget* HUD = Cast<UDropHUDWidget>(HUDWidget))
	{
		HUD->SetGameplayHUDVisible(bVisible);
	}
}

void ADropPlayerController::SetNavigationHUDVisible(bool bVisible)
{
	if (UDropHUDWidget* HUD = Cast<UDropHUDWidget>(HUDWidget))
	{
		HUD->SetNavigationHUDVisible(bVisible);
	}
}

void ADropPlayerController::ShowDeathUI()
{
	if (bInventoryOpen)
	{
		bInventoryOpen = false;
		CloseInventoryWidget();
	}
	SetGameplayHUDVisible(false);
	SetNavigationHUDVisible(false);
	if (DeathUIWidgetClass && !DeathUIWidget)
	{
		DeathUIWidget = CreateWidget<UUserWidget>(this, DeathUIWidgetClass);
		if (DeathUIWidget)
		{
			DeathUIWidget->AddToViewport(30);
		}
	}
	SetInputMode(FInputModeGameOnly());
	if (PlayerCameraManager)
	{
		PlayerCameraManager->StartCameraFade(
		0.f, 1.f, DeathFadeDuration, FLinearColor::Black,
		/*bFadeAudio=*/false, /*bHoldWhenFinished=*/true);
	}
}

void ADropPlayerController::BeginPlay()
{
	Super::BeginPlay();
	EnsureBounds();
	SetGameplayHUDVisible(false);

	if (!IsLocalController())
	{
		return;
	}
	
	const AGameStateBase* GS = GetWorld()->GetGameState();
	if (!GS || !GS->GameModeClass || !GS->GameModeClass->IsChildOf(ADropGameMode::StaticClass()))
	{
		return; // 로비 등 매치맵이 아니면 HUD 생성 스킵
	}

	if (HUDWidgetClass)
	{
		HUDWidget = CreateWidget<UUserWidget>(this, HUDWidgetClass);
		if (HUDWidget)
		{
			HUDWidget->AddToViewport(20);
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
		if (ToggleInventoryAction)
		{
			EIC->BindAction(ToggleInventoryAction, ETriggerEvent::Started, this, &ADropPlayerController::ToggleInventory);
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
	const ADropCharacter* Char = Cast<ADropCharacter>(GetPawn());
	if (Char && Char->bIsDead)
	{
		return;
	}
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

void ADropPlayerController::ToggleInventory()
{
	const ADropCharacter* Char = Cast<ADropCharacter>(GetPawn());
	if (Char && Char->bIsDead)
	{
		return;
	}

	if (bInventoryOpen)
	{
		CloseInventoryWidget();
	}
	else
	{
		ShowInventoryWidget();
	}
}

void ADropPlayerController::OpenLootScreen(ALootContainer* Container)
{
	if (!Container)
	{
		return;
	}

	const ADropCharacter* Char = Cast<ADropCharacter>(GetPawn());
	if (Char && Char->bIsDead)
	{
		return;
	}

	if (bInventoryOpen)
	{
		CloseInventoryWidget();
	}

	ActiveLootContainer = Container;
	ShowInventoryWidget();
}

void ADropPlayerController::ShowInventoryWidget()
{
	bInventoryOpen = true;

	if (InventoryWidgetClass)
	{
		InventoryWidget = CreateWidget<UUserWidget>(this, InventoryWidgetClass);
		if (InventoryWidget)
		{
			InventoryWidget->AddToViewport(10);
		}
	}
	FInputModeGameAndUI Mode;
	Mode.SetHideCursorDuringCapture(false);
	SetInputMode(Mode);
	SetShowMouseCursor(true);
}

void ADropPlayerController::CloseInventoryWidget()
{
	if (InventoryWidget)
	{
		InventoryWidget->RemoveFromParent();
		InventoryWidget = nullptr;
	}
	ActiveLootContainer = nullptr;
	bInventoryOpen = false;
	SetInputMode(FInputModeGameOnly());
	SetShowMouseCursor(false);
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

void ADropPlayerController::GetLocationLabels(float MapPixels, TArray<FVector2D>& OutPositions, TArray<FText>& OutNames) const
{
	OutPositions.Reset();
	OutNames.Reset();

	if (!LocationDataTable)
	{
		return;
	}

	EnsureBounds();
	const FVector2D MapSize(MapPixels, MapPixels);

	TArray<FMapLocationRow*> Rows;
	LocationDataTable->GetAllRows<FMapLocationRow>(TEXT("GetLocationLabels"), Rows);
	for (const FMapLocationRow* Row : Rows)
	{
		if (!Row)
		{
			continue;
		}
		const FVector2D N = UDropMapLibrary::WorldToNormalized(Row->WorldLocation, WorldMin, WorldMax);
		OutPositions.Add(UDropMapLibrary::NormalizedToWidget(FVector2D(N.Y, N.X), MapSize, true));
		OutNames.Add(Row->LocationName);
	}
}

void ADropPlayerController::RefreshLocationLabels(UCanvasPanel* LabelCanvas, TSubclassOf<UUserWidget> LabelWidgetClass, float MapPixels)
{
	if (!LabelCanvas || !LabelWidgetClass)
	{
		return;
	}
	LabelCanvas->ClearChildren();

	TArray<FVector2D> Positions;
	TArray<FText> Names;
	GetLocationLabels(MapPixels, Positions, Names);

	for (int32 i = 0; i < Positions.Num(); ++i)
	{
		UUserWidget* Label = CreateWidget<UUserWidget>(this, LabelWidgetClass);
		if (!Label)
		{
			continue;
		}
		if (UDropMapLabelWidget* MapLabel = Cast<UDropMapLabelWidget>(Label))
		{
			MapLabel->SetLabelText(Names[i]);
		}
		if (UCanvasPanelSlot* Slot = LabelCanvas->AddChildToCanvas(Label))
		{
			Slot->SetAutoSize(true);
			Slot->SetAlignment(FVector2D(0.5f, 0.5f));
			Slot->SetPosition(Positions[i]);
		}
	}
}

void ADropPlayerController::ShowCenterNotification(const FText& Line1, const FText& Line2, FLinearColor Line2Color)
{
	if (UDropHUDWidget* HUD = Cast<UDropHUDWidget>(HUDWidget))
	{
		HUD->ShowCenterNotification(Line1, Line2, Line2Color);
	}
}

void ADropPlayerController::ClientShowCenterNotification_Implementation(const FText& Line1, const FText& Line2,
	FLinearColor Line2Color)
{
	ShowCenterNotification(Line1, Line2, Line2Color);
}