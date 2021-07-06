// Copyright 2020 PacosLelouch, Inc. All Rights Reserved.
// https://github.com/PacosLelouch/

#include "MeshTessellation.h"
#include "MeshParametrization.h"
#include "MeshDenoising.h"
#include "../Query/MeshQuery.h"
#include "ArteriesObject.h"
#include "ArteriesUtil.h"
#include "ProceduralMeshComponent.h"
#include "KismetProceduralMeshLibrary.h"
#include "Materials/MaterialInterface.h"
#include "Engine/StaticMesh.h"
#include "PhysicsEngine/BodySetup.h"
#include "Developer/Profiler/Public/ProfilerCommon.h"
#define JC_VORONOI_IMPLEMENTATION
#include "Voronoi/jc_voronoi.h"

static TArray<jcv_point> Scatter_Impl(const TArray<FVector>& Points, const jcv_rect& bounding_box, int32 Seed, int32 Count, float Density, int32 Iterations)
{
	TArray<float> Areas;
	TArray<int> Triangles;
	float TotalArea = 0;
	int32 v0 = 0;
	for (int32 v1 = 1; v1 < Points.Num() - 1; v1++)
	{
		int32 v2 = v1 + 1;
		Triangles.Add(v0);
		Triangles.Add(v1);
		Triangles.Add(v2);
		TotalArea += GetTriangleArea(Points[v0], Points[v1], Points[v2]);
		Areas.Add(TotalArea);
	}
	TArray<jcv_point> Sites;
	if (Count == 0 && Density >= 1.f) Count = FMath::RoundToInt(TotalArea / Density);
	for (int32 i = 0; i < Count; i++)
	{
		float Area = FMath::FRand() * TotalArea;
		const int32 Triangle = FBinaryFindIndex::GreaterEqual(Areas, Area) * 3;
		FVector Position = RandomPointInTriangle(Points[Triangles[Triangle]], Points[Triangles[Triangle + 1]], Points[Triangles[Triangle + 2]]);
		jcv_point& point = Sites[Sites.AddUninitialized()];
		point.x = Position.X;
		point.y = Position.Y;
	}

	for (int32 i = 0; i < Iterations; i++)
	{
		jcv_diagram diagram;
		memset(&diagram, 0, sizeof(jcv_diagram));
		jcv_diagram_generate(Sites.Num(), Sites.GetData(), &bounding_box, &diagram);
		const jcv_site* sites = jcv_diagram_get_sites(&diagram);
		for (int32 j = 0; j < diagram.numsites; j++)
		{
			TArray<FVector2D> Polygon;
			jcv_graphedge* graph_edge = sites[j].edges;
			while (graph_edge)
			{
				Polygon.Add(FVector2D(graph_edge->pos[0].x, graph_edge->pos[0].y));
				graph_edge = graph_edge->next;
			}
			jcv_point& point = Sites[j];
			FVector2D Centroid = CalculateCentroid(Polygon);
			point.x = Centroid.X;
			point.y = Centroid.Y;
		}
		jcv_diagram_free(&diagram);
	}
	return MoveTemp(Sites);
}

