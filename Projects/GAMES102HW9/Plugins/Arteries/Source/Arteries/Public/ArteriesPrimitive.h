// Author: LiJiayu (JerryLi)
// Mail: lijiayu83@gmail.com (fullike@163.com)
// Copyright 2019. All Rights Reserved.

#pragma once
#include "ArteriesElement.h"
#include "ArteriesPoint.h"
#include "ProceduralMeshComponent.h"
#include "ArteriesPrimitive.generated.h"
class FArteriesPointMapper;
USTRUCT(BlueprintType)
struct ARTERIES_API FArteriesPrimitive : public FArteriesElement
{
	GENERATED_USTRUCT_BODY()
	FArteriesPrimitive();
	virtual UStruct* GetStruct() { return FArteriesPrimitive::StaticStruct(); }
	virtual struct FArteriesPrimitive* ToPrimitive() { return this; }
	void CalculateNormal();
	void CalculateTangents();
	FMatrix GetLocalToWorldTransform() const;
	FMatrix GetToUVTransform() const;
	void Build(FProcMeshSection& Section, TMap<FArteriesPoint*, int32>& VerticesMapper, TArray<FArteriesPrimitive*>* Holes);
	TArray<uint16> Triangulate(const FVector& PolyNormal);
	TArray<FVector> GetDirections() const;
	TArray<FMatrix> GetTransforms() const;
//	TArray<FTransform> GetTargets();
	TArray<float> GetDists(TArray<FVector>* OutDirections = NULL) const;
	void Add(FArteriesPoint* Point)
	{
		if (Points.Num())
		{
			FArteriesPoint* Last = Points.Last();
			if (Point == Last)
				return;
			Last->AddLink(Point, this, false);
			Point->AddLink(Last, this, true);
			if (Point == Points[0])
			{
				bClosed = true;
				return;
			}
		}
		Points.Add(Point);
	}
	bool Insert(FArteriesPoint* Point, int Index)
	{
		FArteriesPoint* Prev = Points[(Index - 1 + Points.Num()) % Points.Num()];
		FArteriesPoint* Next = Points[Index % Points.Num()];
		if (Prev == Point || Next == Point)
			return false;
		Prev->DeleteLinks(Next, this, false);
		Next->DeleteLinks(Prev, this, true);
		Points.Insert(Point, Index);
		Prev->AddLink(Point, this, false);
		Point->AddLink(Next, this, false);
		Next->AddLink(Point, this, true);
		Point->AddLink(Prev, this, true);
		return true;
	}
	void Insert(TArray<FArteriesPoint*>& InPoints, int Index)
	{
		FArteriesPoint* Prev = Points[(Index - 1 + Points.Num()) % Points.Num()];
		FArteriesPoint* Next = Points[Index % Points.Num()];
		Prev->DeleteLinks(Next, this, false);
		Next->DeleteLinks(Prev, this, true);
		for (int i = 0; i < InPoints.Num(); i++)
		{
			FArteriesPoint* Point = InPoints[i];
			Points.Insert(Point, Index+i);
			Prev->AddLink(Point, this, false);
			Point->AddLink(Prev, this, true);
			Prev = Point;
		}
		Prev->AddLink(Next, this, false);
		Next->AddLink(Prev, this, true);
	}
	TArray<FArteriesPoint*> GetCutPoints(int Index)
	{
		TArray<FArteriesPoint*> Out;
		for (int i = 1; i < Points.Num()-1; i++)
			Out.Add(Points[(Index + i) % Points.Num()]);
		return MoveTemp(Out);
	}
	void Delete(int Index, int Count = 1)
	{
		FArteriesPoint* Start = Points[(Index - 1 + Points.Num()) % Points.Num()];
		FArteriesPoint* End = Points[(Index + Count) % Points.Num()];
		TArray<int> Removes;
		for (int i = 0; i <= Count; i++)
		{
			int ThisIndex = (Index + i) % Points.Num();
			if (i < Count)
				Removes.Add(ThisIndex);
			FArteriesPoint* Prev = Points[(Index + i - 1 + Points.Num()) % Points.Num()];
			FArteriesPoint* This = Points[ThisIndex];
			Prev->DeleteLinks(This, this, false);
			This->DeleteLinks(Prev, this, true);
		}
		Start->AddLink(End, this, false);
		End->AddLink(Start, this, true);
		Removes.Sort();
		for (int i = Removes.Num() - 1; i >= 0; i--)
			Points.RemoveAt(Removes[i]);
	}
	int Find(FArteriesPoint* Point)
	{
		return Points.Find(Point);
	}
	void RemoveDuplicatePoints()
	{
		for (int i = 0; i < NumSegments(); i++)
		{
			int Next = (i + 1) % Points.Num();
			if (Points[i] == Points[Next])
				Delete(Next);
		}
	}
	bool IsClosed() const
	{
		return bClosed;
	}
	bool IsSelfIntersect() const
	{
		for (FArteriesPoint* Point : Points)
		{
			if (Point->IsSelfIntersect())
				return true;
		}
		return false;
	}
	bool IsValidPolygon() const
	{
		if (!bClosed) return false;
		float TotalRadian = 0;
		FVector PolyNormal = GetVec3(AVN_TangentZ);
		for (int i = 0; i < NumSegments(); i++)
		{
			FArteriesPoint* Prev = PrevPoint(i);
			FArteriesPoint* This = GetPoint(i);
			FArteriesPoint* Next = NextPoint(i);
			FVector PrevDirection = (Prev->Position - This->Position).GetSafeNormal();
			FVector NextDirection = (Next->Position - This->Position).GetSafeNormal();
			FQuat Quat = FQuat::FindBetweenNormals(NextDirection, PrevDirection);
			FVector Axis;
			float Angle;
			Quat.ToAxisAndAngle(Axis, Angle);
			TotalRadian += (PolyNormal | Axis) > 0.f ? Angle : PI * 2 - Angle;
		}
		float CorrectRadian = (NumSegments() - 2)*PI;
		return FMath::IsNearlyEqual(TotalRadian, CorrectRadian, 0.01f);
	}
	void MakeClose()
	{
		if (!bClosed && Points.Num() > 0)
		{
			FArteriesPoint* Prev = Points.Last();
			FArteriesPoint* Next = Points[0];
			Prev->AddLink(Next, this, false);
			Next->AddLink(Prev, this, true);
			bClosed = true;
		}
	}
	int NumSegments()const
	{
		return Points.Num() - !IsClosed();
	}
	int NumPoints()const
	{
		return Points.Num();
	}
	FArteriesPoint* PrevPoint(int Index)const
	{
		if (Index > 0) return Points[Index - 1];
		if (bClosed) return Points.Last();
		return NULL;
	}
	FArteriesPoint* GetPoint(int Index)const
	{
		return Points[Index];
	}
	FArteriesPoint* NextPoint(int Index)const
	{
		if (Index + 1 < Points.Num()) return Points[Index + 1];
		if (bClosed) return Points[0];
		return NULL;
	}
	void DeleteAllLinks();
	bool HasAnyValidLinks();
	float GetHalfWidth(float DefaultWidth) const
	{
		if (HasFloat("width"))
			return GetFloat("width") / 2;
		return DefaultWidth / 2;
	}
	FBox GetBox(FMatrix* Matrix = NULL, TArray<FVector>* LocalPositions = NULL) const
	{
		FBox Box(EForceInit::ForceInit);
		if (Matrix)
		{
			for (const FArteriesPoint* Point : Points)
			{
				FVector Position = Matrix->TransformPosition(Point->Position);
				Box += Position;
				if (LocalPositions) LocalPositions->Add(Position);
			}
		}
		else
		{
			for (const FArteriesPoint* Point : Points)
			{
				Box += Point->Position;
				if (LocalPositions) LocalPositions->Add(Point->Position);
			}
		}
		return Box;
	}
	void GetOrientedBox(FMatrix& OutMatrix, FVector& OutSize) const;
	float CalculateArea() const;
	FVector Centroid();
	int LongestSegment() const;
	void Clip(FArteriesPointMapper& Mapper, const FPlane& Plane, TArray<FArteriesPoint*>& PPrim, TArray<FArteriesPoint*>& NPrim);
	void Reverse();
	void Serialize(FArchive& Ar, UArteriesObject* Object, TMap<FArteriesPoint*, int>& PointIDs, TMap<FArteriesPrimitive*, int>& PrimitiveIDs);

