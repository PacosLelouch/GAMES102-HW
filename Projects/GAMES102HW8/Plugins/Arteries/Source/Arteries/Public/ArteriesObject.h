// Author: LiJiayu (JerryLi)
// Mail: lijiayu83@gmail.com (fullike@163.com)
// Copyright 2019. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "ArteriesPoint.h"
#include "ArteriesPrimitive.h"
#include "ArteriesGroup.h"
#include "Math/IntPoint.h"
#include "ArteriesObject.generated.h"

inline int GetElementIndex(const TIndirectArray<FArteriesElement>& Elements, FArteriesElement* Element)
{
	for (int i = 0; i < Elements.Num(); i++)
	{
		if (&Elements[i] == Element)
			return i;
	}
	return INDEX_NONE;
}

class AArteriesActor;

UENUM(BlueprintType)
enum class EArteriesGroupType : uint8
{
	Point,
	Primitive,
	Object UMETA(DisplayName = "Object (only once)")
};

UENUM(BlueprintType)
enum class EArteriesAlignType : uint8
{
	Start,
	Center,
	End,
};

UENUM(BlueprintType)
enum class EArteriesTangentOutputType : uint8
{
	None,
	Flatten,
	Normal,
};

USTRUCT(BlueprintType)
struct FArteriesGroupRange
{
	GENERATED_USTRUCT_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ArteriesGroup)
	FName Name;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ArteriesGroup)
	int32 Start = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ArteriesGroup)
	int32 End = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ArteriesGroup)
	int32 Select = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ArteriesGroup)
	int32 Of = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ArteriesGroup)
	int32 Offset = 0;
};

USTRUCT()
struct FArteriesInstances
{
	GENERATED_USTRUCT_BODY()
	void Add(const FTransform& T)
	{
		Transforms.Add(T);
	}
	void Transform(const FTransform& T)
	{
		for (FTransform& Trans : Transforms)
			Trans *= T;
	}
	void Merge(const FArteriesInstances& Other, FTransform* T)
	{
		for (const FTransform& Trans : Other.Transforms)
			Transforms.Add(T ? Trans * (*T) : Trans);
	}
	UPROPERTY(EditAnywhere, Category = ArteriesInstances)
	TArray<FTransform> Transforms;
};

UCLASS(BlueprintType, Blueprintable)
class ARTERIES_API UArteriesObject :public UObject
{
	GENERATED_UCLASS_BODY()
public:
	static UArteriesObject* New_Impl();
	static UArteriesObject* SubmitResult(UArteriesObject* Obj);
	static void GetTangents(int FaceIndex, FVector* X, FVector* Y, FVector* Z);

