// Author: LiJiayu (JerryLi)
// Mail: lijiayu83@gmail.com (fullike@163.com)
// Copyright 2019. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "EdMode.h"
#if ENGINE_MINOR_VERSION < 23
	#include "GenericOctree.h"
#else
	#include "Math/GenericOctree.h"
#endif
#include "ArteriesEditorProperties.h"
struct FArteriesElement;
struct FArteriesPoint;
struct FArteriesEdge;
class UArteriesObject;
class AArteriesActor;
class SArteriesToolbox;
class FBlueprintEditor;
class FUICommandList;

struct FArteriesOctreeElement
{
	FArteriesOctreeElement(FArteriesPoint* InPoint);
	FArteriesOctreeElement(FArteriesPrimitive* InPrimitive);
	FBox LocalBounds;
	FBoxSphereBounds GraphBounds;
	FArteriesElement* Element;
};

struct FArteriesOctreeSemantics
{
	enum { MaxElementsPerLeaf = 16 };
	enum { MinInclusiveElementsPerNode = 7 };
	enum { MaxNodeDepth = 12 };

	typedef TInlineAllocator<MaxElementsPerLeaf> ElementAllocator;
	//typedef FDefaultAllocator ElementAllocator;

	FORCEINLINE static const FBoxSphereBounds& GetBoundingBox(const FArteriesOctreeElement& Element)
	{
		return Element.GraphBounds;
	}

	FORCEINLINE static bool AreElementsEqual(const FArteriesOctreeElement& A, const FArteriesOctreeElement& B)
	{
		return A.Element == B.Element;
	}
	static void SetElementId(const FArteriesOctreeElement& Element, FOctreeElementId Id) {}
};

typedef TOctree<FArteriesOctreeElement, FArteriesOctreeSemantics> FArteriesOctree;

struct HArteriesHitProxy : public HHitProxy
{
	DECLARE_HIT_PROXY();
	FArteriesElement* Element;
	HArteriesHitProxy(FArteriesElement* InElement) :HHitProxy(HPP_UI), Element(InElement)
	{}
	virtual EMouseCursor::Type GetMouseCursor() override
	{
		return EMouseCursor::Crosshairs;
	}
};

class FArteriesEditorMode : public FEdMode
{
public:
	static FEditorModeID ID;
	/** Constructor */
	FArteriesEditorMode();
	~FArteriesEditorMode();
	void SetBlueprintEditor(FBlueprintEditor* InBlueprintEditor);
	void OnSetMode(int InMode) { Mode = InMode; }
	bool IsInMode(int InMode) { return Mode == InMode; }
	void OnSetSubMode(int InSubMode);
	bool IsInSubMode(int InSubMode) { return SubMode == InSubMode; }
	void OnBlueprintCompiled(UBlueprint* Blueprint);
	void OnDisplayThisNode();
	void OnDisplayFinalResult();
	void OnBuild();
	void DestroyOctree()
	{
		Octree.Destroy();
	}
	void CreateOctree();
	void ClearSelection();
	void AddToSelection(FArteriesElement* Element);
	void OnSelectionChanged();
	void RebuildPrimitiveElements();
	void RebuildSelectedPrimitiveElements();
	void BuildPrimitive(FBatchedElements& Elements, UProceduralMeshComponent* Component, FArteriesPrimitive* Primitive, const FLinearColor& Color);
	void ApplyOffset(const FVector& Offset);
	FVector GetPlaneNormal(ELevelViewportType Type, FVector* TangentX=NULL, FVector* TangentY=NULL);
	FVector UpdateMovingPoint(FEditorViewportClient* InViewportClient, FViewport* InViewport, int32 InMouseX, int32 InMouseY);
	UStruct* GetSelectionType();
	FArteriesPoint* FindPointSlow(const FVector& Position);
	virtual FVector GetWidgetLocation() const;
	virtual bool ShouldDrawWidget() const;
	virtual bool GetCursor(EMouseCursor::Type& OutCursor) const;

	virtual bool CapturedMouseMove(FEditorViewportClient* InViewportClient, FViewport* InViewport, int32 InMouseX, int32 InMouseY);
	virtual bool MouseMove(FEditorViewportClient* InViewportClient, FViewport* InViewport, int32 InMouseX, int32 InMouseY);
	virtual bool HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click);
	virtual bool InputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event);
	virtual bool InputDelta(FEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale);
	virtual void Tick(FEditorViewportClient* ViewportClient, float DeltaTime);
	/** FEdMode: Render elements for the Foliage tool */
	virtual void Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI) override;
	virtual void DrawHUD(FEditorViewportClient* ViewportClient, FViewport* Viewport, const FSceneView* View, FCanvas* Canvas);
	void DrawElements(FCanvas* Canvas, FBatchedElements& Elements);
	void DrawPoint(FPrimitiveDrawInterface* PDI, FArteriesPoint* Point, const FLinearColor& Color);
	void DrawPrimitive(FPrimitiveDrawInterface* PDI, FArteriesPrimitive* Primitive, const FLinearColor& Color);
	HHitProxy* GetHitProxy(FArteriesElement* InElement)
	{
		if (TRefCountPtr<HHitProxy>* Proxy = HitProxies.Find(InElement))
			return Proxy->GetReference();
		return NULL;
	}
	void DeleteProxy(FArteriesElement* InElement)
	{
		HitProxies.Remove(InElement);
	}
	void RebuildHitProxies();
	void OnSave();
	void OnSaveSelection();
	void OnExportSelection();
	void OnDeleteClicked();
	void OnReverseClicked();
	void OnFlipXClicked();
	void OnFlipYClicked();
	void OnFlipZClicked();
	void SetArteriesActor(AArteriesActor* InActor);
	void SetArteriesObject(UArteriesObject* InObject);
	int Mode;
	int SubMode;
	bool IsLeftDragging;
	bool IsInvalidHitProxies;
	AArteriesActor* Actor;
	UArteriesObject* Object;
	SArteriesToolbox* Toolbox;
	UArteriesEditorProperties* Properties;
	FBlueprintEditor* BlueprintEditor;
	TSet<FArteriesElement*> SelectedElements;
	FBox SelectionBounds;
	FVector MouseDownPoint;
	FVector MovingPoint;
	FArteriesOctree Octree;
	FMatrix ViewProjectionMatrix;
	TRefCountPtr<HHitProxy> HoveredProxy;
	TMap<FArteriesElement*, TRefCountPtr<HHitProxy>> HitProxies;
	FBatchedElements SelectedPrimitiveElements;
	FBatchedElements PrimitiveElements;
	TSharedPtr<FUICommandList> UICommandList;
#if ENGINE_MINOR_VERSION < 23
	IRendererModule* RendererModule;
#endif
};