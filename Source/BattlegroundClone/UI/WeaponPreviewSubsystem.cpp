#include "UI/WeaponPreviewSubsystem.h"

#include "Weapon/WeaponBase.h"
#include "Components/Image.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"

bool UWeaponPreviewSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld() && !IsRunningDedicatedServer();
}

void UWeaponPreviewSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RecaptureTimer);
	}
	if (IsValid(StudioActor))
	{
		StudioActor->Destroy();
	}
	StudioActor = nullptr;
	Cache.Empty();

	Super::Deinitialize();
}

bool UWeaponPreviewSubsystem::ApplyWeaponPreview(const UObject* WorldContextObject, UImage* TargetImage, TSubclassOf<AWeaponBase> WeaponClass)
{
	if (!TargetImage || !WeaponClass || !WorldContextObject)
	{
		return false;
	}
	const UWorld* World = WorldContextObject->GetWorld();
	UWeaponPreviewSubsystem* Subsystem = World ? World->GetSubsystem<UWeaponPreviewSubsystem>() : nullptr;
	UMaterialInstanceDynamic* MID = Subsystem ? Subsystem->GetOrCreatePreview(WeaponClass) : nullptr;
	if (!MID)
	{
		return false;
	}

	FSlateBrush Brush = TargetImage->GetBrush();
	Brush.SetResourceObject(MID);
	Brush.SetImageSize(FVector2D(RenderWidth, RenderHeight));
	TargetImage->SetBrush(Brush);
	return true;
}

void UWeaponPreviewSubsystem::EnsureStudio()
{
	if (IsValid(StudioActor))
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.ObjectFlags |= RF_Transient;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	StudioActor = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, Params);

	USceneComponent* Root = NewObject<USceneComponent>(StudioActor, TEXT("StudioRoot"));
	StudioActor->SetRootComponent(Root);
	Root->RegisterComponent();
	Root->SetWorldLocation(FVector(0.f, 0.f, StudioHeight));

	StudioCapture = NewObject<USceneCaptureComponent2D>(StudioActor, TEXT("StudioCapture"));
	StudioCapture->SetupAttachment(Root);
	StudioCapture->SetRelativeLocation(FVector(-1000.f, 0.f, 0.f));
	StudioCapture->ProjectionType = ECameraProjectionMode::Orthographic;
	StudioCapture->CaptureSource = ESceneCaptureSource::SCS_SceneColorHDR;
	StudioCapture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
	StudioCapture->bCaptureEveryFrame = false;
	StudioCapture->bCaptureOnMovement = false;
	StudioCapture->ShowFlags.SetAtmosphere(false);
	StudioCapture->ShowFlags.SetFog(false);
	StudioCapture->RegisterComponent();

	PreviewMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/UI/M_CharacterPreview.M_CharacterPreview"));
}

UMaterialInstanceDynamic* UWeaponPreviewSubsystem::GetOrCreatePreview(TSubclassOf<AWeaponBase> WeaponClass)
{
	if (FWeaponPreviewEntry* Found = Cache.Find(WeaponClass))
	{
		return Found->MID;
	}

	const AWeaponBase* CDO = WeaponClass->GetDefaultObject<AWeaponBase>();
	const UStaticMeshComponent* SrcMesh = CDO ? CDO->GetWeaponMesh() : nullptr;
	UStaticMesh* StaticMesh = SrcMesh ? SrcMesh->GetStaticMesh() : nullptr;
	if (!StaticMesh)
	{
		return nullptr;
	}

	EnsureStudio();
	if (!PreviewMaterial)
	{
		return nullptr;
	}

	FWeaponPreviewEntry Entry;
	
	Entry.Mesh = NewObject<UStaticMeshComponent>(StudioActor);
	Entry.Mesh->SetupAttachment(StudioActor->GetRootComponent());
	Entry.Mesh->SetStaticMesh(StaticMesh);
	for (int32 i = 0; i < StaticMesh->GetStaticMaterials().Num(); ++i)
	{
		Entry.Mesh->SetMaterial(i, SrcMesh->GetMaterial(i));
	}
	Entry.Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Entry.Mesh->SetCastShadow(false);
	Entry.Mesh->bVisibleInSceneCaptureOnly = true;
	Entry.Mesh->bForceMipStreaming = true;
	
	const FVector Scale = SrcMesh->GetRelativeScale3D();
	const FBox Box = StaticMesh->GetBoundingBox();
	const FVector Ext = Box.GetExtent() * Scale.GetAbs();
	const FRotator Rot = (Ext.X >= Ext.Y) ? FRotator(0.f, 90.f, 0.f) : FRotator::ZeroRotator;
	Entry.Mesh->SetRelativeScale3D(Scale);
	Entry.Mesh->SetRelativeRotation(Rot);
	Entry.Mesh->SetRelativeLocation(-Rot.RotateVector(Box.GetCenter() * Scale));
	Entry.Mesh->RegisterComponent();

	const float Width = 2.f * FMath::Max(Ext.X, Ext.Y);
	const float Height = 2.f * Ext.Z;
	const float Aspect = static_cast<float>(RenderWidth) / RenderHeight;
	Entry.OrthoWidth = FMath::Max(Width, Height * Aspect) * 1.02f; // 여백 10%

	Entry.RenderTarget = NewObject<UTextureRenderTarget2D>(this);
	Entry.RenderTarget->ClearColor = FLinearColor(0.f, 0.f, 0.f, 1.f); // 알파 1 = 빈 배경 (M_CharacterPreview와 동일 규칙)
	Entry.RenderTarget->InitCustomFormat(RenderWidth, RenderHeight, PF_FloatRGBA, true);
	Entry.RenderTarget->UpdateResourceImmediate(true);

	Entry.MID = UMaterialInstanceDynamic::Create(PreviewMaterial, this);
	Entry.MID->SetTextureParameterValue(TEXT("PreviewTexture"), Entry.RenderTarget);
	Entry.MID->SetScalarParameterValue(TEXT("Brightness"), 1.f);

	Capture(Entry);
	UMaterialInstanceDynamic* Result = Entry.MID;
	Cache.Add(WeaponClass, Entry);
	
	RecapturesLeft = 3;
	FTimerManager& TM = GetWorld()->GetTimerManager();
	if (!TM.IsTimerActive(RecaptureTimer))
	{
		TM.SetTimer(RecaptureTimer, this, &UWeaponPreviewSubsystem::RecaptureAll, 0.75f, true);
	}
	return Result;
}

void UWeaponPreviewSubsystem::Capture(FWeaponPreviewEntry& Entry)
{
	if (!StudioCapture || !Entry.Mesh || !Entry.RenderTarget)
	{
		return;
	}
	StudioCapture->OrthoWidth = Entry.OrthoWidth;
	StudioCapture->ClearShowOnlyComponents();
	StudioCapture->ShowOnlyComponent(Entry.Mesh);
	StudioCapture->TextureTarget = Entry.RenderTarget;
	StudioCapture->CaptureScene();
}

void UWeaponPreviewSubsystem::RecaptureAll()
{
	for (TPair<TSubclassOf<AWeaponBase>, FWeaponPreviewEntry>& Pair : Cache)
	{
		Capture(Pair.Value);
	}
	if (--RecapturesLeft <= 0)
	{
		GetWorld()->GetTimerManager().ClearTimer(RecaptureTimer);
	}
}
