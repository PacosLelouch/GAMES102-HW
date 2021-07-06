// Author: LiJiayu (JerryLi)
// Mail: lijiayu83@gmail.com (fullike@163.com)
// Copyright 2019. All Rights Reserved.

#include "ArteriesEditorMode.h"
#include "ArteriesEditorCommands.h"
#include "ArteriesActor.h"
#include "ArteriesUtil.h"
#include "ArteriesEditor.h"
#include "ArteriesToolbox.h"
#include "ArteriesObjectFactory.h"
#include "AssetToolsModule.h"
#include "Engine/Selection.h"
#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "EditorModeManager.h"
#include "Misc/FileHelper.h"
#include "SceneView.h"
#include "EditorViewportClient.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/Application/SlateApplication.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Toolkits/ToolkitManager.h"

#if ENGINE_MINOR_VERSION < 22
	#include "DrawingPolicy.h"
#else
	#include "MeshPassProcessor.h"
#endif
#if ENGINE_MINOR_VERSION >= 23
	#include "RenderTargetPool.h"
#endif

FArteriesOctreeElement::FArteriesOctreeElement(FArteriesPoint* InPoint):Element(InPoint)
{
	LocalBounds = FBox(InPoint->Position, InPoint->Position);
	GraphBounds = FBoxSphereBounds(LocalBounds);
}
FArteriesOctreeElement::FArteriesOctreeElement(FArteriesPrimitive* InPrimitive):Element(InPrimitive)
{
	LocalBounds = InPrimitive->GetBox();
	GraphBounds = FBoxSphereBounds(LocalBounds);
}

#define LOCTEXT_NAMESPACE "ArteriesEditor"

