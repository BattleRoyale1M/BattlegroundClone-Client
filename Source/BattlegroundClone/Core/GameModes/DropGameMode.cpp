#include "Core/GameModes/DropGameMode.h"

#include "Drop/AirPlane.h"
#include "Kismet/GameplayStatics.h"

#include "Core/DropMapLibrary.h"
#include "Character/DropCharacter.h"
#include "Core/DropPlayerController.h"
#include "Core/DropPlayerState.h"
#include "Core/DropGameState.h"
#include "TimerManager.h"
#include "AIController.h"
#include "Weapon/WeaponBase.h"

ADropGameMode::ADropGameMode()
{
	DefaultPawnClass = ADropCharacter::StaticClass();
	PlayerControllerClass = ADropPlayerController::StaticClass();
	PlayerStateClass = ADropPlayerState::StaticClass();
	GameStateClass = ADropGameState::StaticClass();
}

void ADropGameMode::RegisterCombatant(ADropCharacter* Character)
{
	if (!Character || bMatchOver || AliveCombatants.Contains(Character))
	{
		return;
	}
	if (Cast<AAIController>(Character->GetController()) || Character->IsNetStartupActor())
	{
		if (MaxBots >= 0 && RegisteredBots >= MaxBots)
		{
			ExtraBots.Add(Character);
			GetWorldTimerManager().SetTimerForNextTick(this, &ADropGameMode::RemoveExtraBots);
			return;
		}
		++RegisteredBots;
	}
	AliveCombatants.Add(Character);
	if (ADropGameState* GS = GetGameState<ADropGameState>())
	{
		GS->TotalCount++;
	}
	SyncAliveCount();
}

void ADropGameMode::RemoveExtraBots()
{
	for (const TWeakObjectPtr<ADropCharacter>& Bot : ExtraBots)
	{
		ADropCharacter* BotChar = Bot.Get();
		if (!BotChar)
		{
			continue;
		}
		TArray<AActor*> Attached;
		BotChar->GetAttachedActors(Attached, true, true);
		for (AActor* Child : Attached)
		{
			if (Cast<AWeaponBase>(Child))
			{
				Child->Destroy();
			}
		}
		BotChar->Destroy();
	}
	ExtraBots.Reset();
}

void ADropGameMode::NotifyCombatantDied(ADropCharacter* Victim)
{
	if (!Victim || !AliveCombatants.Contains(Victim))
	{
		return;
	}
	const int32 Placement = AliveCombatants.Num();
	AliveCombatants.Remove(Victim);
	SyncAliveCount();

	if (ADropPlayerController* PC = Cast<ADropPlayerController>(Victim->GetController()))
	{
		const ADropGameState* GS = GetGameState<ADropGameState>();
		const ADropPlayerState* PS = PC->GetPlayerState<ADropPlayerState>();
		PC->ClientShowMatchResult(false, Placement, GS ? GS->TotalCount : Placement, PS ? PS->KillCount : 0);
	}
	CheckForWinner();
}

void ADropGameMode::RemoveCombatant(ADropCharacter* Character)
{
	if (AliveCombatants.Remove(Character) > 0)
	{
		SyncAliveCount();
		CheckForWinner();
	}
}

void ADropGameMode::SyncAliveCount()
{
	AliveCombatants.RemoveAll([](const TWeakObjectPtr<ADropCharacter>& C) { return !C.IsValid(); });
	if (ADropGameState* GS = GetGameState<ADropGameState>())
	{
		GS->AliveCount = AliveCombatants.Num();
	}
}

void ADropGameMode::CheckForWinner()
{
	if (bMatchOver || AliveCombatants.Num() != 1)
	{
		return;
	}
	bMatchOver = true;
	Winner = AliveCombatants[0];
	GetWorldTimerManager().SetTimer(VictoryTimerHandle, this, &ADropGameMode::AnnounceWinner, VictoryDelay, false);
}

void ADropGameMode::AnnounceWinner()
{
	ADropCharacter* WinnerChar = Winner.Get();
	if (!WinnerChar)
	{
		return;
	}
	if (ADropPlayerController* PC = Cast<ADropPlayerController>(WinnerChar->GetController()))
	{
		const ADropGameState* GS = GetGameState<ADropGameState>();
		const ADropPlayerState* PS = PC->GetPlayerState<ADropPlayerState>();
		PC->ClientShowMatchResult(true, 1, GS ? GS->TotalCount : 1, PS ? PS->KillCount : 0);
	}
}

void ADropGameMode::GetMapBounds(FVector2D& OutWorldMin, FVector2D& OutWorldMax) const
{
	OutWorldMin = WorldMin;
	OutWorldMax = WorldMax;
}

/*
Player 마커 컬러 배정
*/
void ADropGameMode::OnPostLogin(AController* NewPlayer)
{
	Super::OnPostLogin(NewPlayer);
	
	if (ADropPlayerState* PS = NewPlayer -> GetPlayerState<ADropPlayerState>())
	{
		if (MarkerPalette.Num() > 0)
		{
			PS->MarkerColor = MarkerPalette[NextMarkerIndex++ % MarkerPalette.Num()];
		}
	}
}

void ADropGameMode::BeginPlay()
{
	Super::BeginPlay();
	if (!PlaneClass)
	{
		return;
	}
	
	const FTransform Xform(FRotator::ZeroRotator, PlaneRouteStart);
	if (AAirPlane* Plane = GetWorld()->SpawnActorDeferred<AAirPlane>(
			PlaneClass, Xform, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
	{
		Plane->SetRoute(PlaneRouteStart, PlaneRouteEnd);
		UGameplayStatics::FinishSpawningActor(Plane, Xform);
	}
}

void ADropPlayerController::GetMinimapView(float MapPixels, float ViewportPixels, FVector2D& OutPan, float& OutSelfAngle, bool& bMarkerValid, FVector2D& OutMarkerPos) const
{
	EnsureBounds();
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
	EnsureBounds();
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
	EnsureBounds();
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