void UArteriesMeshTessellation::CentroidalVoronoiTessellationInPlaceForEachPrimitive(UArteriesObject* Obj, int32 Seed, int32 Count, float Density, int32 Iterations)
{
	UArteriesObject* TempObj = Obj->Copy_Impl();
	Obj->ClearAll();
	FMath::RandInit(Seed);
	FArteriesPointMapper Mapper(Obj, "", 0.01f);
	for (FArteriesPrimitive& PrimitiveRef : TempObj->Primitives)
	{
		FArteriesPrimitive* Primitive = &PrimitiveRef;
		if (Primitive->IsClosed())
		{
			((FArteriesPrimitive*)Primitive)->CalculateTangents();
			FMatrix LocalToWorld = Primitive->GetLocalToWorldTransform();
			FMatrix WorldToLocal = LocalToWorld.Inverse();

			TArray<FVector> Pts;
			FBox Box(EForceInit::ForceInit);
			for (FArteriesPoint* Point : Primitive->Points)
			{
				FVector LocalPosition = WorldToLocal.TransformPosition(Point->Position);
				Pts.Add(LocalPosition);
				Box += LocalPosition;
			}
			jcv_rect bounding_box = { { Box.Min.X, Box.Min.Y }, { Box.Max.X, Box.Max.Y } };

			TArray<jcv_point> Sites = Scatter_Impl(Pts, bounding_box, Seed, Count, Density, Iterations);
			jcv_diagram diagram;
			memset(&diagram, 0, sizeof(jcv_diagram));
			jcv_diagram_generate(Sites.Num(), Sites.GetData(), &bounding_box, &diagram);
			const jcv_site* sites = jcv_diagram_get_sites(&diagram);
			for (int32 i = 0; i < diagram.numsites; i++)
			{
				FArteriesPrimitive* Prim = Obj->AddPrimitive();
				jcv_graphedge* graph_edge = sites[i].edges;
				int32 Intersects[2] = { INDEX_NONE, INDEX_NONE };
				FVector2D Pt0(graph_edge->pos[0].x, graph_edge->pos[0].y);
				bool bInside0 = IsPointInside(Pts, Pt0);
				while (graph_edge)
				{
					if (bInside0)
						Prim->Add(Mapper.GetPoint(LocalToWorld.TransformPosition(FVector(Pt0, 0))));
					FVector2D Pt1(graph_edge->pos[1].x, graph_edge->pos[1].y);
					bool bInside1 = IsPointInside(Pts, Pt1);
					if (bInside0 != bInside1)
					{
						float T;
						Intersects[bInside1] = Intersect(Pts, Pt0, Pt1, T);
						if (bInside0)
							Prim->Add(Mapper.GetPoint(LocalToWorld.TransformPosition(FVector(FMath::Lerp(Pt0, Pt1, T), 0))));
						if (Intersects[!bInside1] != INDEX_NONE)
						{
							for (int32 j = Intersects[0]; j != Intersects[1];)
							{
								int32 NextIndex = (j + 1) % Primitive->NumPoints();
								Prim->Add(Mapper.GetPoint(Primitive->GetPoint(NextIndex)->Position));
								j = NextIndex;
							}
						}
						if (bInside1)
							Prim->Add(Mapper.GetPoint(LocalToWorld.TransformPosition(FVector(FMath::Lerp(Pt0, Pt1, T), 0))));
					}
					if (bInside1)
						Prim->Add(Mapper.GetPoint(LocalToWorld.TransformPosition(FVector(Pt1, 0))));
					Pt0 = Pt1;
					bInside0 = bInside1;
					graph_edge = graph_edge->next;
				}
				if (!Prim->Points.Num())
					Obj->DeletePrimitive(Prim);
				else
					Prim->MakeClose();
			}
			jcv_diagram_free(&diagram);
		}
	}
}