FEditorModeID FArteriesEditorMode::ID = FName(TEXT("ArteriesEditor"));
IMPLEMENT_HIT_PROXY(HArteriesHitProxy, HHitProxy);
FArteriesEditorMode::FArteriesEditorMode():Mode(0), SubMode(0), IsLeftDragging(false), IsInvalidHitProxies(false), Object(NULL), Toolbox(NULL), BlueprintEditor(NULL), SelectionBounds(EForceInit::ForceInit), Octree(FVector::ZeroVector, HALF_WORLD_MAX)
{
#if ENGINE_MINOR_VERSION < 23
	RendererModule = &FModuleManager::GetModuleChecked<IRendererModule>(FName("Renderer"));
#endif
	Properties = NewObject<UArteriesEditorProperties>(GetTransientPackage());
	Properties->Mode = this;
	Properties->AddToRoot();
}
FArteriesEditorMode::~FArteriesEditorMode()
{
//	BlueprintEditorPtr->GetBlueprintObj()->OnCompiled().RemoveAll(this);
	SetArteriesActor(NULL);
	Properties->RemoveFromRoot();
}
void FArteriesEditorMode::SetBlueprintEditor(FBlueprintEditor* InBlueprintEditor)
{
	if (!BlueprintEditor)
	{
		BlueprintEditor = InBlueprintEditor;
	//	BlueprintEditorPtr->GetBlueprintObj()->OnCompiled().AddRaw(this, &FArteriesEditorMode::OnBlueprintCompiled);
		UICommandList = MakeShareable(new FUICommandList);
		const FArteriesEditorCommands& Commands = FArteriesEditorCommands::Get();
		UICommandList->MapAction(
			Commands.Select,
			FExecuteAction::CreateRaw(this, &FArteriesEditorMode::OnSetMode, 0),
			FCanExecuteAction(),
			FIsActionChecked::CreateRaw(this, &FArteriesEditorMode::IsInMode, 0));
		UICommandList->MapAction(
			Commands.Create,
			FExecuteAction::CreateRaw(this, &FArteriesEditorMode::OnSetMode, 1),
			FCanExecuteAction(),
			FIsActionChecked::CreateRaw(this, &FArteriesEditorMode::IsInMode, 1));
		UICommandList->MapAction(
			Commands.Settings,
			FExecuteAction::CreateRaw(this, &FArteriesEditorMode::OnSetMode, 2),
			FCanExecuteAction(),
			FIsActionChecked::CreateRaw(this, &FArteriesEditorMode::IsInMode, 2));
		UICommandList->MapAction(
			Commands.Stats,
			FExecuteAction::CreateRaw(this, &FArteriesEditorMode::OnSetMode, 3),
			FCanExecuteAction(),
			FIsActionChecked::CreateRaw(this, &FArteriesEditorMode::IsInMode, 3));
		UICommandList->MapAction(
			Commands.InternalObjects,
			FExecuteAction::CreateRaw(this, &FArteriesEditorMode::OnSetMode, 4),
			FCanExecuteAction(),
			FIsActionChecked::CreateRaw(this, &FArteriesEditorMode::IsInMode, 4));

		UICommandList->MapAction(
			Commands.Delete,
			FExecuteAction::CreateRaw(this, &FArteriesEditorMode::OnDeleteClicked),
			FCanExecuteAction::CreateSP(this, &FArteriesEditorMode::IsInSubMode, 1));
		UICommandList->MapAction(
			Commands.Reverse,
			FExecuteAction::CreateRaw(this, &FArteriesEditorMode::OnReverseClicked),
			FCanExecuteAction::CreateSP(this, &FArteriesEditorMode::IsInSubMode, 1));
		UICommandList->MapAction(
			Commands.FlipX,
			FExecuteAction::CreateRaw(this, &FArteriesEditorMode::OnFlipXClicked),
			FCanExecuteAction::CreateSP(this, &FArteriesEditorMode::IsInSubMode, 1));
		UICommandList->MapAction(
			Commands.FlipY,
			FExecuteAction::CreateRaw(this, &FArteriesEditorMode::OnFlipYClicked),
			FCanExecuteAction::CreateSP(this, &FArteriesEditorMode::IsInSubMode, 1));
		UICommandList->MapAction(
			Commands.FlipZ,
			FExecuteAction::CreateRaw(this, &FArteriesEditorMode::OnFlipZClicked),
			FCanExecuteAction::CreateSP(this, &FArteriesEditorMode::IsInSubMode, 1));

		TSharedRef<FUICommandList> ToolkitCommandList = BlueprintEditor->GetToolkitCommands();
		ToolkitCommandList->MapAction(Commands.DisplayThisNode,FExecuteAction::CreateRaw(this, &FArteriesEditorMode::OnDisplayThisNode));
		ToolkitCommandList->MapAction(Commands.DisplayFinalResult, FExecuteAction::CreateRaw(this, &FArteriesEditorMode::OnDisplayFinalResult));
		ToolkitCommandList->MapAction(Commands.Build, FExecuteAction::CreateRaw(this, &FArteriesEditorMode::OnBuild));
	}
}
void FArteriesEditorMode::OnSetSubMode(int InSubMode)
{
	if (SubMode != InSubMode)
	{
		DestroyOctree();
		ClearSelection();
		SubMode = InSubMode;
		if (Toolbox)
			Toolbox->RefreshSelection();
		RebuildHitProxies();
		CreateOctree();
		RebuildPrimitiveElements();
	}
}
void FArteriesEditorMode::OnBlueprintCompiled(UBlueprint* Blueprint)
{
}
void FArteriesEditorMode::OnDisplayThisNode()
{
	if (Actor)
	{
		if (UEdGraphNode* Node = BlueprintEditor->GetSingleSelectedNode())
			Actor->SetDisplayNode(Node);
	}
}
void FArteriesEditorMode::OnDisplayFinalResult()
{
	if (Actor)
		Actor->SetDisplayNode(NULL);
}
void FArteriesEditorMode::OnBuild()
{
	if (Actor)
		Actor->Build(true);
}
void FArteriesEditorMode::CreateOctree()
{
	if (SubMode == 0)
	{
		for (FArteriesPoint& Point : Object->Points)
			Octree.AddElement(FArteriesOctreeElement(&Point));
	}
	else
	{
		for (FArteriesPrimitive& Primitive : Object->Primitives)
			Octree.AddElement(FArteriesOctreeElement(&Primitive));
	}
}
void FArteriesEditorMode::ClearSelection()
{
	SelectionBounds = FBox(EForceInit::ForceInit);
	SelectedElements.Empty();
	OnSelectionChanged();
}
void FArteriesEditorMode::AddToSelection(FArteriesElement* Element)
{
	if (SelectedElements.Num() && SelectedElements[FSetElementId::FromInteger(0)]->GetStruct() != Element->GetStruct())
		ClearSelection();
	SelectedElements.Add(Element);
	if (FArteriesPoint* Point = Element->ToPoint())
		SelectionBounds += Point->Position;
	else if (FArteriesPrimitive* Primitive = Element->ToPrimitive())
		SelectionBounds += Primitive->GetBox();
}
void FArteriesEditorMode::OnSelectionChanged()
{
	RebuildSelectedPrimitiveElements();
	Properties->OnSelectionChanged();
	if (Toolbox)
		Toolbox->RefreshProperties();
}
void FArteriesEditorMode::RebuildPrimitiveElements()
{
	PrimitiveElements.Clear();
	if (Actor && Mode == 0 && SubMode == 1)
	{
		UProceduralMeshComponent* Component = Cast<UProceduralMeshComponent>(Actor->GetComponentByClass(UProceduralMeshComponent::StaticClass()));
		for (FArteriesPrimitive& Primitive : Object->Primitives)
			BuildPrimitive(PrimitiveElements, Component, &Primitive, FLinearColor(0, 0, 0, 0));
	}
}
void FArteriesEditorMode::RebuildSelectedPrimitiveElements()
{
	SelectedPrimitiveElements.Clear();
	if (Actor && Mode == 0 && SubMode == 1)
	{
		UProceduralMeshComponent* Component = Cast<UProceduralMeshComponent>(Actor->GetComponentByClass(UProceduralMeshComponent::StaticClass()));
		for (FArteriesElement* Element : SelectedElements)
		{
			if (FArteriesPrimitive* Primitive = Element->ToPrimitive())
				BuildPrimitive(SelectedPrimitiveElements, Component, Primitive, FLinearColor(0, 0, 1.f, 0.5f));
		}
	}
}
void FArteriesEditorMode::BuildPrimitive(FBatchedElements& Elements, UProceduralMeshComponent* Component, FArteriesPrimitive* Primitive, const FLinearColor& Color)
{
	int SectionIndex = Component->GetMaterials().Find(Primitive->Material);
	FProcMeshSection* Section = Component->GetProcMeshSection(SectionIndex);
	HHitProxy* Proxy = GetHitProxy(Primitive);
	TMap<int, int> IndicesMap;
	for (uint32 i = 0; i < Primitive->NumTriangles; i++)
	{
		int BaseIndex = Primitive->StartIndex + i * 3;
		int Indices[3];
		for (uint32 j = 0; j < 3; j++)
		{
			int Index = Section->ProcIndexBuffer[BaseIndex + j];
			if (!IndicesMap.Contains(Index))
			{
				FProcMeshVertex& Vertex = Section->ProcVertexBuffer[Index];
				int NewIndex = Elements.AddVertex(FVector4(Vertex.Position, 1), FVector2D::ZeroVector, Color, Proxy->Id);
				IndicesMap.Add(Index, NewIndex);
			}
			Indices[j] = IndicesMap[Index];
		}
		Elements.AddTriangle(Indices[0], Indices[1], Indices[2], GWhiteTexture, EBlendMode::BLEND_Translucent);
	}
}
void FArteriesEditorMode::ApplyOffset(const FVector& Offset)
{
	if (GetSelectionType() == FArteriesPoint::StaticStruct())
	{
		for (FArteriesElement* Element : SelectedElements)
		{
			if (FArteriesPoint* Point = Element->ToPoint())
				Point->Position += Offset;
		}
	}
	else
	{
		TSet<FArteriesPoint*> Points;
		for (FArteriesElement* Element : SelectedElements)
		{
			if (FArteriesPrimitive* Primitive = Element->ToPrimitive())
				Points.Append(Primitive->Points);
		}
		for (FArteriesPoint* Point : Points)
			Point->Position += Offset;
	}
	SelectionBounds.Min += Offset;
	SelectionBounds.Max += Offset;
}
FVector FArteriesEditorMode::GetPlaneNormal(ELevelViewportType Type, FVector* TangentX, FVector* TangentY)
{
	FVector TangentZ;
	switch (Type)
	{
	case LVT_OrthoXY:
		UArteriesObject::GetTangents(4, TangentX, TangentY, &TangentZ); break;
	case LVT_OrthoNegativeXY:
		UArteriesObject::GetTangents(5, TangentX, TangentY, &TangentZ); break;
	case LVT_OrthoXZ:
		UArteriesObject::GetTangents(2, TangentX, TangentY, &TangentZ); break;
	case LVT_OrthoNegativeXZ:
		UArteriesObject::GetTangents(3, TangentX, TangentY, &TangentZ); break;
	case LVT_OrthoYZ:
		UArteriesObject::GetTangents(0, TangentX, TangentY, &TangentZ); break;
	case LVT_OrthoNegativeYZ:
		UArteriesObject::GetTangents(1, TangentX, TangentY, &TangentZ); break;
	default:
		TangentZ = FVector::UpVector;
		if (TangentX)*TangentX = FVector::ForwardVector;
		if (TangentY)*TangentY = FVector::RightVector;
	}
	return TangentZ;
}
FVector FArteriesEditorMode::UpdateMovingPoint(FEditorViewportClient* InViewportClient, FViewport* InViewport, int32 InMouseX, int32 InMouseY)
{
	FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(
		InViewport,
		InViewportClient->GetScene(),
		InViewportClient->EngineShowFlags)
		.SetRealtimeUpdate(InViewportClient->IsRealtime()));
	FSceneView* View = InViewportClient->CalcSceneView(&ViewFamily);
	FViewportCursorLocation Ray(View, InViewportClient, InMouseX, InMouseY);
	FVector TangentZ = GetPlaneNormal(InViewportClient->GetViewportType());
	FPlane CreationPlane(TangentZ, 0);
