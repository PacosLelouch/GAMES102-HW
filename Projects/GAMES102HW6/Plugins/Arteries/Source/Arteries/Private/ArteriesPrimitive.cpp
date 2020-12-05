// Author: LiJiayu (JerryLi)
// Mail: lijiayu83@gmail.com (fullike@163.com)
// Copyright 2019. All Rights Reserved.

#include "ArteriesPrimitive.h"
#include "ArteriesObject.h"
#include "ArteriesCustomVersion.h"
#include "Arteries.h"
#include "GluContext.h"
#include "GeomTools.h"
/** Given a polygon, decompose into triangles. */
bool TriangulatePoly(TArray<uint32>& OutTris, const TArray<FProcMeshVertex>& PolyVerts, TArray<int32>& VertIndices, const FVector& PolyNormal)
{
	const int32 TriBase = OutTris.Num();
	// Keep iterating while there are still vertices
	while (VertIndices.Num() >= 3)
	{
		// Look for an 'ear' triangle
		bool bFoundEar = false;
		for (int32 EarVertexIndex = 0; EarVertexIndex < VertIndices.Num(); EarVertexIndex++)
		{
			// Triangle is 'this' vert plus the one before and after it
			const int32 AIndex = (EarVertexIndex == 0) ? VertIndices.Num() - 1 : EarVertexIndex - 1;
			const int32 BIndex = EarVertexIndex;
			const int32 CIndex = (EarVertexIndex + 1) % VertIndices.Num();

			const FProcMeshVertex& AVert = PolyVerts[VertIndices[AIndex]];
			const FProcMeshVertex& BVert = PolyVerts[VertIndices[BIndex]];
			const FProcMeshVertex& CVert = PolyVerts[VertIndices[CIndex]];

			// Check that this vertex is convex (cross product must be positive)
			const FVector ABEdge = BVert.Position - AVert.Position;
			const FVector ACEdge = CVert.Position - AVert.Position;
			const float TriangleDeterminant = (ABEdge ^ ACEdge) | PolyNormal;
			if (TriangleDeterminant > 0.f)
			{
				continue;
			}

			bool bFoundVertInside = false;
			// Look through all verts before this in array to see if any are inside triangle
			for (int32 VertexIndex = 0; VertexIndex < VertIndices.Num(); VertexIndex++)
			{
				const FProcMeshVertex& TestVert = PolyVerts[VertIndices[VertexIndex]];

				if (VertexIndex != AIndex &&
					VertexIndex != BIndex &&
					VertexIndex != CIndex &&
					FGeomTools::PointInTriangle(AVert.Position, BVert.Position, CVert.Position, TestVert.Position))
				{
					bFoundVertInside = true;
					break;
				}
			}

			// Triangle with no verts inside - its an 'ear'! 
			if (!bFoundVertInside)
			{
				OutTris.Add(VertIndices[AIndex]);
				OutTris.Add(VertIndices[CIndex]);
				OutTris.Add(VertIndices[BIndex]);

				// And remove vertex from polygon
				VertIndices.RemoveAt(EarVertexIndex);

				bFoundEar = true;
				break;
			}
		}

		// If we couldn't find an 'ear' it indicates something is bad with this polygon - discard triangles and return.
		if (!bFoundEar)
		{
			OutTris.SetNum(TriBase, true);
			return false;
		}
	}

	return true;
}
bool DoLineSegmentsIntersect(const FVector2D& Segment1Start, const FVector2D& Segment1End, const FVector2D& Segment2Start, const FVector2D& Segment2End, float* T1=NULL, float* T2=NULL)
{
	const FVector2D Segment1Dir = Segment1End - Segment1Start;
	const FVector2D Segment2Dir = Segment2End - Segment2Start;
	const float Determinant = FVector2D::CrossProduct(Segment1Dir, Segment2Dir);
	if (!FMath::IsNearlyZero(Determinant, KINDA_SMALL_NUMBER))
	{
		const FVector2D SegmentStartDelta = Segment2Start - Segment1Start;
		const float OneOverDet = 1.0f / Determinant;
		const float Seg1Intersection = FVector2D::CrossProduct(SegmentStartDelta, Segment2Dir) * OneOverDet;
		const float Seg2Intersection = FVector2D::CrossProduct(SegmentStartDelta, Segment1Dir) * OneOverDet;
		//	const float Epsilon = 1 / 65536.0f;
		if (T1) *T1 = Seg1Intersection;
		if (T2) *T2 = Seg2Intersection;
		return (Seg1Intersection >= -KINDA_SMALL_NUMBER && Seg1Intersection <= 1.0f + KINDA_SMALL_NUMBER && Seg2Intersection >= -KINDA_SMALL_NUMBER && Seg2Intersection <= 1.0f + KINDA_SMALL_NUMBER);
	}
	return false;
}
static void SerializeArray(FArchive& Ar, TArray<FArteriesElement*>& Array, TIndirectArray<FArteriesElement>& Elements, TMap<FArteriesElement*,int>& Map)
{
	int Num = Array.Num();
	Ar << Num;
	if (Ar.IsLoading())
	{
		Array.Empty(Num);
		for (int i = 0; i < Num; i++)
		{
			int Idx;
			Ar << Idx;
			Array.Add(&Elements[Idx]);
		}
	}
	else
	{
		for (int i = 0; i < Num; i++)
		{
			int Idx = Map[Array[i]];
			Ar << Idx;
		}
	}
}