	UFUNCTION(BlueprintPure, Category = Arteries, meta = (AdvancedDisplay = 1))
	static FTransform Flatten(const FTransform& Transform);

	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	static UArteriesObject* New();

	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	static UArteriesObject* Line(FVector Origin = FVector::ZeroVector, FVector Direction = FVector(0,0,1), float Length = 100.f, int NumPoints = 2);
	
	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	static UArteriesObject* Grid(FVector Origin = FVector::ZeroVector, FRotator Rotation = FRotator::ZeroRotator, FVector2D Size = FVector2D(100.f, 100.f), int NumPointsX = 2, int NumPointsY = 2);

	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	static UArteriesObject* Circle(FVector Origin = FVector::ZeroVector, FRotator Rotation = FRotator::ZeroRotator, FVector2D Radius = FVector2D(100.f, 100.f), int NumPoints = 32);

	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	static UArteriesObject* Sphere(FVector Origin = FVector::ZeroVector, FRotator Rotation = FRotator::ZeroRotator, FVector Radius = FVector(100.f, 100.f, 100.f), int Rows = 16, int Columns = 32);

	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	static UArteriesObject* Tube(FVector Origin = FVector::ZeroVector, FRotator Rotation = FRotator::ZeroRotator, FVector2D Radius = FVector2D(100.f, 100.f), float Height = 100.f, int Rows = 2, int Columns = 32);

	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	static UArteriesObject* Torus(FVector Origin = FVector::ZeroVector, FRotator Rotation = FRotator::ZeroRotator, FVector2D Radius = FVector2D(100.f, 100.f), int Rows = 16, int Columns = 32);

	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	static UArteriesObject* Box(FVector Origin = FVector::ZeroVector, FRotator Rotation = FRotator::ZeroRotator, FVector Size = FVector(100.f, 100.f, 100.f), int NumPointsX = 2, int NumPointsY = 2, int NumPointsZ = 2);
	
	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	static UArteriesObject* Merge(UArteriesObject* Obj0, UArteriesObject* Obj1, UArteriesObject* Obj2, UArteriesObject* Obj3, UArteriesObject* Obj4);

	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	static UArteriesObject* MergeArray(const TArray<UArteriesObject*>& Objs);

	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	UArteriesObject* Add(UObject* Source, FTransform Transform);

	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	UArteriesObject* Arc(const FString& Groups, int NumSegments = 4, float Angle = 90.f);

	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	UArteriesObject* Copy();

	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	UArteriesObject* CopyAndTransform(int NumCopies, FTransform Transform);

	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	UArteriesObject* CopyToPoints(const FString& Groups, UObject* Source, FTransform LocalTransform);

	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	UArteriesObject* Execute(const EArteriesGroupType ForEach, const FString& Code);

	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	UArteriesObject* Carve(EArteriesAlignType StartAlign = EArteriesAlignType::Start, float StartU = 0, EArteriesAlignType EndAlign = EArteriesAlignType::End, float EndU = 0);

	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	UArteriesObject* BreakPoints(const FString& Groups);

	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	UArteriesObject* Scatter(const FString& Groups, int Seed, int Count, float Density = 0.f, int Iterations = 10);

	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	UArteriesObject* Voronoi(const FString& Groups, int Seed, int Count, float Density = 0.f, int Iterations = 10);

	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	UArteriesObject* SubDivide(const FString& Groups, float MinLength = 1000.f);

	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	UArteriesObject* Blast(const FString& Groups, const FString& Tags = "", bool DeleteNonSelected = false);

	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	UArteriesObject* Facet(float Tolerance = 1.e-4f);

	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	UArteriesObject* Triangulate();

	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	UArteriesObject* Divide(float Tolerance = 0.999f);

	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	UArteriesObject* Fuse(const FString& Groups, float SnapDist);

	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	UArteriesObject* Clean();

	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	UArteriesObject* Hole(const FString& OuterGroups, const FString& InnerGroups, float Tolerance = 10.f);

	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	UArteriesObject* SortRandomly(int Seed);

	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	UArteriesObject* SortByAttribute(FName AttrName);

	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	UArteriesObject* GroupRange(const TArray<FArteriesGroupRange>& Groups);

	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	UArteriesObject* Measure(FName Name);

	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	UArteriesObject* Bridge(FName StartGroupName, FName EndGroupName);

	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	UArteriesObject* Road(float DefaultWidth = 1000.f);

	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	UArteriesObject* Ground();

	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	UArteriesObject* MakeGrids(const FString& Groups, float GridSize = 0.f, int MinGrids = 3, bool OBBOnly=false);

	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	UArteriesObject* PolyExpand(const FString& Groups, float Offset, FName CurveGroup = NAME_None, FName SurfaceGroup = "Surface", bool OutputTangents = true, bool DeleteSource = true);

	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	UArteriesObject* Clip(const FString& Groups, FVector PlaneNormal = FVector(1,0,0), float PlaneDist = 0, FName PositiveGroup = "ClipPos", FName NegativeGroup = "ClipNeg");

	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	UArteriesObject* Resample(bool ByEdge = true, float SegmentLength = 100.f, int NumSegments = 0, EArteriesTangentOutputType OutputType = EArteriesTangentOutputType::Normal);

	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	UArteriesObject* Reverse(const FString& Groups);
	
	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	UArteriesObject* Transform(const EArteriesGroupType GroupType, const FString& Groups, FRotator Rotation = FRotator::ZeroRotator, FVector Translation = FVector::ZeroVector, FVector Scale = FVector(1,1,1));

	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	UArteriesObject* PolyBevel(const EArteriesGroupType GroupType, const FString& Groups, float Distance, int Divisions = 2, bool bBevelSingleCurve = true);

	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	UArteriesObject* PolyExtrude(const FString& Groups, float Distance, float Inset = 0.f, FName FrontGroup = "Front", FName SideGroup = "Side", bool DeleteSource = true);

	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	UArteriesObject* Sweep(UArteriesObject* Backbones);

	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	UArteriesObject* SetMaterial(const FString& Groups, UMaterialInterface* Material, FVector2D UVScale = FVector2D(0.01f, 0.01f), FVector2D UVOffset = FVector2D::ZeroVector, float UVRotation = 0, bool NullOnly = false);
	
	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	int NumPoints() const{ return Points.Num(); }
	
	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	int LastPoint() const { return Points.Num() - 1; }

	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	int NumPrimitives() const{ return Primitives.Num(); }
	
	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	int LastPrimitive() const { return Primitives.Num() - 1; }

	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	int GetPointInt(FName Key, int Index) const{ return Points[Index].GetInt(Key); }
	
	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	int GetPrimitiveInt(FName Key, int Index) const{ return Primitives[Index].GetInt(Key); }

	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	bool PointInGroup(FName GroupName, int Index) const { return PointGroups.Contains(GroupName) && PointGroups[GroupName].Points.Find((FArteriesPoint*)&Points[Index]) != INDEX_NONE; }

	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	bool PrimitiveInGroup(FName GroupName, int Index) const { return PrimitiveGroups.Contains(GroupName) && PrimitiveGroups[GroupName].Primitives.Find((FArteriesPrimitive*)&Primitives[Index]) != INDEX_NONE; }

	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (DisplayName = "GetPoint", AdvancedDisplay = 1))
	FTransform GetPointTransform(int Index) const { return Points[Index].GetTransform(); }

	FArteriesPoint* GetPoint(int Index) { return Points.IsValidIndex(Index) ? &Points[Index] : NULL; }
	FArteriesPrimitive* GetPrimitive(int Index) { return Primitives.IsValidIndex(Index) ? &Primitives[Index] : NULL; }

	TSet<FArteriesPoint*> GetPoints(const FString& Str);
	TSet<TPair<FArteriesPoint*, FArteriesPoint*>> GetEdges(const FString& Str);
	TSet<FArteriesPrimitive*> GetPrimitives(const FString& Str);
	TSet<FArteriesPrimitive*> GetPrimitivesWithTags(const FString& Tags);

	void SetPointGroup(FArteriesPoint* Point, const FName& GroupName, bool State, bool Unique = true);
	void SetPointGroupANSI(FArteriesPoint* Point, const char* GroupName, bool State);
	void SetPrimitiveGroup(FArteriesPrimitive* Primitive, const FName& GroupName, bool State, bool Unique = true);
	void SetPrimitiveGroupANSI(FArteriesPrimitive* Primitive, const char* GroupName, bool State);

	FArteriesPoint* AddPoint();
	FArteriesPoint* AddPoint(const FVector& InPosition);
	FArteriesPoint* AddPoint(const FArteriesPoint* P, FTransform* Transform = NULL);
	FArteriesPoint* AddPointLerp(const FArteriesPoint* P0, const FArteriesPoint* P1, float Alpha);
	void DeletePoint(int Index);
	void DeletePoint(FArteriesPoint* InPoint);

	FArteriesPrimitive* AddPrimitive();
	FArteriesPrimitive* AddPrimitive(const TArray<FArteriesPoint*>& InPoints, bool bClosed);
	void DeletePrimitive(int Index);
	void DeletePrimitive(FArteriesPrimitive* InPrimitive);
	void DeleteEdge(TPair<FArteriesPoint*, FArteriesPoint*> Edge);
	void CleanPoints();
	void CleanPrimitives();
	FArteriesPrimitive* AddLine(FVector Origin, FVector Direction, float Length, int NumPoints);
	TArray<FArteriesPrimitive*> AddGrid(FVector Origin, FVector2D Size, int NumPointsX, int NumPointsY);
	FArteriesPrimitive* AddCircle(FVector Origin, FVector2D Radius, int NumPoints);
	UArteriesObject* Copy_Impl();
	void Clip_Impl(const FString& Groups, const FPlane& Plane, const FName& PositiveGroup, const FName& NegativeGroup);
	void Bridge_Impl(FArteriesPrimitive* From, FArteriesPrimitive* To);
	UArteriesObject* Sweep_Impl(UArteriesObject* Backbones);
	void Merge_Impl(UArteriesObject* Obj, FTransform* Transform = NULL);
	void Transform_Impl(const FTransform& Transform, const EArteriesGroupType GroupType = EArteriesGroupType::Point, const FString& Groups = "");
	void Build(TMap<UMaterialInterface*, FProcMeshSection>& Sections);
	virtual void Serialize(FArchive& Ar);
	virtual void PostLoad();
	virtual bool IsPostLoadThreadSafe() const
	{
		return true;
	}
	void RemoveAllLinks();
	void RebuildLinks();
	int GetPointIndex(FArteriesPoint* Point) const { return GetElementIndex((const TIndirectArray<FArteriesElement>&)Points, Point);  }
	int GetPrimitiveIndex(FArteriesPrimitive* Primitive) const { return GetElementIndex((const TIndirectArray<FArteriesElement>&)Primitives, Primitive); }
	FBox GetBox() const
	{
		FBox Box(EForceInit::ForceInit);
		for (const FArteriesPoint& Point : Points)
			Box += Point.Position;
		return Box;
	}
	void ClearAll()
	{
		PointGroups.Empty();
		EdgeGroups.Empty();
		PrimitiveGroups.Empty();
		InstancesMap.Empty();
		Points.Empty();
		Primitives.Empty();
	}
	UPROPERTY(EditAnywhere, Category = ArteriesObject)
	TMap<FName, FArteriesPointGroup> PointGroups;
	UPROPERTY(EditAnywhere, Category = ArteriesObject)
	TMap<FName, FArteriesEdgeGroup> EdgeGroups;
	UPROPERTY(EditAnywhere, Category = ArteriesObject)
	TMap<FName, FArteriesPrimitiveGroup> PrimitiveGroups;
	UPROPERTY(EditAnywhere, Category = ArteriesObject)
	TMap<UStaticMesh*, FArteriesInstances> InstancesMap;
	FArteriesPointArray Points;
	FArteriesPrimitiveArray Primitives;
};

class ARTERIES_API FArteriesPointMapper
{
public:
	FArteriesPointMapper(UArteriesObject* InObject, const FString& Groups = "", float InSnapDist = 1.f);
	FArteriesPoint* GetPoint(const FVector& InPosition);
	FArteriesPoint* GetPoint(const FArteriesPoint* Point);
	FArteriesPoint* GetPointLerp(const FArteriesPoint* P0, const FArteriesPoint* P1, float Alpha);
	void Fuse();
	UArteriesObject* Object;
	TMap<FIntVector, TArray<FArteriesPoint*>> Points;
	float SnapDist;
};