#if ENGINE_MINOR_VERSION < 20
	MovingPoint = FMath::LinePlaneIntersection(Ray.GetOrigin(), Ray.GetOrigin() + Ray.GetDirection()* 99999999.f, CreationPlane);
#else
	MovingPoint = FMath::RayPlaneIntersection(Ray.GetOrigin(), Ray.GetDirection(), CreationPlane);
#endif
	if (GetDefault<ULevelEditorViewportSettings>()->GridEnabled)
		MovingPoint = MovingPoint.GridSnap(GEditor->GetGridSize());
	return TangentZ;
}
UStruct* FArteriesEditorMode::GetSelectionType()
{
	if (SelectedElements.Num())
		return SelectedElements[FSetElementId::FromInteger(0)]->GetStruct();
	return NULL;
}
FArteriesPoint* FArteriesEditorMode::FindPointSlow(const FVector& Position)
{
	float Size = GEditor->GetGridSize() / 2;
	for (FArteriesPoint& Point : Object->Points)
	{
		if (Point.Position.Equals(Position, Size))
			return &Point;
	}
	return NULL;
}
FVector FArteriesEditorMode::GetWidgetLocation() const
{
	return SelectionBounds.GetCenter();
}
bool FArteriesEditorMode::ShouldDrawWidget() const
{
	return SelectedElements.Num() > 0;
}
bool FArteriesEditorMode::GetCursor(EMouseCursor::Type& OutCursor) const
{
	OutCursor = HoveredProxy ? HoveredProxy->GetMouseCursor() : EMouseCursor::Default;
	return true;
}
bool FArteriesEditorMode::CapturedMouseMove(FEditorViewportClient* InViewportClient, FViewport* InViewport, int32 InMouseX, int32 InMouseY)
{
	if (IsLeftDragging && Mode == 0 && GetModeManager()->GetWidgetMode() == FWidget::WM_None && InViewportClient->GetViewportType() != LVT_Perspective)
	{
		FVector PlaneNormal = UpdateMovingPoint(InViewportClient, InViewport, InMouseX, InMouseY);
		if (!InViewport->KeyState(EKeys::LeftControl) && !InViewport->KeyState(EKeys::RightControl))
			ClearSelection();
		FBox Region(EForceInit::ForceInit);
		Region += MouseDownPoint - PlaneNormal * 99999999.f;
		Region += MovingPoint + PlaneNormal * 99999999.f;
		for (FArteriesOctree::TConstElementBoxIterator<> It(Octree, FBoxCenterAndExtent(Region)); It.HasPendingElements(); It.Advance())
			AddToSelection(It.GetCurrentElement().Element);
		OnSelectionChanged();
		return true;
	}
	return false;
}
bool FArteriesEditorMode::MouseMove(FEditorViewportClient* InViewportClient, FViewport* InViewport, int32 InMouseX, int32 InMouseY)
{
	UpdateMovingPoint(InViewportClient, InViewport, InMouseX, InMouseY);
	HHitProxy* HitProxy = InViewport->GetHitProxy(InMouseX, InMouseY);
	if (HitProxy && (HitProxy->IsA(HArteriesHitProxy::StaticGetType()) || HitProxy->IsA(HWidgetAxis::StaticGetType())))
		HoveredProxy = HitProxy;
	else
		HoveredProxy = NULL;
	return true;
}
bool FArteriesEditorMode::HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click)
{
	if (Object)
	{
		switch (Mode)
		{
		case 0:
			if (Click.GetKey() == EKeys::RightMouseButton)
			{
				const FArteriesEditorCommands& Commands = FArteriesEditorCommands::Get();
				const bool bShouldCloseWindowAfterMenuSelection = true;
				FMenuBuilder MenuBuilder(bShouldCloseWindowAfterMenuSelection, UICommandList);
				MenuBuilder.BeginSection("General", LOCTEXT("General", "General"));
				MenuBuilder.AddMenuEntry(Commands.Delete);
				MenuBuilder.EndSection();
				if (SubMode == 0)
				{
					MenuBuilder.BeginSection("Point", LOCTEXT("Point", "Point"));
					MenuBuilder.EndSection();
				}
				else if (SubMode == 1)
				{
					MenuBuilder.BeginSection("Primitive", LOCTEXT("Primitive", "Primitive"));
					MenuBuilder.AddMenuEntry(Commands.Reverse);
					MenuBuilder.AddMenuEntry(Commands.FlipX);
					MenuBuilder.AddMenuEntry(Commands.FlipY);
					MenuBuilder.AddMenuEntry(Commands.FlipZ);
					MenuBuilder.EndSection();
				}
				TSharedPtr<SWidget> MenuWidget = MenuBuilder.MakeWidget();
				if (MenuWidget.IsValid())
				{
					// @todo: Should actually use the location from a click event instead!
					const FVector2D MouseCursorLocation = FSlateApplication::Get().GetCursorPos();
					FSlateApplication::Get().PushMenu(
						InViewportClient->GetEditorViewportWidget().ToSharedRef(),
						FWidgetPath(),
						MenuWidget.ToSharedRef(),
						MouseCursorLocation,
						FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu));
					return true;
				}
			}
			else
			{
				if (!InViewportClient->Viewport->KeyState(EKeys::LeftControl) && !InViewportClient->Viewport->KeyState(EKeys::RightControl))
					ClearSelection();
				if (HitProxy && HitProxy->IsA(HArteriesHitProxy::StaticGetType()))
				{
					HArteriesHitProxy* Proxy = (HArteriesHitProxy*)HitProxy;
					AddToSelection(Proxy->Element);
					OnSelectionChanged();
				}
			}
			break;
		case 1:
			Object->Modify();
			FArteriesPoint* NewPoint = NULL;
			if (HitProxy && HitProxy->IsA(HArteriesHitProxy::StaticGetType()))
			{
				HArteriesHitProxy* Proxy = (HArteriesHitProxy*)HitProxy;
				NewPoint = Proxy->Element->ToPoint();
			}
			if (!NewPoint) NewPoint = FindPointSlow(MovingPoint);
			if (!NewPoint) NewPoint = Object->AddPoint(MovingPoint);
			FArteriesPrimitive* Primitive = SelectedElements.Num() ? SelectedElements[FSetElementId::FromInteger(0)]->ToPrimitive() : NULL;
			if (!Primitive)
			{
				if (SelectedElements.Num())
					ClearSelection();
				Primitive = Object->AddPrimitive();
				AddToSelection(Primitive);
				OnSelectionChanged();
			}
			Primitive->Add(NewPoint);
			break;
		}
	}
	return true;
}
bool FArteriesEditorMode::InputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event)
{
	if (UICommandList->ProcessCommandBindings(Key, FSlateApplication::Get().GetModifierKeys(), (Event == IE_Repeat)))
		return true;
	if (Event == EInputEvent::IE_Pressed)
	{
		if (Key == EKeys::LeftMouseButton)
		{
			IsLeftDragging = true;
			MouseDownPoint = MovingPoint;
		}
		if (Key == EKeys::Escape)
		{
			if (GetModeManager()->GetWidgetMode() != FWidget::WM_None)
			{
				GetModeManager()->SetWidgetMode(FWidget::WM_None);
				return true;
			}
			if (SelectedElements.Num())
			{
				ClearSelection();
				return true;
			}
		}
		switch (Mode)
		{
		case 0:
			/*
			if (Key == EKeys::Delete)
			{
				OnDeleteClicked();
				return true;
			}*/
			break;
		case 1:
			if (Key == EKeys::Escape)
			{
				Mode = 0;
				return true;
			}
			break;
		}
	}
	if (Event == EInputEvent::IE_Released)
	{
		if (Key == EKeys::LeftMouseButton)
			IsLeftDragging = false;
	}
	return false;
}
bool FArteriesEditorMode::InputDelta(FEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale)
{
	if (SelectedElements.Num())
	{
		const EAxisList::Type CurrentAxis = InViewportClient->GetCurrentWidgetAxis();
		if (CurrentAxis != EAxisList::None)
		{
			const FWidget::EWidgetMode WidgetMode = GetModeManager()->GetWidgetMode();
			if (WidgetMode == FWidget::WM_Translate)
			{
			//	bool bSnap = GetDefault<ULevelEditorViewportSettings>()->GridEnabled;
				Object->Modify();
				ApplyOffset(InDrag);
				/*
				for (FArteriesElement* Element : SelectedElements)
				{
					if (FArteriesPoint* Point = Element->ToPoint())
					{
						Point->Position += InDrag;
						if (bSnap)
							Point->Position = Point->Position.GridSnap(GEditor->GetGridSize());
					}
					if (FArteriesPrimitive* Primitive = Element->ToPrimitive())
					{
					}
				}*/
				return true;
			}
		}
	}
	return FEdMode::InputDelta(InViewportClient, InViewport, InDrag, InRot, InScale);
}
void FArteriesEditorMode::Tick(FEditorViewportClient* ViewportClient, float DeltaTime)
{
	FEdMode::Tick(ViewportClient, DeltaTime);
	SetArteriesActor(Cast<AArteriesActor>(BlueprintEditor->GetPreviewActor()));
	if (IsInvalidHitProxies)
	{
		ViewportClient->Viewport->InvalidateHitProxy();
		IsInvalidHitProxies = false;
	}
}
/** FEdMode: Render elements for the Foliage tool */
void FArteriesEditorMode::Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI)
{
	/** Call parent implementation */
	FEdMode::Render(View, Viewport, PDI);
	if (Object)
	{
		if (Toolbox->bDisplayPoints)
		{
			for (FArteriesPoint& Point : Object->Points)
			{
				if (!SelectedElements.Contains(&Point))
					DrawPoint(PDI, &Point, FLinearColor::Gray);
			}
		}
		if (Toolbox->bDisplayPrimitives)
		{
			for (FArteriesPrimitive& Primitive : Object->Primitives)
			{
				if (!SelectedElements.Contains(&Primitive))
					DrawPrimitive(PDI, &Primitive, FLinearColor::Gray);
			}
		}
		if (Toolbox->bDisplayPoints && SubMode == 0)
		{
			for (FArteriesElement* Element : SelectedElements)
				DrawPoint(PDI, Element->ToPoint(), FLinearColor::Blue);
		}
		if (Toolbox->bDisplayPrimitives && SubMode == 1)
		{
			for (FArteriesElement* Element : SelectedElements)
				DrawPrimitive(PDI, Element->ToPrimitive(), FLinearColor::Blue);
		}
		if (Mode == 0)
		{
			if (IsLeftDragging && GetModeManager()->GetWidgetMode() == FWidget::WM_None)
			{
				ELevelViewportType Type = ((FEditorViewportClient*)Viewport->GetClient())->GetViewportType();
				if (Type != LVT_Perspective)
				{
					FVector TangentX, TangentY;
					FVector TangentZ = GetPlaneNormal(Type, &TangentX, &TangentY);
					FMatrix LocalToWorld(TangentX, TangentY, TangentZ, MouseDownPoint);
					FMatrix WorldToLocal = LocalToWorld.Inverse();
					FVector MovingLocal = WorldToLocal.TransformPosition(MovingPoint);
					FVector P0 = LocalToWorld.TransformPosition(FVector(MovingLocal.X, 0, 0));
					FVector P1 = LocalToWorld.TransformPosition(FVector(0, MovingLocal.Y, 0));
					PDI->DrawLine(MouseDownPoint, P0, FLinearColor::Blue, SDPG_Foreground);
					PDI->DrawLine(MouseDownPoint, P1, FLinearColor::Blue, SDPG_Foreground);
					PDI->DrawLine(MovingPoint, P0, FLinearColor::Blue, SDPG_Foreground);
					PDI->DrawLine(MovingPoint, P1, FLinearColor::Blue, SDPG_Foreground);
				}
			}
		}
		else if (Mode == 1)
		{
			PDI->SetHitProxy(NULL);
			if (SelectedElements.Num())
			{
				if (FArteriesPrimitive* Primitive = SelectedElements[FSetElementId::FromInteger(SelectedElements.Num() - 1)]->ToPrimitive())
					PDI->DrawLine(Primitive->Points.Last()->Position, MovingPoint, FLinearColor::Blue, SDPG_Foreground);
			}
			PDI->DrawPoint(MovingPoint, FLinearColor::Blue, 8, SDPG_Foreground);
		}
	}
}
void FArteriesEditorMode::DrawHUD(FEditorViewportClient* ViewportClient, FViewport* Viewport, const FSceneView* View, FCanvas* Canvas)
{
	auto DrawNumber = [&](const FVector& Position, const FLinearColor& Color, int Number)
	{
		FVector2D Pos2D;
		if (View->WorldToPixel(Position, Pos2D))
		{
			Pos2D /= ViewportClient->GetDPIScale();
			FCanvasTextItem TextItem(FIntPoint(FMath::RoundToInt(Pos2D.X), FMath::RoundToInt(Pos2D.Y)), FText::FromString(FString::Printf(TEXT("%d"), Number)), GEngine->GetLargeFont(), Color);
			TextItem.EnableShadow(FLinearColor::White);
			TextItem.bCentreX = true;
			TextItem.bCentreY = true;
			TextItem.Draw(Canvas);
		}
	};
	if (Object)
	{
		if (Mode == 0 && SubMode == 1)
		{
			ViewProjectionMatrix = View->ViewMatrices.GetViewProjectionMatrix();
			if (Canvas->IsHitTesting())
				DrawElements(Canvas, PrimitiveElements);
			else
				DrawElements(Canvas, SelectedPrimitiveElements);
		}
		if (Toolbox->bDisplayPointIds)
		{
			for (int i = 0; i < Object->Points.Num(); i++)
				DrawNumber(Object->Points[i].Position, FColor::Blue, i);
		}
		if (Toolbox->bDisplayPrimitiveIds)
		{
			for (int i = 0; i < Object->Primitives.Num(); i++)
				DrawNumber(Object->Primitives[i].Centroid(), FColor::Red, i);
		}
	}
	FEdMode::DrawHUD(ViewportClient, Viewport, View, Canvas);
}
void FArteriesEditorMode::DrawElements(FCanvas* Canvas, FBatchedElements& Elements)
{
	if (Elements.HasPrimsToDraw())
	{
		// current render target set for the canvas
		const FRenderTarget* CanvasRenderTarget = Canvas->GetRenderTarget();
		bool bNeedsToSwitchVerticalAxis = RHINeedsToSwitchVerticalAxis(Canvas->GetShaderPlatform()) && !Canvas->GetAllowSwitchVerticalAxis();
		// Render the batched elements.
		struct FBatchedDrawParameters
		{
			uint32 bHitTesting : 1;
			uint32 bNeedsToSwitchVerticalAxis : 1;
			uint32 ViewportSizeX;
			uint32 ViewportSizeY;
			float DisplayGamma;
			uint32 AllowedCanvasModes;
			ERHIFeatureLevel::Type FeatureLevel;
			EShaderPlatform ShaderPlatform;
		};
		// all the parameters needed for rendering
		FBatchedDrawParameters DrawParameters =
		{
			(uint32)(Canvas->IsHitTesting() ? 1 : 0),
			(uint32)(bNeedsToSwitchVerticalAxis ? 1 : 0),
			(uint32)CanvasRenderTarget->GetSizeXY().X,
			(uint32)CanvasRenderTarget->GetSizeXY().Y,
			1.0f,
			Canvas->GetAllowedModes(),
			Canvas->GetFeatureLevel(),
			Canvas->GetShaderPlatform()
		};
		ENQUEUE_RENDER_COMMAND(BatchedDrawCommand)([this, &Elements, CanvasRenderTarget, DrawParameters](FRHICommandList& RHICmdList)
		{
			FSceneView SceneView = FBatchedElements::CreateProxySceneView(ViewProjectionMatrix, FIntRect(0, 0, DrawParameters.ViewportSizeX, DrawParameters.ViewportSizeY));
#if ENGINE_MINOR_VERSION < 22
			FDrawingPolicyRenderState DrawRenderState(SceneView);
#else
			FMeshPassProcessorRenderState DrawRenderState(SceneView);
#endif
			// disable depth test & writes
			if (DrawParameters.bHitTesting)
			{
				TRefCountPtr<IPooledRenderTarget> DS;
				FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(FIntPoint(DrawParameters.ViewportSizeX, DrawParameters.ViewportSizeY), PF_DepthStencil, FClearValueBinding((float)ERHIZBuffer::FarPlane, 0), TexCreate_None, TexCreate_DepthStencilTargetable, false));
#if ENGINE_MINOR_VERSION < 23
				RendererModule->RenderTargetPoolFindFreeElement(GRHICommandList.GetImmediateCommandList(), Desc, DS, TEXT("DS"));
#else
				GRenderTargetPool.FindFreeElement(GRHICommandList.GetImmediateCommandList(), Desc, DS, TEXT("DS"));
#endif
#if ENGINE_MINOR_VERSION < 22
				SetRenderTarget(RHICmdList, CanvasRenderTarget->GetRenderTargetTexture(), DS->GetRenderTargetItem().TargetableTexture, ESimpleRenderTargetMode::EClearColorAndDepth);
#else
				FRHIRenderPassInfo RPInfo(CanvasRenderTarget->GetRenderTargetTexture(), ERenderTargetActions::Clear_Store, DS->GetRenderTargetItem().TargetableTexture, EDepthStencilTargetActions::ClearDepthStencil_StoreStencilNotDepth);
				RHICmdList.BeginRenderPass(RPInfo, TEXT("ClearCDS"));
#endif
				DrawRenderState.SetDepthStencilState(TStaticDepthStencilState<true, CF_DepthNearOrEqual>::GetRHI());
			}
			else
			{
#if ENGINE_MINOR_VERSION >= 22
				FRHIRenderPassInfo RPInfo(CanvasRenderTarget->GetRenderTargetTexture(), ERenderTargetActions::Load_Store);
				RHICmdList.BeginRenderPass(RPInfo, TEXT("LoadC"));
#endif
				DrawRenderState.SetDepthStencilState(TStaticDepthStencilState<false, CF_Always>::GetRHI());
			}
			DrawRenderState.SetBlendState(TStaticBlendState<>::GetRHI());

			// draw batched items
			Elements.Draw(
				RHICmdList,
				DrawRenderState,
				DrawParameters.FeatureLevel,
				DrawParameters.bNeedsToSwitchVerticalAxis,
				SceneView,
				DrawParameters.bHitTesting,
				DrawParameters.DisplayGamma);
#if ENGINE_MINOR_VERSION >= 22
			RHICmdList.EndRenderPass();
#endif
		});
	}
}
void FArteriesEditorMode::DrawPoint(FPrimitiveDrawInterface* PDI, FArteriesPoint* Point, const FLinearColor& Color)
{
	PDI->SetHitProxy(GetHitProxy(Point));
	PDI->DrawPoint(Point->Position, Color, 8, SDPG_Foreground);
}
void FArteriesEditorMode::DrawPrimitive(FPrimitiveDrawInterface* PDI, FArteriesPrimitive* Primitive, const FLinearColor& Color)
{
	PDI->SetHitProxy(GetHitProxy(Primitive));
	for (int i = 0; i < Primitive->NumSegments(); i++)
	{
		FArteriesPoint* Start = Primitive->GetPoint(i);
		FArteriesPoint* End = Primitive->NextPoint(i);
		PDI->DrawLine(Start->Position, End->Position, Color, SDPG_Foreground);
	}
}
void FArteriesEditorMode::RebuildHitProxies()
{
	HitProxies.Empty();
	if (Object)
	{
		if (SubMode == 0)
		{
			for (FArteriesPoint& Point : Object->Points)
			{
				HHitProxy* Proxy = new HArteriesHitProxy(&Point);
				HitProxies.Add(&Point, Proxy);
			}
		}
		else if (SubMode == 1)
		{
			for (FArteriesPrimitive& Primitive : Object->Primitives)
			{
				HHitProxy* Proxy = new HArteriesHitProxy(&Primitive);
				HitProxies.Add(&Primitive, Proxy);
			}
		}
	}
	IsInvalidHitProxies = true;
}
void FArteriesEditorMode::OnSave()
{
	if (Object)
	{
		UArteriesObjectFactory* Factory = NewObject<UArteriesObjectFactory>();
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		UArteriesObject* NewObject = (UArteriesObject*)AssetTools.CreateAssetWithDialog(UArteriesObject::StaticClass(), Factory);
		NewObject->Merge_Impl(Object);
	}
}
void FArteriesEditorMode::OnSaveSelection()
{
	if (Object)
	{
		UArteriesObjectFactory* Factory = NewObject<UArteriesObjectFactory>();
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		UArteriesObject* NewObject = (UArteriesObject*)AssetTools.CreateAssetWithDialog(UArteriesObject::StaticClass(), Factory);
		if (SubMode == 0)
		{
			for (FArteriesElement* Element : SelectedElements)
			{
				if (FArteriesPoint* Point = Element->ToPoint())
				{
					FArteriesPoint* NewPoint = NewObject->AddPoint(Point);
					NewPoint->CopyValues(*Point);
				}
			}
		}
		else if (SubMode == 1)
		{
			TMap<FArteriesPoint*, FArteriesPoint*> Map;
			for (FArteriesElement* Element : SelectedElements)
			{
				if (FArteriesPrimitive* Primitive = Element->ToPrimitive())
				{
					for (FArteriesPoint* Point : Primitive->Points)
					{
						if (!Map.Contains(Point))
						{
							FArteriesPoint* NewPoint = NewObject->AddPoint(Point);
							NewPoint->CopyValues(*Point);
							Map.Add(Point, NewPoint);
						}
					}
				}
			}
			for (FArteriesElement* Element : SelectedElements)
			{
				if (FArteriesPrimitive* Primitive = Element->ToPrimitive())
				{
					FArteriesPrimitive* NewP = NewObject->AddPrimitive();
					for (int i = 0; i < Primitive->Points.Num(); i++)
						NewP->Add(Map[Primitive->Points[i]]);
					if (Primitive->IsClosed())
						NewP->MakeClose();
					NewP->Material = Primitive->Material;
					NewP->CopyValues(*Primitive);
				}
			}
		}
	}
}
void FArteriesEditorMode::OnExportSelection()
{
	if (Object && SubMode == 1)
	{
		FString Str;
		Str += TEXT("[");
		for (FArteriesElement* Element : SelectedElements)
		{
			if (FArteriesPrimitive* Primitive = Element->ToPrimitive())
			{
				Str += TEXT("[");
				for (FArteriesPoint* Point : Primitive->Points)
				{
					FVector& P = Point->Position;
					Str += FString::Printf(TEXT("[%f,%f,%f],"),P.X, P.Y, P.Z);
				}
				Str.RemoveAt(Str.Len()-1);
				Str += TEXT("],");
			}
		}
		Str.RemoveAt(Str.Len() - 1);
		Str += TEXT("]");
		FFileHelper::SaveStringToFile(Str, TEXT("f:/Polygons.txt"));
	}
}
void FArteriesEditorMode::OnDeleteClicked()
{
	if (SelectedElements.Num())
	{
		Object->Modify();
		for (FArteriesElement* Element : SelectedElements)
		{
			DeleteProxy(Element);
			if (Element->ToPoint())
				Object->DeletePoint(Element->ToPoint());
			else if (Element->ToPrimitive())
			{
				Object->DeletePrimitive(Element->ToPrimitive());
				Object->CleanPoints();
			}
		}
		ClearSelection();
	}
}
void FArteriesEditorMode::OnReverseClicked()
{
	for (FArteriesElement* Element : SelectedElements)
	{
		if (FArteriesPrimitive* Prim = Element->ToPrimitive())
			Prim->Reverse();
	}
}
void FArteriesEditorMode::OnFlipXClicked()
{
	for (FArteriesElement* Element : SelectedElements)
	{
		if (FArteriesPrimitive* Prim = Element->ToPrimitive())
		{
			for (FArteriesPoint* Point : Prim->Points)
				Point->Position.X = -Point->Position.X;
		}
	}
}
void FArteriesEditorMode::OnFlipYClicked()
{
	for (FArteriesElement* Element : SelectedElements)
	{
		if (FArteriesPrimitive* Prim = Element->ToPrimitive())
		{
			for (FArteriesPoint* Point : Prim->Points)
				Point->Position.Y = -Point->Position.Y;
		}
	}
}
void FArteriesEditorMode::OnFlipZClicked()
{
	for (FArteriesElement* Element : SelectedElements)
	{
		if (FArteriesPrimitive* Prim = Element->ToPrimitive())
		{
			for (FArteriesPoint* Point : Prim->Points)
				Point->Position.Z = -Point->Position.Z;
		}
	}
}

void FArteriesEditorMode::SetArteriesActor(AArteriesActor* InActor)
{
	if (Actor != InActor)
	{
		if (Actor)
		{
		//	Actor->OnEditorBuildCompleted.RemoveAll(this);
		}
		Actor = InActor;
		if (Actor)
		{
			Actor->OnEditorBuildCompleted.AddSP(this, &FArteriesEditorMode::SetArteriesObject);
			SetArteriesObject(Actor->FinalObject);
		}
		else
			SetArteriesObject(NULL);
	}
}
void FArteriesEditorMode::SetArteriesObject(UArteriesObject* InObject)
{
	if (Object != InObject)
	{
		Object = InObject;
		//这里不会触发PostEditChangeProperty，但是编辑器会自动刷新
		Properties->CurrentObject = InObject;
		if (Actor && Object)
		{
			if (Toolbox)
			{
				Toolbox->RefreshSelection();
				Toolbox->RefreshStats();
			}
		}
		DestroyOctree();
		ClearSelection();
		RebuildHitProxies();
		if (Object)
		{
			CreateOctree();
			RebuildPrimitiveElements();
		}
	}
}
#undef LOCTEXT_NAMESPACE