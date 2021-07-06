// Copyright 2020 PacosLelouch, Inc. All Rights Reserved.
// https://github.com/PacosLelouch/

#include "MeshQuery.h"
#include "ArteriesObject.h"
#include "ProceduralMeshComponent.h"
#include "KismetProceduralMeshLibrary.h"
#include "Materials/MaterialInterface.h"
#include "Engine/StaticMesh.h"
#include "PhysicsEngine/BodySetup.h"


void UArteriesMeshQuery::FindBoundary(TSet<FArteriesPoint*>& OutBoundaryPoints, UArteriesObject* Obj)
{
	//TMap<TPair<int32, int32>, int32> EdgeCount;
	TMap<TPair<FArteriesPoint*, FArteriesPoint*>, int32> EdgeCount;
	//for (int32 i = 0; i < InIndexBuffer.Num(); i += 3) {
	for (int32 i = 0; i < Obj->Primitives.Num(); ++i) {
		const FArteriesPrimitive& Prim = Obj->Primitives[i];
		for (int32 j = 0; j < Prim.Points.Num(); ++j) {
			FArteriesPoint* P0 = Prim.Points[j];
			FArteriesPoint* P1 = Prim.Points[j + 1 == Prim.Points.Num() ? 0 : j + 1];
			//int32 I0 = InIndexBuffer[i + j];
			//int32 I1 = InIndexBuffer[i + (j + 1 >= 3 ? j - 2 : j + 1)]; // Positive order
			////int32 I1 = InSection.ProcIndexBuffer[i + (j + 2 >= 3 ? j - 1 : j + 2)]; // Negative order
			//TPair<int32, int32> I0I1 = MakeTuple(I0, I1), I1I0 = MakeTuple(I1, I0);
			TPair<FArteriesPoint*, FArteriesPoint*> P0P1 = MakeTuple(P0, P1), P1P0 = MakeTuple(P1, P0);
			//int32* Count0Ptr = EdgeCount.Find(I0I1);
			//int32* Count1Ptr = EdgeCount.Find(I1I0);
			int32* Count0Ptr = EdgeCount.Find(P0P1);
			int32* Count1Ptr = EdgeCount.Find(P1P0);

			if (Count0Ptr) {
				++(*Count0Ptr);
			}
			else if (Count1Ptr) {
				++(*Count1Ptr);
			}
			else {
				EdgeCount.Add(P0P1, 1);
			}
		}
	}

	//TMap<int32, TArray<int32> > IndexFromTo;
	TMap<FArteriesPoint*, TArray<FArteriesPoint*> > PointFromTo;
	for (const TPair<TPair<FArteriesPoint*, FArteriesPoint*>, int32>& Pair : EdgeCount) {
		if (Pair.Value == 1) {
			//IndexFromTo.FindOrAdd(Pair.Key.Key).Add(Pair.Key.Value);
			PointFromTo.FindOrAdd(Pair.Key.Key).Add(Pair.Key.Value);
			//OutContourVertexIndices.Add(Pair.Key.Key);
			OutBoundaryPoints.Add(Pair.Key.Key);
		}
	}

	//while (IndexFromTo.Num()) {
	//	TArray<int32>* IndexClusterPtr = nullptr;
	//	if (OutIndexClusters.Num() == 0 || OutIndexClusters.Last().Num() > 0) {
	//		IndexClusterPtr = &OutIndexClusters.AddDefaulted_GetRef();
	//	}
	//	else {
	//		IndexClusterPtr = &OutIndexClusters.Last();
	//	}
	//	TArray<int32>& IndexCluster = *IndexClusterPtr;
	//	TPair<int32, TArray<int32> > Pair = *IndexFromTo.begin();
	//	int32 Index = Pair.Key;
	//	TArray<int32>* NextArrayPtr = &Pair.Value;
	//	while (NextArrayPtr && NextArrayPtr->Num() > 0) {
	//		IndexCluster.Add(Index);
	//		int32 PrevIndex = Index;
	//		//Index = IndexFromTo[Index];
	//		//IndexFromTo.Remove(PrevIndex);
	//		Index = NextArrayPtr->Pop();
	//		if (NextArrayPtr->Num() == 0) {
	//			IndexFromTo.Remove(PrevIndex);
	//		}
	//		NextArrayPtr = IndexFromTo.Find(Index);
	//	}
	//}
	//if (OutIndexClusters.Num() > 0 && OutIndexClusters.Last().Num() == 0) {
	//	OutIndexClusters.Pop();
	//}
}

void UArteriesMeshQuery::FindBoundary(TSet<int32>& OutBoundaryVertices, FProcMeshSection* SectionPtr)
{
	TMap<TPair<int32, int32>, int32> BoundaryEdgesCount;
	for (int32 i = 0; i < SectionPtr->ProcIndexBuffer.Num(); i += 3)
	{
		int32 Idx0 = SectionPtr->ProcIndexBuffer[i];
		int32 Idx1 = SectionPtr->ProcIndexBuffer[i + 1];
		int32 Idx2 = SectionPtr->ProcIndexBuffer[i + 2];
		TArray<int32> Idxs{ Idx0, Idx1, Idx2 };
		Idxs.Sort();
		TPair<int32, int32> Edge = MakeTuple(Idxs[0], Idxs[1]);
		++BoundaryEdgesCount.FindOrAdd(Edge, 0);
		Edge = MakeTuple(Idxs[0], Idxs[2]);
		++BoundaryEdgesCount.FindOrAdd(Edge, 0);
		Edge = MakeTuple(Idxs[1], Idxs[2]);
		++BoundaryEdgesCount.FindOrAdd(Edge, 0);
	}
	OutBoundaryVertices.Empty();
	for (auto EdgeCountPair : BoundaryEdgesCount)
	{
		if (EdgeCountPair.Value == 1)
		{
			OutBoundaryVertices.Add(EdgeCountPair.Key.Key);
			OutBoundaryVertices.Add(EdgeCountPair.Key.Value);
		}
	}
}