UArteriesObject* UArteriesMeshTessellation::CentroidalVoronoiTessellation(const TArray<FVector>& SortedBoundaryPoints, int32 Seed, int32 Count, float Density, int32 Iterations)
{
	UArteriesObject* Obj = UArteriesObject::New_Impl();
	FMath::RandInit(Seed);
	FArteriesPointMapper Mapper(Obj, "", 0.01f);
	{
		{
			FBox Box(EForceInit::ForceInit);
			for (const FVector& Vec : SortedBoundaryPoints)
			{
				Box += Vec;
			}
			jcv_rect bounding_box = { { Box.Min.X, Box.Min.Y }, { Box.Max.X, Box.Max.Y } };

			TArray<jcv_point> Sites = Scatter_Impl(SortedBoundaryPoints, bounding_box, Seed, Count, Density, Iterations);
			jcv_diagram diagram;
			memset(&diagram, 0, sizeof(jcv_diagram));
			jcv_diagram_generate(Sites.Num(), Sites.GetData(), &bounding_box, &diagram);
			const jcv_site* sites = jcv_diagram_get_sites(&diagram);
			for (int32 i = 0; i < diagram.numsites; i++)
			{
				FArteriesPrimitive* Prim = Obj->AddPrimitive();
				jcv_graphedge* graph_edge = sites[i].edges;
				int32 Intersects[2] = { INDEX_NONE, INDEX_NONE };
				FVector2D Pt0(graph_edge->pos[0].x, graph_edge->pos[0].y);
				bool bInside0 = IsPointInside(SortedBoundaryPoints, Pt0);
				while (graph_edge)
				{
					if (bInside0)
						Prim->Add(Mapper.GetPoint(FVector(Pt0, 0)));
					FVector2D Pt1(graph_edge->pos[1].x, graph_edge->pos[1].y);
					bool bInside1 = IsPointInside(SortedBoundaryPoints, Pt1);
					if (bInside0 != bInside1)
					{
						float T;
						Intersects[bInside1] = Intersect(SortedBoundaryPoints, Pt0, Pt1, T);
						if (bInside0)
							Prim->Add(Mapper.GetPoint(FVector(FMath::Lerp(Pt0, Pt1, T), 0)));
						if (Intersects[!bInside1] != INDEX_NONE)
						{
							for (int32 j = Intersects[0]; j != Intersects[1];)
							{
								int32 NextIndex = (j + 1) % SortedBoundaryPoints.Num();
								Prim->Add(Mapper.GetPoint(SortedBoundaryPoints[NextIndex]));
								j = NextIndex;
							}
						}
						if (bInside1)
							Prim->Add(Mapper.GetPoint(FVector(FMath::Lerp(Pt0, Pt1, T), 0)));
					}
					if (bInside1)
						Prim->Add(Mapper.GetPoint(FVector(Pt1, 0)));
					Pt0 = Pt1;
					bInside0 = bInside1;
					graph_edge = graph_edge->next;
				}
				if (!Prim->Points.Num())
					Obj->DeletePrimitive(Prim);
				else
					Prim->MakeClose();
			}
			jcv_diagram_free(&diagram);
		}
	}
	return Obj;
}