FArteriesPrimitive::FArteriesPrimitive():Material(NULL),bClosed(0),
#if WITH_EDITORONLY_DATA
	StartIndex(0),
	NumTriangles(0),
#endif
	Outer(NULL)
{
}
void FArteriesPrimitive::CalculateNormal()
{
	FVector TangentZ = FVector::ZeroVector;
	int v0 = 0;
	for (int v1 = 1; v1 < Points.Num() - 1; v1++)
	{
		int v2 = v1 + 1;
		const FVector DPosition1 = Points[v1]->Position - Points[v0]->Position;
		const FVector DPosition2 = Points[v2]->Position - Points[v0]->Position;
		TangentZ += FVector::CrossProduct(DPosition1, DPosition2);
	}
	SetVec3(AVN_TangentZ, TangentZ.GetSafeNormal());
}
void FArteriesPrimitive::CalculateTangents()
{
	if (!HasVec3(AVN_TangentZ))
		CalculateNormal();
	FVector TangentZ = GetVec3(AVN_TangentZ);
	if (HasVec3(AVN_TangentX))
	{
		if (!HasVec3(AVN_TangentY))
			SetVec3(AVN_TangentY, (TangentZ ^ GetVec3(AVN_TangentX)).GetSafeNormal());
	}
	else if (HasVec3(AVN_TangentY))
		SetVec3(AVN_TangentX, (GetVec3(AVN_TangentY) ^ TangentZ).GetSafeNormal());
	else
	{
		FVector TangentX = TangentZ ^ FVector::ForwardVector;
		float Size = TangentX.Size();
		if (Size < 0.0174524f)
			TangentX = (TangentZ ^ FVector::UpVector).GetSafeNormal();
		else
			TangentX = TangentX / Size;
		SetVec3(AVN_TangentX, TangentX);
		SetVec3(AVN_TangentY, (TangentZ ^ TangentX).GetSafeNormal());
	}
}
FMatrix FArteriesPrimitive::GetLocalToWorldTransform() const
{
	FMatrix Matrix = FMatrix(GetVec3(AVN_TangentX), GetVec3(AVN_TangentY), GetVec3(AVN_TangentZ), Points[0]->Position);
	return MoveTemp(Matrix);
}
FMatrix FArteriesPrimitive::GetToUVTransform() const
{
	FVector2D BaseUV;
	FArteriesPoint* BasePoint = NULL;
	for (FArteriesPoint* Point : Points)
	{
		if (Point->HasVec2(AVN_UV0))
		{
			BasePoint = Point;
			BaseUV = Point->GetVec2(AVN_UV0);
			break;
		}
	}
	if (!BasePoint)
	{
		BasePoint = Points[0];
		BaseUV = FVector2D::ZeroVector;
	}
	FMatrix ToUV = FMatrix(GetVec3(AVN_TangentX), GetVec3(AVN_TangentY), GetVec3(AVN_TangentZ), BasePoint->Position).Inverse() * FTranslationMatrix(FVector(BaseUV, 0));
	return MoveTemp(ToUV);
}
void FArteriesPrimitive::Build(FProcMeshSection& Section, TMap<FIntVector, TArray<int>>& VerticesMapper, TArray<FArteriesPrimitive*>* Holes)
{
	if (Points.Num() < 3 || !IsClosed() || (Outer && !Material))
		return;
	FMatrix WorldToUV = GetToUVTransform();
	FMatrix UVToWorld = WorldToUV.Inverse();
	auto CreateVertex = [&](FArteriesPoint* Point, bool bHole)->int
	{
		FVector2D UV;
		if (Point->HasVec2(AVN_UV0))
			UV = Point->GetVec2(AVN_UV0);
		else
			UV = FVector2D(WorldToUV.TransformPosition(Point->Position));
		if (HasVec2(AVN_UVScale))
			UV *= GetVec2(AVN_UVScale);
		else
			UV *= 0.01f;
		FVector X, Y, Z;
		//目前默认夹角小于30度可以Smooth
		//const float Cos30 = 0.866f;
		float SmoothThreshold = HasFloat(AVN_Smooth) ? GetFloat(AVN_Smooth) : 0.866f;
		Point->GetSmoothTangentsForPrimitive(this, X, Y, Z, SmoothThreshold);
		//FIntVector Key = Point->IntKey(1.f);
		//TArray<int>& Indices = VerticesMapper.FindOrAdd(Key);
		//for (int& Idx : Indices)
		//{
		//	FProcMeshVertex& Vert = Section.ProcVertexBuffer[Idx];
		//	if (Vert.Normal.Equals(Z) && Vert.UV0.Equals(UV))
		//		return Idx;
		//}
		int Index = Section.ProcVertexBuffer.AddDefaulted();
		//Indices.Add(Index);
		FProcMeshVertex& Vert = Section.ProcVertexBuffer[Index];
		if (bHole)
			Vert.Position = UVToWorld.TransformPosition(FVector(FVector2D(WorldToUV.TransformPosition(Point->Position)), 0));
		else
			Vert.Position = Point->Position;
		Vert.Tangent = FProcMeshTangent(X, false);
		Vert.Normal = Z;
		Vert.UV0 = UV;
		if (Point->HasVec2(AVN_UV1))
		{
			Vert.UV1 = Point->GetVec2(AVN_UV1);
		}
		if (Point->HasVec2(AVN_UV2))
		{
			Vert.UV1 = Point->GetVec2(AVN_UV2);
		}
		if (Point->HasVec2(AVN_UV3))
		{
			Vert.UV1 = Point->GetVec2(AVN_UV3);
		}
		if (Point->HasVec3(AVN_Color))
		{
			Vert.Color = FLinearColor(Point->GetVec3(AVN_Color)).ToFColor(true);
		}
		Section.SectionLocalBox += Vert.Position;
		return Index;
	};
#if WITH_EDITORONLY_DATA
	StartIndex = Section.ProcIndexBuffer.Num();
#endif
	//if (Holes)
	if (Points.Num() > 3)
	{
		GluTriangulator Triangulator(Section);
		auto AddContour = [&](FArteriesPrimitive* Prim, bool bHole)
		{
			Triangulator.BeginContour();
			for (FArteriesPoint* Point : Prim->Points)
			{
				int Index = CreateVertex(Point, bHole);
				FProcMeshVertex& Vertex = Section.ProcVertexBuffer[Index];
				//Triangulator.Vertex(Vertex.UV0, Index);
				Triangulator.Vertex(WorldToUV.TransformPosition(Vertex.Position), Index);
			}
			Triangulator.EndContour();
		};
		Triangulator.Begin(TESS_WINDING_ODD, true);
		AddContour(this, false);
		if (Holes)
		{
			for (FArteriesPrimitive* Hole : *Holes)
				AddContour(Hole, true);
		}
		Triangulator.End();
	}
	else
	{
		TArray<int32> VertIndices;
		for (int i = 0; i < Points.Num(); i++)
		{
			FArteriesPoint* Point = Points[i];
			VertIndices.Add(CreateVertex(Point, false));
		}
		TriangulatePoly(Section.ProcIndexBuffer, Section.ProcVertexBuffer, VertIndices, -GetVec3(AVN_TangentZ));
	}
	if (HasInt(AVN_Collision))
	{
		Section.bEnableCollision = (bool)GetInt(AVN_Collision);
	}
	if (HasInt(AVN_Visible))
	{
		Section.bSectionVisible = (bool)GetInt(AVN_Visible);
	}
#if WITH_EDITORONLY_DATA
	NumTriangles = (Section.ProcIndexBuffer.Num() - StartIndex) / 3;
#endif
}
void FArteriesPrimitive::DeleteAllLinks()
{
	for (int i = 0; i < NumSegments(); i++)
	{
		FArteriesPoint* Start = GetPoint(i);
		FArteriesPoint* End = NextPoint(i);
		Start->DeleteLinks(End, this, false);
		End->DeleteLinks(Start, this, true);
	}
}
bool FArteriesPrimitive::HasAnyValidLinks()
{
	for (int i = 0; i < NumSegments(); i++)
	{
		FArteriesPoint* Start = GetPoint(i);
		FArteriesPoint* End = NextPoint(i);
		if (Start->GetLinks(End, this, -1).Num())
			return true;
		if (End->GetLinks(Start, this, -1).Num())
			return true;
	}
	return false;
}
void FArteriesPrimitive::GetOrientedBox(FMatrix& OutMatrix, FVector& OutSize) const
{
	((FArteriesPrimitive*)this)->CalculateTangents();
	FVector TangentZ = GetVec3(AVN_TangentZ);
	float MinArea = MAX_FLT;
	for (int i = 0; i < NumSegments(); i++)
	{
		FVector& Start = GetPoint(i)->Position;
		FVector& End = NextPoint(i)->Position;
		FVector TangentX = (End - Start).GetSafeNormal();
		FVector TangentY = (TangentZ ^ TangentX).GetSafeNormal();
		FMatrix L2W = FMatrix(TangentX, TangentY, TangentZ, Start);
		FMatrix W2L = L2W.Inverse();
		FBox Box(EForceInit::ForceInit);
		for (int j = 0; j < NumPoints(); j++)
			Box += W2L.TransformPosition(Points[j]->Position);
		FVector Center = Box.GetCenter();
		FVector Size = Box.GetSize();
		float Area = Size.X * Size.Y;
		if (MinArea > Area)
		{
			MinArea = Area;
			OutSize = Size;
			OutMatrix = FMatrix(TangentX, TangentY, TangentZ, L2W.TransformPosition(Center));
		}
	}
}
float FArteriesPrimitive::CalculateArea()const
{
	if (!IsClosed() || Points.Num() < 3)
		return 0;
	FVector Side1, Side2;
	float Area = 0;
	Side1 = Points[1]->Position - Points[0]->Position;
	for (int i = 2; i < Points.Num(); i++)
	{
		Side2 = Points[i]->Position - Points[0]->Position;
		Area += (Side1 ^ Side2).Size() * 0.5f;
		Side1 = Side2;
	}
	return Area;
}
FVector FArteriesPrimitive::Centroid()
{
	if (!HasVec3(AVN_TangentZ))
		CalculateNormal();
	FVector TangentZ = GetVec3(AVN_TangentZ);
	FVector Centroid(0, 0, 0);
	float Area = 0;
	for (int i = 0; i < Points.Num(); i++)
	{
		FVector v0 = Points[i]->Position;
		FVector v1 = Points[(i + 1) % Points.Num()]->Position;
		FVector Cross = v0 ^ v1;
		float A = (Cross | TangentZ);
		Area += A;
		Centroid += (v0 + v1)*A;
	}
	Area *= 0.5f;
	Centroid /= (6.f * Area);
	return Centroid;
}
int FArteriesPrimitive::LongestSegment() const
{
	float MaxDist = 0;
	int MaxIndex = INDEX_NONE;
	for (int i = 0; i < NumSegments(); i++)
	{
		FArteriesPoint* This = GetPoint(i);
		FArteriesPoint* Next = NextPoint(i);
		float DistSq = (Next->Position - This->Position).SizeSquared();
		if (MaxDist < DistSq)
		{
			MaxDist = DistSq;
			MaxIndex = i;
		}
	}
	return MaxIndex;
}
void FArteriesPrimitive::Clip(FArteriesPointMapper& Mapper, const FPlane& Plane, TArray<FArteriesPoint*>& PPrim, TArray<FArteriesPoint*>& NPrim)
{
	//暂时没考虑凹多边形和切面重合的情况！
	TArray<float> Dots;
	for (FArteriesPoint* P : Points)
		Dots.Add(Plane.PlaneDot(P->Position));
	for (int i = 0; i < Points.Num(); i++)
	{
		FArteriesPoint* P0 = Points[i];
		float Dot0 = Dots[i];
		if (Dot0 > 0)
			PPrim.Add(P0);
		else
			NPrim.Add(P0);
		float Dot1 = Dots[(i + 1) % Dots.Num()];
		if (Dot0 * Dot1 < 0)
		{
			FArteriesPoint* P1 = Points[(i + 1) % Points.Num()];
			float Alpha = Dot0 / (Dot0 - Dot1);
			FArteriesPoint* NewP = Mapper.GetPointLerp(P0, P1, Alpha);
			PPrim.Add(NewP);
			NPrim.Add(NewP);
		}
	}
}
void FArteriesPrimitive::Reverse()
{
	for (int i = 0; i < NumSegments(); i++)
	{
		FArteriesPoint* This = GetPoint(i);
		FArteriesPoint* Next = NextPoint(i);
		This->GetLinks(Next, this, 0)[0]->Inverse = true;
		Next->GetLinks(This, this, 1)[0]->Inverse = false;
	}
	Algo::Reverse(Points);
}
void FArteriesPrimitive::Serialize(FArchive& Ar, UArteriesObject* Object, TMap<FArteriesPoint*, int>& PointIDs, TMap<FArteriesPrimitive*, int>& PrimitiveIDs)
{
	if (Ar.IsSaving() || Ar.IsObjectReferenceCollector())
	{
		if (int* OuterID = PrimitiveIDs.Find(Outer))
			SetInt(AVN_OuterID, *OuterID);
		else
			DelInt(AVN_OuterID);
	}
	if (Ar.IsLoading() || Ar.IsSaving() || Ar.IsCountingMemory())
	{
		UStruct* Struct = FArteriesPrimitive::StaticStruct();
		Struct->SerializeTaggedProperties(Ar, (uint8*)this, Struct, NULL);
	}
	SerializeArray(Ar, (TArray<FArteriesElement*>&)Points, (TIndirectArray<FArteriesElement>&)Object->Points, (TMap<FArteriesElement*,int>&)PointIDs);
	if (Ar.IsLoading())
	{
		if (HasInt(AVN_OuterID))
			Outer = &Object->Primitives[GetInt(AVN_OuterID)];
	}
}
//NumSegments()
TArray<FVector> FArteriesPrimitive::GetDirections() const
{
	TArray<FVector> Directions;
	for (int i = 0; i < NumSegments(); i++)
	{
		FArteriesPoint* Start = GetPoint(i);
		FArteriesPoint* End = NextPoint(i);
		Directions.Add((End->Position - Start->Position).GetSafeNormal());
	}
	return MoveTemp(Directions);
}
//NumPoints()
TArray<FMatrix> FArteriesPrimitive::GetTransforms() const
{
	TArray<FMatrix> Transforms;
	TArray<FVector> Directions = GetDirections();
	for (int i = 0; i < NumPoints(); i++)
	{
		FArteriesPoint* Point = Points[i];
		FVector PrevDirection = (i == 0) ? (bClosed ? Directions.Last() : Directions[0]) : Directions[i - 1];
		FVector NextDirection = (i == Directions.Num()) ? Directions[i - 1] : Directions[i];
		FVector Direction = (PrevDirection + NextDirection).GetSafeNormal();
		Transforms.Add(GetTransform(Point->Position, Direction, 1.f / (Direction | NextDirection)));
	}
	return MoveTemp(Transforms);
}
/*
TArray<FTransform> FArteriesPrimitive::GetTargets()
{
	bool bSmooth = false;
	TArray<FTransform> Transforms;
	TArray<FVector> Directions = GetDirections();
	bool bClosed = IsClosed();
	int NumTransforms = Points.Num() - bClosed;
	CalculateTangents();
	FVector Z = GetVec3(AVN_TangentZ);
	for (int i = 0; i < NumTransforms; i++)
	{
		FArteriesPoint* Point = Points[i];
		FVector PrevDirection, NextDirection;
		if (Directions.Num())
		{
			PrevDirection = (i == 0) ? (bClosed ? Directions.Last() : Directions[0]) : Directions[i - 1];
			NextDirection = (i == Points.Num() - 1) ? Directions[i - 1] : Directions[i];
		}
		else
			PrevDirection = NextDirection = GetVec3(AVN_TangentX);
		FVector X = bSmooth ? (PrevDirection + NextDirection).GetSafeNormal() : NextDirection;
		FVector Y = (Z ^ X).GetSafeNormal();
		Transforms.Add(FTransform(X, Y, Z, Point->Position));
	}
	return MoveTemp(Transforms);
}*/
//NumSegments() + 1
TArray<float> FArteriesPrimitive::GetDists(TArray<FVector>* OutDirections) const
{
	TArray<float> Dists;
	float Dist = 0;
	Dists.Add(Dist);
	for (int i = 0; i < NumSegments(); i++)
	{
		FArteriesPoint* Start = GetPoint(i);
		FArteriesPoint* End = NextPoint(i);
		FVector Direction = End->Position - Start->Position;
		float Size = Direction.Size();
		Dist += Size;
		if (OutDirections)
			OutDirections->Add(Direction / Size);
		Dists.Add(Dist);
	}
	return MoveTemp(Dists);
}
float DistanceToLine(const FVector2D& LineStart, const FVector2D& LineDirection, const FVector2D& Point)
{
	FVector2D PointToStart = Point - LineStart;
	FVector2D Normal(LineDirection.Y, -LineDirection.X);
	return Normal | PointToStart;
}
bool RayIntersectRay(const FVector2D& Segment1Start, const FVector2D& Segment1Dir, const FVector2D& Segment2Start, const FVector2D& Segment2Dir, float* T1 = NULL, float* T2 = NULL)
{
	const float Determinant = FVector2D::CrossProduct(Segment1Dir, Segment2Dir);
	if (!FMath::IsNearlyZero(Determinant, KINDA_SMALL_NUMBER))
	{
		const FVector2D SegmentStartDelta = Segment2Start - Segment1Start;
		const float OneOverDet = 1.0f / Determinant;
		const float Seg1Intersection = FVector2D::CrossProduct(SegmentStartDelta, Segment2Dir) * OneOverDet;
		const float Seg2Intersection = FVector2D::CrossProduct(SegmentStartDelta, Segment1Dir) * OneOverDet;
	//	const float Epsilon = 1 / 65536.0f;
		if (T1) *T1 = Seg1Intersection;
		if (T2) *T2 = Seg2Intersection;
		return (Seg1Intersection > 0 && Seg2Intersection > 0);
	}
	return false;
}
//http://blog.csdn.net/chaosllgao/article/details/6938294
bool IsPointInside(const TArray<FVector>& Points, const FVector2D& pt)
{
	bool inside = false;
	for (int i = 0; i < Points.Num(); i++)
	{
		const FVector& polyI = Points[i];
		const FVector& polyJ = Points[(i+1)%Points.Num()];
		if ((polyI.Y < pt.Y && polyJ.Y >= pt.Y) || (polyJ.Y < pt.Y && polyI.Y >= pt.Y))
		{
			//统计左边的交点个数
			if ((pt.Y - polyI.Y)*(polyJ.X - polyI.X) / (polyJ.Y - polyI.Y) + polyI.X < pt.X)
				inside = !inside;
		}
	}
	return inside;
}
int Intersect(const TArray<FVector>& Points, const FVector2D& P0, const FVector2D& P1, float& MinT)
{
	int Index = INDEX_NONE;
	MinT = MAX_FLT;
	for (int i = 0; i < Points.Num(); i++)
	{
		FVector2D& Start = (FVector2D&)Points[i];
		FVector2D& End = (FVector2D&)Points[(i+1)%Points.Num()];
		float T;
		if (DoLineSegmentsIntersect(P0, P1, Start, End, &T))
		{
			if (MinT > T)
			{
				MinT = T;
				Index = i;
			}
		}
	}
	return Index;
}