// Copyright 2020 PacosLelouch, Inc. All Rights Reserved.
// https://github.com/PacosLelouch/

#include "MeshParametrization.h"
#include "MeshDenoising.h"
#include "../Query/MeshQuery.h"
#include "ArteriesObject.h"
#include "ProceduralMeshComponent.h"
#include "KismetProceduralMeshLibrary.h"
#include "Materials/MaterialInterface.h"
#include "Engine/StaticMesh.h"
#include "PhysicsEngine/BodySetup.h"

void UArteriesMeshParametrization::FloaterParametrizationInPlace(UArteriesObject* Obj)
{
	if (!IsValid(Obj))
	{
		return;
	}

	//UArteriesObject* CopiedObj = Cast<UArteriesObject>(StaticDuplicateObject(Obj, Obj->GetOuter()));
	//UArteriesMeshDenoising::MinimalSurfaceGlobalHardInPlace(CopiedObj);
	//MapUV(CopiedObj);
	//for (int32 i = 0; i < CopiedObj->Points.Num(); ++i)
	//{
	//	Obj->Points[i].SetVec2(AVN_UV0, CopiedObj->Points[i].GetVec2(AVN_UV0));
	//}
	TSet<FArteriesPoint*> BoundaryPoints;
	UArteriesMeshQuery::FindBoundary(BoundaryPoints, Obj);
	MapUV(Obj, BoundaryPoints);
	UArteriesMeshDenoising::MinimalSurfaceValueGlobalHardInPlace(Obj, BoundaryPoints, 2, AVN_UV0);
}

void UArteriesMeshParametrization::MapUV(UArteriesObject* Obj, const TSet<FArteriesPoint*>& BoundaryPoints)
{
	if (!IsValid(Obj))
	{
		return;
	}

	TArray<float> DistanceNext;
	TArray<FArteriesPoint*> SortedBoundary;
	TSet<FArteriesPoint*> VisitedPoints;
	DistanceNext.Reserve(BoundaryPoints.Num() + 1);
	DistanceNext.Add(0.f);
	SortedBoundary.Reserve(BoundaryPoints.Num());
	VisitedPoints.Reserve(BoundaryPoints.Num());
	for (FArteriesPoint* StartPointToVisit : BoundaryPoints)
	{
		if (VisitedPoints.Contains(StartPointToVisit))
		{
			continue;
		}

		TQueue<FArteriesPoint*> BFS;
		BFS.Enqueue(StartPointToVisit);
		SortedBoundary.Add(StartPointToVisit);
		VisitedPoints.Add(StartPointToVisit);
		if (SortedBoundary.Num() > 1)
		{
			DistanceNext.Add(DistanceNext.Last() + FVector::Dist(SortedBoundary.Last(0)->Position, SortedBoundary.Last(1)->Position));
		}

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
				SortedBoundary.Add(NextPoint);
				VisitedPoints.Add(NextPoint);
				if (SortedBoundary.Num() > 1)
				{
					DistanceNext.Add(DistanceNext.Last() + FVector::Dist(SortedBoundary.Last(0)->Position, SortedBoundary.Last(1)->Position));
				}
				break;
			}
		}
	}

	if (SortedBoundary.Num() < 3)
	{
		return;
	}

	DistanceNext.Add(DistanceNext.Last() + FVector::Dist(SortedBoundary[0]->Position, SortedBoundary.Last(0)->Position));

	TArray<int32> TurnIndex;
	if (SortedBoundary.Num() <= 4)
	{
		for (int32 i = 0; i < SortedBoundary.Num(); ++i)
		{
			TurnIndex.Add(i);
		}
		while (TurnIndex.Num() < 4)
		{
			TurnIndex.Add(SortedBoundary.Num() - 1);
		}
	}
	else
	{
		TurnIndex.Add(0);

		for (int32 i = 0; i < SortedBoundary.Num(); ++i)
		{
			if (DistanceNext[i] >= DistanceNext.Last() * 0.25f * TurnIndex.Num() && TurnIndex.Num() < 4)
			{
				TurnIndex.Add(i);
			}
		}
	}
	SortedBoundary[TurnIndex[0]]->SetVec2(AVN_UV0, FVector2D(0.f, 0.f));
	SortedBoundary[TurnIndex[1]]->SetVec2(AVN_UV0, FVector2D(1.f, 0.f));
	SortedBoundary[TurnIndex[2]]->SetVec2(AVN_UV0, FVector2D(1.f, 1.f));
	SortedBoundary[TurnIndex[3]]->SetVec2(AVN_UV0, FVector2D(0.f, 1.f));

	for (int32 i = 0; i < TurnIndex.Num() - 1; ++i)
	{
		for (int32 j = TurnIndex[i] + 1; j < TurnIndex[i + 1]; ++j)
		{
			float SDist = DistanceNext[TurnIndex[i]];
			float EDist = DistanceNext[TurnIndex[i + 1]];
			float CDist = DistanceNext[j];
			FVector2D SUV0 = SortedBoundary[TurnIndex[i]]->GetVec2(AVN_UV0);
			FVector2D EUV0 = SortedBoundary[TurnIndex[i + 1]]->GetVec2(AVN_UV0);
			SortedBoundary[j]->SetVec2(AVN_UV0, FMath::Lerp(SUV0, EUV0, FMath::IsNearlyZero(EDist - SDist) ? 0.5f : (CDist - SDist) / (EDist - SDist)));
		}
	}
	for (int32 j = TurnIndex.Last() + 1; j < SortedBoundary.Num(); ++j)
	{
		float SDist = DistanceNext[TurnIndex.Last()];
		float EDist = DistanceNext.Last();
		float CDist = DistanceNext[j];
		FVector2D SUV0 = SortedBoundary[TurnIndex.Last()]->GetVec2(AVN_UV0);
		FVector2D EUV0 = SortedBoundary[0]->GetVec2(AVN_UV0);
		SortedBoundary[j]->SetVec2(AVN_UV0, FMath::Lerp(SUV0, EUV0, FMath::IsNearlyZero(EDist - SDist) ? 0.5f : (CDist - SDist) / (EDist - SDist)));
	}
}