	UPROPERTY(EditAnywhere, Category = ArteriesPrimitive)
	UMaterialInterface* Material;
	UPROPERTY(EditAnywhere, Category = ArteriesPrimitive)
	uint32 bClosed : 1;
#if WITH_EDITORONLY_DATA
	uint32 StartIndex;
	uint32 NumTriangles;
#endif
	TArray<FArteriesPoint*> Points;
	FArteriesPrimitive* Outer;
};

class FArteriesPrimitiveArray : public TIndirectArray<FArteriesPrimitive>
{
public:
	void Serialize(FArchive& Ar, UArteriesObject* Object, TMap<FArteriesPoint*, int>& PointIDs, TMap<FArteriesPrimitive*, int>& PrimitiveIDs)
	{
		CountBytes(Ar);
		if (Ar.IsLoading() || Ar.IsCountingMemory())
		{
			// Load array.
			int32 NewNum;
			Ar << NewNum;
			Empty(NewNum);
			for (int32 Index = 0; Index < NewNum; Index++)
			{
				Add(new FArteriesPrimitive);
			}
			for (int32 Index = 0; Index < NewNum; Index++)
			{
				(*this)[Index].Serialize(Ar, Object, PointIDs, PrimitiveIDs);
			}
		}
		else
		{
			// Save array.
			int32 N = Num();
			Ar << N;
			for (int32 Index = 0; Index < N; Index++)
			{
				(*this)[Index].Serialize(Ar, Object, PointIDs, PrimitiveIDs);
			}
		}
	}
};


bool ARTERIES_API IsPointInside(const TArray<FVector>& Points, const FVector2D& pt);
int ARTERIES_API Intersect(const TArray<FVector>& Points, const FVector2D& P0, const FVector2D& P1, float& MinT);