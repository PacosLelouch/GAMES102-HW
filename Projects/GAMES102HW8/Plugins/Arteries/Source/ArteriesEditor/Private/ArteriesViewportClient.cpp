// Author: LiJiayu (JerryLi)
// Mail: lijiayu83@gmail.com (fullike@163.com)
// Copyright 2019. All Rights Reserved.

#include "ArteriesViewportClient.h"
#include "ArteriesViewport.h"
#include "CanvasItem.h"
#include "EngineUtils.h"
#include "UnrealEdGlobals.h"
#include "SEditorViewport.h"
#include "EngineGlobals.h"
#include "Editor.h"
#include "SSCSEditor.h"
#include "SKismetInspector.h"
#include "ScopedTransaction.h"
#include "ISCSEditorCustomization.h"
#include "CanvasTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureCube.h"
#include "Editor/EditorPerProjectUserSettings.h"
#include "Editor/UnrealEdEngine.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/ComponentEditorUtils.h"
#include "Materials/Material.h"
#include "ThumbnailRendering/SceneThumbnailInfo.h"
#include "ThumbnailRendering/ThumbnailManager.h"
#include "Settings/LevelEditorViewportSettings.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"

/////////////////////////////////////////////////////////////////////////
// FArteriesViewportClient

FArteriesViewportClient::FArteriesViewportClient(TWeakPtr<FBlueprintEditor>& InBlueprintEditorPtr, FPreviewScene* InPreviewScene, const TSharedRef<SArteriesViewport>& InArteriesViewport)
	: FEditorViewportClient(nullptr, InPreviewScene, StaticCastSharedRef<SEditorViewport>(InArteriesViewport))
	, BlueprintEditorPtr(InBlueprintEditorPtr)
	, PreviewActorBounds(ForceInitToZero)
	, bIsManipulating(false)
	, ScopedTransaction(NULL)
	, bIsSimulateEnabled(false)
{
	SetViewModes(VMI_Lit, VMI_Lit);

	EngineShowFlags.DisableAdvancedFeatures();

	check(Widget);
	Widget->SetSnapEnabled(true);

	// Selectively set particular show flags that we need
	EngineShowFlags.SetSelectionOutline(GetDefault<ULevelEditorViewportSettings>()->bUseSelectionOutline);

	// Set if the grid will be drawn
	DrawHelper.bDrawGrid = GetDefault<UEditorPerProjectUserSettings>()->bSCSEditorShowGrid;

	// now add floor
	EditorFloorComp = NewObject<UStaticMeshComponent>(GetTransientPackage(), TEXT("EditorFloorComp"));

	UStaticMesh* FloorMesh = LoadObject<UStaticMesh>(NULL, TEXT("/Engine/EditorMeshes/PhAT_FloorBox.PhAT_FloorBox"), NULL, LOAD_None, NULL);
	if (ensure(FloorMesh))
	{
		EditorFloorComp->SetStaticMesh(FloorMesh);
	}

	UMaterial* Material = LoadObject<UMaterial>(NULL, TEXT("/Engine/EditorMaterials/PersonaFloorMat.PersonaFloorMat"), NULL, LOAD_None, NULL);
	if (ensure(Material))
	{
		EditorFloorComp->SetMaterial(0, Material);
	}

	EditorFloorComp->SetRelativeScale3D(FVector(3.f, 3.f, 1.f));
	EditorFloorComp->SetVisibility(GetDefault<UEditorPerProjectUserSettings>()->bSCSEditorShowFloor);
	EditorFloorComp->SetCollisionEnabled(GetDefault<UEditorPerProjectUserSettings>()->bSCSEditorShowFloor? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	PreviewScene->AddComponent(EditorFloorComp, FTransform::Identity);

	// Turn off so that actors added to the world do not have a lifespan (so they will not auto-destroy themselves).
	PreviewScene->GetWorld()->bBegunPlay = false;

	PreviewScene->SetSkyCubemap(GUnrealEd->GetThumbnailManager()->AmbientCubemap);
}

FArteriesViewportClient::~FArteriesViewportClient()
{
}

void FArteriesViewportClient::Tick(float DeltaSeconds)
{
	FEditorViewportClient::Tick(DeltaSeconds);

	// Register the selection override delegate for the preview actor's components
	TSharedPtr<SSCSEditor> SCSEditor = BlueprintEditorPtr.Pin()->GetSCSEditor();
	AActor* PreviewActor = GetPreviewActor();
	if (PreviewActor != nullptr)
	{
		for (UActorComponent* Component : PreviewActor->GetComponents())
		{
			if (UPrimitiveComponent* PrimComponent = Cast<UPrimitiveComponent>(Component))
			{
				if (!PrimComponent->SelectionOverrideDelegate.IsBound())
				{
					SCSEditor->SetSelectionOverride(PrimComponent);
				}
			}
		}
	}
	else
	{
		InvalidatePreview(false);
	}

	// Tick the preview scene world.
	if (!GIntraFrameDebuggingGameThread)
	{
		// Ensure that the preview actor instance is up-to-date for component editing (e.g. after compiling the Blueprint, the actor may be reinstanced outside of this class)
		if(PreviewActor != BlueprintEditorPtr.Pin()->GetBlueprintObj()->SimpleConstructionScript->GetComponentEditorActorInstance())
		{
			BlueprintEditorPtr.Pin()->GetBlueprintObj()->SimpleConstructionScript->SetComponentEditorActorInstance(PreviewActor);
		}

		// Allow full tick only if preview simulation is enabled and we're not currently in an active SIE or PIE session
		if(bIsSimulateEnabled && GEditor->PlayWorld == NULL && !GEditor->bIsSimulatingInEditor)
		{
			PreviewScene->GetWorld()->Tick(IsRealtime() ? LEVELTICK_All : LEVELTICK_TimeOnly, DeltaSeconds);
		}
		else
		{
			PreviewScene->GetWorld()->Tick(IsRealtime() ? LEVELTICK_ViewportsOnly : LEVELTICK_TimeOnly, DeltaSeconds);
		}
	}
}

int32 FArteriesViewportClient::GetCameraSpeedSetting() const
{
	return GetDefault<UEditorPerProjectUserSettings>()->SCSViewportCameraSpeed;
}

void FArteriesViewportClient::SetCameraSpeedSetting(int32 SpeedSetting)
{
	GetMutableDefault<UEditorPerProjectUserSettings>()->SCSViewportCameraSpeed = SpeedSetting;
}

void FArteriesViewportClient::InvalidatePreview(bool bResetCamera)
{
	// Ensure that the editor is valid before continuing
	if(!BlueprintEditorPtr.IsValid())
	{
		return;
	}

	UBlueprint* Blueprint = BlueprintEditorPtr.Pin()->GetBlueprintObj();
	check(Blueprint);

	const bool bIsPreviewActorValid = GetPreviewActor() != nullptr;

	// Create or update the Blueprint actor instance in the preview scene
	BlueprintEditorPtr.Pin()->UpdatePreviewActor(Blueprint, !bIsPreviewActorValid);

	Invalidate();
	RefreshPreviewBounds();
	
	if( bResetCamera )
	{
		ResetCamera();
	}
}
const float AutoViewportOrbitCameraTranslate = 256.0f;
void FArteriesViewportClient::ResetCamera()
{
	UBlueprint* Blueprint = BlueprintEditorPtr.Pin()->GetBlueprintObj();

	// For now, loosely base default camera positioning on thumbnail preview settings
	USceneThumbnailInfo* ThumbnailInfo = Cast<USceneThumbnailInfo>(Blueprint->ThumbnailInfo);
	if (ThumbnailInfo)
	{
		if (PreviewActorBounds.SphereRadius + ThumbnailInfo->OrbitZoom < 0)
		{
			ThumbnailInfo->OrbitZoom = -PreviewActorBounds.SphereRadius;
		}
	}
	else
	{
		ThumbnailInfo = USceneThumbnailInfo::StaticClass()->GetDefaultObject<USceneThumbnailInfo>();
	}

	ToggleOrbitCamera(true);
	{
		float TargetDistance = PreviewActorBounds.SphereRadius;
		if (TargetDistance <= 0.0f)
		{
			TargetDistance = AutoViewportOrbitCameraTranslate;
		}

		FRotator ThumbnailAngle(ThumbnailInfo->OrbitPitch, ThumbnailInfo->OrbitYaw, 0.0f);

		SetViewLocationForOrbiting(PreviewActorBounds.Origin);
		SetViewLocation(GetViewLocation() + FVector(0.0f, TargetDistance * 1.5f + ThumbnailInfo->OrbitZoom - AutoViewportOrbitCameraTranslate, 0.0f));
		SetViewRotation(ThumbnailAngle);

	}

	Invalidate();
}

void FArteriesViewportClient::ToggleRealtimePreview()
{
	SetRealtime(!IsRealtime());

	Invalidate();
}

AActor* FArteriesViewportClient::GetPreviewActor() const
{
	return BlueprintEditorPtr.Pin()->GetPreviewActor();
}

void FArteriesViewportClient::FocusViewportToSelection()
{
	AActor* PreviewActor = GetPreviewActor();
	if(PreviewActor)
	{
		TArray<FSCSEditorTreeNodePtrType> SelectedNodes = BlueprintEditorPtr.Pin()->GetSelectedSCSEditorTreeNodes();
		if(SelectedNodes.Num() > 0)
		{
			// Use the last selected item for the widget location
			USceneComponent* SceneComp = Cast<USceneComponent>(SelectedNodes.Last()->FindComponentInstanceInActor(PreviewActor));
			if( SceneComp )
			{
				FocusViewportOnBox( SceneComp->Bounds.GetBox() );
			}
		}
		else
		{
			FocusViewportOnBox( PreviewActor->GetComponentsBoundingBox( true ) );
		}
	}
}

bool FArteriesViewportClient::GetIsSimulateEnabled() 
{ 
	return bIsSimulateEnabled;
}

void FArteriesViewportClient::ToggleIsSimulateEnabled() 
{
	// Must destroy existing actors before we toggle the world state
	BlueprintEditorPtr.Pin()->DestroyPreview();

	bIsSimulateEnabled = !bIsSimulateEnabled;
	PreviewScene->GetWorld()->bBegunPlay = bIsSimulateEnabled;
	PreviewScene->GetWorld()->bShouldSimulatePhysics = bIsSimulateEnabled;

	TSharedPtr<SWidget> SCSEditor = BlueprintEditorPtr.Pin()->GetSCSEditor();
	TSharedRef<SWidget> Inspector = BlueprintEditorPtr.Pin()->GetInspector();

	// When simulate is enabled, we don't want to allow the user to modify the components
	BlueprintEditorPtr.Pin()->UpdatePreviewActor(BlueprintEditorPtr.Pin()->GetBlueprintObj(), true);

	SCSEditor->SetEnabled(!bIsSimulateEnabled);
	Inspector->SetEnabled(!bIsSimulateEnabled);

	if(!IsRealtime())
	{
		ToggleRealtimePreview();
	}
}

bool FArteriesViewportClient::GetShowFloor() 
{
	return GetDefault<UEditorPerProjectUserSettings>()->bSCSEditorShowFloor;
}

void FArteriesViewportClient::ToggleShowFloor() 
{
	auto* Settings = GetMutableDefault<UEditorPerProjectUserSettings>();

	bool bShowFloor = Settings->bSCSEditorShowFloor;
	bShowFloor = !bShowFloor;
	
	EditorFloorComp->SetVisibility(bShowFloor);
	EditorFloorComp->SetCollisionEnabled(bShowFloor? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);

	Settings->bSCSEditorShowFloor = bShowFloor;
	Settings->PostEditChange();

	Invalidate();
}

bool FArteriesViewportClient::GetShowGrid() 
{
	return GetDefault<UEditorPerProjectUserSettings>()->bSCSEditorShowGrid;
}

void FArteriesViewportClient::ToggleShowGrid() 
{
	auto* Settings = GetMutableDefault<UEditorPerProjectUserSettings>();

	bool bShowGrid = Settings->bSCSEditorShowGrid;
	bShowGrid = !bShowGrid;

	DrawHelper.bDrawGrid = bShowGrid;

	Settings->bSCSEditorShowGrid = bShowGrid;
	Settings->PostEditChange();
	
	Invalidate();
}

void FArteriesViewportClient::RefreshPreviewBounds()
{
	AActor* PreviewActor = GetPreviewActor();

	if(PreviewActor)
	{
		// Compute actor bounds as the sum of its visible parts
		PreviewActorBounds = FBoxSphereBounds(ForceInitToZero);
		for (UActorComponent* Component : PreviewActor->GetComponents())
		{
			// Aggregate primitive components that either have collision enabled or are otherwise visible components in-game
			if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(Component))
			{
				if (PrimComp->IsRegistered() && (!PrimComp->bHiddenInGame || PrimComp->IsCollisionEnabled()) && PrimComp->Bounds.SphereRadius < HALF_WORLD_MAX)
				{
					PreviewActorBounds = PreviewActorBounds + PrimComp->Bounds;
				}
			}
		}
	}
}