void UArteriesMeshTessellation::CentroidalVoronoiTessellationInPlaceByBoundary(UArteriesObject* Obj, const TArray<FArteriesPoint*>& SortedBoundaryPoints, int32 Seed, int32 Count, float Density, int32 Iterations)
{
	//UArteriesObject* TempObj = Obj->Copy_Impl();
	FMath::RandInit(Seed);

	TArray<FVector> Pts;
	FBox Box(EForceInit::ForceInit);

	FArteriesPrimitive* Primitive = Obj->GetPrimitive(0);
	Primitive->CalculateTangents();
	FMatrix LocalToWorld = Primitive->GetLocalToWorldTransform();
	FMatrix WorldToLocal = LocalToWorld.Inverse();

	for (FArteriesPoint* Point : SortedBoundaryPoints)
	{
		FVector LocalPosition = WorldToLocal.TransformPosition(Point->Position);
		Pts.Add(LocalPosition);
		Box += LocalPosition;
	}

	Obj->ClearAll();
	FArteriesPointMapper Mapper(Obj, "", 0.01f);

	jcv_rect bounding_box = { { Box.Min.X, Box.Min.Y }, { Box.Max.X, Box.Max.Y } };

	TArray<jcv_point> Sites = Scatter_Impl(Pts, bounding_box, Seed, Count, Density, Iterations);
	jcv_diagram diagram;
	memset(&diagram, 0, sizeof(jcv_diagram));
	jcv_diagram_generate(Sites.Num(), Sites.GetData(), &bounding_box, &diagram);
	const jcv_site* sites = jcv_diagram_get_sites(&diagram);
	for (int32 i = 0; i < diagram.numsites; i++)
	{
		FArteriesPrimitive* Prim = Obj->AddPrimitive();
		jcv_graphedge* graph_edge = sites[i].edges;
		int32 Intersects[2] = { INDEX_NONE, INDEX_NONE };
		FVector2D Pt0(graph_edge->pos[0].x, graph_edge->pos[0].y);
		bool bInside0 = IsPointInside(Pts, Pt0);
		while (graph_edge)
		{
			if (bInside0)
				Prim->Add(Mapper.GetPoint(LocalToWorld.TransformPosition(FVector(Pt0, 0))));
			FVector2D Pt1(graph_edge->pos[1].x, graph_edge->pos[1].y);
			bool bInside1 = IsPointInside(Pts, Pt1);
			if (bInside0 != bInside1)
			{
				float T;
				Intersects[bInside1] = Intersect(Pts, Pt0, Pt1, T);
				if (bInside0)
					Prim->Add(Mapper.GetPoint(LocalToWorld.TransformPosition(FVector(FMath::Lerp(Pt0, Pt1, T), 0))));
				if (Intersects[!bInside1] != INDEX_NONE)
				{
					for (int32 j = Intersects[0]; j != Intersects[1];)
					{
						int32 NextIndex = (j + 1) % Pts.Num();
						Prim->Add(Mapper.GetPoint(LocalToWorld.TransformPosition(Pts[NextIndex])));
						j = NextIndex;
					}
				}
				if (bInside1)
					Prim->Add(Mapper.GetPoint(LocalToWorld.TransformPosition(FVector(FMath::Lerp(Pt0, Pt1, T), 0))));
			}
			if (bInside1)
				Prim->Add(Mapper.GetPoint(LocalToWorld.TransformPosition(FVector(Pt1, 0))));
			Pt0 = Pt1;
			bInside0 = bInside1;
			graph_edge = graph_edge->next;
		}
		if (!Prim->Points.Num())
			Obj->DeletePrimitive(Prim);
		else
			Prim->MakeClose();
	}
	jcv_diagram_free(&diagram);
}

void UArteriesMeshTessellation::GetSortedBoundary(TArray<FArteriesPoint*>& SortedBoundaryPoints, const TSet<FArteriesPoint*>& BoundaryPoints)
{
	TSet<FArteriesPoint*> VisitedPoints;
	SortedBoundaryPoints.Empty(BoundaryPoints.Num());
	VisitedPoints.Reserve(BoundaryPoints.Num());
	for (FArteriesPoint* StartPointToVisit : BoundaryPoints)
	{
		if (VisitedPoints.Contains(StartPointToVisit))
		{
			continue;
		}

		TQueue<FArteriesPoint*> BFS;
		BFS.Enqueue(StartPointToVisit);
		SortedBoundaryPoints.Add(StartPointToVisit);
		VisitedPoints.Add(StartPointToVisit);
		//if (SortedBoundary.Num() > 1)
		//{
		//	DistanceNext.Add(DistanceNext.Last() + FVector::Dist(SortedBoundary.Last(0)->Position, SortedBoundary.Last(1)->Position));
		//}

		FArteriesPoint* CurPoint = nullptr;
		while (BFS.Dequeue(CurPoint))
		{
			for (FArteriesLink& Link : CurPoint->Links)
			{
				if (Link.Target == CurPoint || VisitedPoints.Contains(Link.Target) || !BoundaryPoints.Contains(Link.Target))
				{
					continue;
				}

				FArteriesPoint* NextPoint = Link.Target;
				BFS.Enqueue(NextPoint);
				SortedBoundaryPoints.Add(NextPoint);
				VisitedPoints.Add(NextPoint);
				//if (SortedBoundary.Num() > 1)
				//{
				//	DistanceNext.Add(DistanceNext.Last() + FVector::Dist(SortedBoundary.Last(0)->Position, SortedBoundary.Last(1)->Position));
				//}
				break;
			}
		}
	}
}
