// Copyright 2020 PacosLelouch, Inc. All Rights Reserved.
// https://github.com/PacosLelouch/

#include "MeshSimplification.h"
#include "../Query/MeshQuery.h"
#include "ArteriesObject.h"
#include "ProceduralMeshComponent.h"
#include "KismetProceduralMeshLibrary.h"
#include "Materials/MaterialInterface.h"
#include "Engine/StaticMesh.h"
#include "PhysicsEngine/BodySetup.h"
#include "Eigen/Dense"

template<typename T = FVector>
void GetAlphaBeta(float& Alpha, float& Beta, const T& Target, const T& First, const T& Second)
{
	T PDiff0 = Second - First;
	T Dir0 = PDiff0.GetSafeNormal();
	float PDiff0Size = PDiff0 | Dir0;
	T PDiff1 = Target - First;

	Alpha = (Dir0 | PDiff1) / PDiff0Size;
	T Normal0 = PDiff1 - PDiff0 * Alpha;
	Beta = Normal0.Size() / PDiff0Size;//TODO Beta
}

template<typename T = FVector>
T LerpFromAlphaBeta(const T& First, const T& Second, float Alpha, float Beta)
{
	T Target = FMath::Lerp(First, Second, Alpha);
	//TODO Beta
	return MoveTemp(Target);
}

void UArteriesMeshSimplification::QuadricErrorMetrics(UArteriesObject* Obj, float SimpRatio)
{
	//UArteriesObject* TempObj = Obj->Copy_Impl();
	//Obj->ClearAll();
	UArteriesObject* TempObj = Obj;
	int32 TargetNum = FMath::CeilToInt(TempObj->Points.Num() * FMath::Clamp(1.f - SimpRatio, 0.f, 1.f));

	// 1. Make Error Matrices
	TMap<FArteriesPoint*, Eigen::Matrix4d> QuadricMatricesPoint;
	QuadricMatricesPoint.Reserve(TempObj->Points.Num());
	TMap<FArteriesPrimitive*, Eigen::Matrix4d> QuadricMatricesPrim;
	QuadricMatricesPrim.Reserve(TempObj->Primitives.Num());

	static Eigen::Matrix4d ZeroMat;
	ZeroMat.setZero();

	for (FArteriesPrimitive& Prim : TempObj->Primitives)
	{
		Prim.CalculateTangents();
		Eigen::Matrix4d& PrimMat = QuadricMatricesPrim.FindOrAdd(&Prim, ZeroMat);
		if (Prim.Points.Num() >= 3)
		{
			FPlane Plane(Prim.Points[0]->Position, Prim.Points[1]->Position, Prim.Points[2]->Position);
			Eigen::Vector4d NormalVec(Plane.X, Plane.Y, Plane.Z, Plane.W);
			PrimMat = NormalVec * NormalVec.transpose();
		}
	}
	TMap<FArteriesPoint*, bool> Valid;
	Valid.Reserve(TempObj->Points.Num());

	for (FArteriesPoint& P : TempObj->Points)
	{
		Valid.Add(&P, true);
		Eigen::Matrix4d& Mat = QuadricMatricesPoint.FindOrAdd(&P, ZeroMat);
		for (FArteriesPrimitive* Primitive : P.GetPrimitives())
		{
			Mat += QuadricMatricesPrim[Primitive];
		}
	}

	// 2. Get Valid Pairs
	TSortedMap<TPair<FArteriesPoint*, FArteriesPoint*>, double> QuadricErrorsEdge;
	TSet<TPair<FArteriesPoint*, FArteriesPoint*> > EdgeSet;
	for (FArteriesPoint& P : TempObj->Points)
	{
		for (FArteriesLink& Link : P.Links)
		{
			if (Link.Inverse)
			{
				EdgeSet.Add(MakeTuple(Link.Target, &P));
			}
			else
			{
				EdgeSet.Add(MakeTuple(&P, Link.Target));
			}
		}
	}
	QuadricErrorsEdge.Reserve(EdgeSet.Num());
	for (auto& EdgePair : EdgeSet)
	{
		FVector PosFirst = EdgePair.Get<0>()->Position, PosSecond = EdgePair.Get<1>()->Position;
		Eigen::Vector4d Pos4First(PosFirst.X, PosFirst.Y, PosFirst.Z, 1.f), Pos4Second(PosSecond.X, PosSecond.Y, PosSecond.Z, 1.f);
		double Value = (Pos4First.transpose() * QuadricMatricesPoint[EdgePair.Get<0>()] * Pos4First);
		Value += (Pos4Second.transpose() * QuadricMatricesPoint[EdgePair.Get<1>()] * Pos4Second);
		QuadricErrorsEdge.Add(EdgePair, Value);
	}
	for (int32 CurrentNum = TempObj->Points.Num(); CurrentNum > TargetNum; --CurrentNum)
	{
		// 3. 
		auto& Pair = *QuadricErrorsEdge.begin();
		FArteriesPoint* First = Pair.Get<0>().Get<0>();
		FArteriesPoint* Second = Pair.Get<0>().Get<1>();

		auto& FirstMat = QuadricMatricesPoint[First];
		FVector TargetPos = (First->Position + Second->Position) * 0.5f;
		if (!FMath::IsNearlyZero(FirstMat.determinant()))
		{
			Eigen::Vector4d TargetPos4 = FirstMat.lu().solve(Eigen::Vector4d(0., 0., 0., 1.));
			double De = 1.;
			if (!FMath::IsNearlyZero(TargetPos4.w()))
			{
				De = TargetPos4.w();
			}
			TargetPos.X = TargetPos4.x() / De;
			TargetPos.Y = TargetPos4.y() / De;
			TargetPos.Z = TargetPos4.z() / De;
		}
		First->Position = TargetPos;

		// 4. Update Edges
		TSet<FArteriesPoint*> AdjPoints;

		QuadricMatricesPoint[First] += QuadricMatricesPoint[Second];
		Eigen::Vector4d Pos4First(First->Position.X, First->Position.Y, First->Position.Z, 1.);
		for (FArteriesLink& Link : First->Links)
		{
			if (Link.Target == Second)
			{
				continue;
			}
			Eigen::Vector4d Pos4NewSec(Link.Target->Position.X, Link.Target->Position.Y, Link.Target->Position.Z, 1.);
			double NewValue = (Pos4First.transpose() * QuadricMatricesPoint[First] * Pos4First);
			NewValue += (Pos4NewSec.transpose() * QuadricMatricesPoint[Link.Target] * Pos4NewSec);
			TPair<FArteriesPoint*, FArteriesPoint*> Pair1 = MakeTuple(First, Link.Target), Pair2 = MakeTuple(Link.Target, First);

			double* Value1 = QuadricErrorsEdge.Find(Pair1);
			if (Value1)
			{
				QuadricErrorsEdge.Remove(Pair1);
			}
			double* Value2 = QuadricErrorsEdge.Find(Pair2);
			if (Value2)
			{
				QuadricErrorsEdge.Remove(Pair2);
			}
			if (Valid[Link.Target])
			{
				QuadricErrorsEdge.Add(MakeTuple(Link.Target, First), NewValue);
				// Update Links InPlace
				//for (FArteriesLink& AdjLink : Link.Target->Links)
				//{
				//	if (AdjLink.Target == Second && !AdjPoints.Contains(AdjLink.Target))
				//	{
				//		AdjLink.Target = First;
				//		AdjPoints.Add(AdjLink.Target);
				//	}
				//}
			}
		}
		QuadricErrorsEdge.Remove(Pair.Get<0>());
		for (FArteriesLink& Link : Second->Links)
		{
			if (Link.Target == First)
			{
				continue;
			}
			Eigen::Vector4d Pos4NewSec(Link.Target->Position.X, Link.Target->Position.Y, Link.Target->Position.Z, 1.);
			double NewValue = (Pos4First.transpose() * QuadricMatricesPoint[First] * Pos4First);
			NewValue += (Pos4NewSec.transpose() * QuadricMatricesPoint[Link.Target] * Pos4NewSec);
			TPair<FArteriesPoint*, FArteriesPoint*> Pair1 = MakeTuple(Second, Link.Target), Pair2 = MakeTuple(Link.Target, Second);

			double* Value1 = QuadricErrorsEdge.Find(Pair1);
			if (Value1)
			{
				QuadricErrorsEdge.Remove(Pair1);
			}
			double* Value2 = QuadricErrorsEdge.Find(Pair2);
			if (Value2)
			{
				QuadricErrorsEdge.Remove(Pair2);
			}
			if (Valid[Link.Target])
			{
				QuadricErrorsEdge.Add(MakeTuple(Link.Target, First), NewValue);
				// Update Links InPlace
				for (FArteriesLink& AdjLink : Link.Target->Links)
				{
					if (AdjLink.Target == Second && !AdjPoints.Contains(Link.Target))
					{
						AdjLink.Target = First;
						AdjPoints.Add(Link.Target);
					}
				}
			}
		}
		Valid[Second] = false;
	}

	// 0. Remove
	//TMap<TPair<FArteriesPoint*, FArteriesPoint*>, bool> Visited;
	//TMap<FArteriesPoint*, FArteriesPoint*> NewPointMaps;
	//NewPointMaps.Reserve(Valid.Num());
	//for (auto& Pair : Valid)
	//{
	//	FArteriesPoint* NewPoint = Obj->AddPoint(Pair.Get<0>()->Position);
	//	NewPointMaps.Add(Pair.Get<0>(), NewPoint);
	//}

	//Visited.Reserve(QuadricErrorsEdge.Num());
	//for (auto& Pair : QuadricErrorsEdge)
	//{
	//	if (Visited.Contains(Pair.Get<0>()))
	//	{
	//		continue;
	//	}
	//	Visited.Add(Pair.Get<0>(), true);
	//	FArteriesPoint* First = Pair.Get<0>().Get<0>();
	//	FArteriesPoint* Second = Pair.Get<0>().Get<1>();
	//	
	//	FArteriesPrimitive* NewPrimitive = Obj->AddPrimitive();
	//	NewPrimitive->Add(NewPointMaps[First]);
	//	FArteriesPoint* Ori = First;
	//	while (Second != Ori)
	//	{
	//		for (FArteriesLink& Link : Second->Links)
	//		{
	//			if (Link.Target == First)
	//			{
	//				continue;
	//			}
	//			auto NextPair = MakeTuple(Second, Link.Target);
	//			if (!QuadricErrorsEdge.Contains(NextPair) || Visited[NextPair])
	//			{
	//				continue;
	//			}
	//			Visited.Add(NextPair, true);
	//			First = NextPair.Get<0>();
	//			Second = NextPair.Get<1>();
	//			NewPrimitive->Add(NewPointMaps[First]);
	//			break;
	//		}
	//	}
	//}

	for (int32 i = TempObj->Points.Num() - 1; i >= 0; --i)
	{
		if (Valid[&TempObj->Points[i]] == false)
		{
			if (TempObj->NumPoints() > 0)
			{
				TempObj->DeletePoint(i);
			}
		}
	}
	TempObj->CleanPrimitives();
}

void UArteriesMeshSimplification::QuadricErrorMetricsDirect(UProceduralMeshComponent* Mesh, float SimpRatio, bool bKeepBoundary)
{
	for (int32 i = 0; i < Mesh->GetNumSections(); ++i)
	{
		FProcMeshSection* SectionPtr = Mesh->GetProcMeshSection(i);

		TSet<int32> BoundaryVertices;
		if (bKeepBoundary)
		{
			UArteriesMeshQuery::FindBoundary(BoundaryVertices, SectionPtr);
		}

		TArray<FProcMeshVertex>& Vertices = SectionPtr->ProcVertexBuffer;
		TArray<uint32>& Indices = SectionPtr->ProcIndexBuffer;

		int32 OriVertexNum = Vertices.Num();
		int32 OriTriangleNum = Indices.Num() / 3;

		int32 TargetNum = FMath::CeilToInt(OriVertexNum * FMath::Clamp(1.f - SimpRatio, 0.f, 1.f));

		// 0. Make Vertex To Triangles Map
		TMap<int32, TSet<int32>> VertToTris;
		VertToTris.Reserve(OriVertexNum);
		for (int32 TriIdx = 0; TriIdx < OriTriangleNum; ++TriIdx)
		{
			VertToTris.FindOrAdd(Indices[TriIdx * 3 + 0]).Add(TriIdx);
			VertToTris.FindOrAdd(Indices[TriIdx * 3 + 1]).Add(TriIdx);
			VertToTris.FindOrAdd(Indices[TriIdx * 3 + 2]).Add(TriIdx);
		}

		// 1. Make Error Matrices
		TMap<int32, Eigen::Matrix4d> QuadricMatricesVertex;
		QuadricMatricesVertex.Reserve(OriVertexNum);
		TMap<int32, Eigen::Matrix4d> QuadricMatricesTri;
		QuadricMatricesTri.Reserve(OriTriangleNum);

		static Eigen::Matrix4d ZeroMat;
		ZeroMat.setZero();

		for (int32 TriIdx = 0; TriIdx < OriTriangleNum; ++TriIdx)
		{
			Eigen::Matrix4d& PrimMat = QuadricMatricesTri.FindOrAdd(TriIdx, ZeroMat);
			FPlane Plane(
				Vertices[Indices[TriIdx * 3 + 0]].Position, 
				Vertices[Indices[TriIdx * 3 + 1]].Position, 
				Vertices[Indices[TriIdx * 3 + 2]].Position);
			Eigen::Vector4d NormalVec(Plane.X, Plane.Y, Plane.Z, Plane.W);
			PrimMat = NormalVec * NormalVec.transpose();
		}
		TMap<int32, bool> ValidVertices;
		TMap<int32, bool> ValidTriangles;
		ValidVertices.Reserve(OriVertexNum);
		ValidTriangles.Reserve(OriTriangleNum);

		for (int32 VIdx = 0; VIdx < Vertices.Num(); ++VIdx)
		{
			ValidVertices.Add(VIdx, true);
			Eigen::Matrix4d& Mat = QuadricMatricesVertex.FindOrAdd(VIdx, ZeroMat);
			for (int32 TriIdx : VertToTris[VIdx])
			{
				Mat += QuadricMatricesTri[TriIdx];
			}
		}

		// 2. Get Valid Pairs
		TMap<TPair<int32, int32>, double> QuadricErrorsEdge;
		TSet<TPair<int32, int32> > EdgeSet;

		for (int32 TriIdx = 0; TriIdx < OriTriangleNum; ++TriIdx)
		{
			ValidTriangles.Add(TriIdx, true);
			TArray<uint32> VIdxs { Indices[TriIdx * 3 + 0], Indices[TriIdx * 3 + 1], Indices[TriIdx * 3 + 2] };
			VIdxs.Sort();

			EdgeSet.Add(MakeTuple(static_cast<int32>(VIdxs[0]), static_cast<int32>(VIdxs[1])));
			EdgeSet.Add(MakeTuple(static_cast<int32>(VIdxs[0]), static_cast<int32>(VIdxs[2])));
			EdgeSet.Add(MakeTuple(static_cast<int32>(VIdxs[1]), static_cast<int32>(VIdxs[2])));
		}

		QuadricErrorsEdge.Reserve(EdgeSet.Num());
		for (auto& EdgePair : EdgeSet)
		{
			FVector PosFirst = Vertices[EdgePair.Get<0>()].Position, PosSecond = Vertices[EdgePair.Get<1>()].Position;
			Eigen::Vector4d Pos4First(PosFirst.X, PosFirst.Y, PosFirst.Z, 1.f), Pos4Second(PosSecond.X, PosSecond.Y, PosSecond.Z, 1.f);
			double Value = (Pos4First.transpose() * QuadricMatricesVertex[EdgePair.Get<0>()] * Pos4First);
			Value += (Pos4Second.transpose() * QuadricMatricesVertex[EdgePair.Get<1>()] * Pos4Second);
			QuadricErrorsEdge.Add(EdgePair, Value);
		}

		for (int32 Num = OriVertexNum; Num > TargetNum; --Num)
		{
			// 3. Update Vertex
			static auto GetSmallestErrorEdge =
				[Num, bKeepBoundary, &BoundaryVertices](TMap<TPair<int32, int32>, double>& QuadricErrorsEdge) -> TPair<int32, int32>*
			{
				TPair<int32, int32>* PairPtr = nullptr;
				TOptional<double> CurError;
				for (auto& QEEPair : QuadricErrorsEdge)
				{
					if (bKeepBoundary && (BoundaryVertices.Contains(QEEPair.Key.Get<0>()) || BoundaryVertices.Contains(QEEPair.Key.Get<1>())))
					{
						continue;
					}
					if (!CurError || QEEPair.Value < CurError.GetValue())
					{
						CurError = QEEPair.Value;
						PairPtr = &QEEPair.Key;
					}
				}
				if (PairPtr)
				{
					UE_LOG(LogTemp, Warning, TEXT("[%d]: Get pair <%d, %d> with error %lf"),
						Num, PairPtr->Key, PairPtr->Value, CurError.GetValue());
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("[%d]: Get pair <null, null>"),
						Num);
				}
				return PairPtr;
			};

			auto* QuadricErrorPairPtr = GetSmallestErrorEdge(QuadricErrorsEdge);
			if (!QuadricErrorPairPtr)
			{
				continue;
			}
			auto& QuadricErrorPair = *QuadricErrorPairPtr;
			int32 First = QuadricErrorPair.Get<0>();
			int32 Second = QuadricErrorPair.Get<1>();

			auto& FirstMat = QuadricMatricesVertex[First];
			FVector FirstPos = Vertices[First].Position;
			FVector SecondPos = Vertices[Second].Position;
			FVector TargetPos = (FirstPos + SecondPos) * 0.5f;
			double Denom = 1.;
			double Det = FirstMat.determinant();
			if (!FMath::IsNearlyZero(Det))
			{
				Eigen::Vector4d TargetPos4 = FirstMat.lu().solve(Eigen::Vector4d(0., 0., 0., 1.));
				if (!FMath::IsNearlyZero(TargetPos4.w()))
				{
					Denom = -TargetPos4.w();
				}
				TargetPos.X = TargetPos4.x() / Denom;
				TargetPos.Y = TargetPos4.y() / Denom;
				TargetPos.Z = TargetPos4.z() / Denom;
			}

			float Alpha = 0.5f, Beta = 0.f;
			GetAlphaBeta(Alpha, Beta, TargetPos, FirstPos, SecondPos);

			Vertices[First].Position = TargetPos;
			Vertices[Second].Position = TargetPos;

			Vertices[Second].Color = Vertices[First].Color = LerpFromAlphaBeta(
				FLinearColor(Vertices[First].Color), 
				FLinearColor(Vertices[Second].Color), 
				Alpha, Beta).ToFColor(true);
			Vertices[Second].UV0 = Vertices[First].UV0 = LerpFromAlphaBeta(Vertices[First].UV0, Vertices[Second].UV0, Alpha, Beta);
			Vertices[Second].UV1 = Vertices[First].UV1 = LerpFromAlphaBeta(Vertices[First].UV1, Vertices[Second].UV1, Alpha, Beta);
			Vertices[Second].UV2 = Vertices[First].UV2 = LerpFromAlphaBeta(Vertices[First].UV2, Vertices[Second].UV2, Alpha, Beta);
			Vertices[Second].UV3 = Vertices[First].UV3 = LerpFromAlphaBeta(Vertices[First].UV3, Vertices[Second].UV3, Alpha, Beta);

			QuadricMatricesVertex[First] += QuadricMatricesVertex[Second];

			// 4. Update Edges
			Eigen::Vector4d Pos4First(Vertices[First].Position.X, Vertices[First].Position.Y, Vertices[First].Position.Z, 1.);

			TArray<TPair<int32, int32>> TrisToUpdate;
			TrisToUpdate.Reserve(VertToTris[First].Num() + VertToTris[Second].Num());
			for (int32 Tri : VertToTris[First])
			{
				if (!ValidTriangles[Tri])
				{
					continue;
				}
				TrisToUpdate.Add(MakeTuple(First, Tri));
			}
			for (int32 Tri : VertToTris[Second])
			{
				if (!ValidTriangles[Tri])
				{
					continue;
				}
				TrisToUpdate.Add(MakeTuple(Second, Tri));
			}

			TSet<TPair<int32, int32>> ProcessedEdges;

			for (TPair<int32, int32>& TarTriPair : TrisToUpdate)
			{
				int32 SrcVIdx = TarTriPair.Key;
				int32 TarTriIdx = TarTriPair.Value;
				TArray<uint32> TarVIdxs{ Indices[TarTriIdx * 3 + 0], Indices[TarTriIdx * 3 + 1], Indices[TarTriIdx * 3 + 2] };
				for (int32 TarVIdx : TarVIdxs)
				{
					// 4.1 Update QuadricErrorsEdge
					if (TarVIdx == SrcVIdx)
					{
						continue;
					}

					FProcMeshVertex& TargetVertex = Vertices[TarVIdx];

					Eigen::Vector4d Pos4Target(TargetVertex.Position.X, TargetVertex.Position.Y, TargetVertex.Position.Z, 1.);
					double NewValue = (Pos4First.transpose() * QuadricMatricesVertex[First] * Pos4First);
					NewValue += (Pos4Target.transpose() * QuadricMatricesVertex[TarVIdx] * Pos4Target);
					TPair<int32, int32> Pair1 = MakeTuple(SrcVIdx, TarVIdx), Pair2 = MakeTuple(TarVIdx, SrcVIdx);

					double* Value1 = QuadricErrorsEdge.Find(Pair1);
					if (Value1 && !ProcessedEdges.Contains(Pair1))
					{
						QuadricErrorsEdge.Remove(Pair1);
					}
					double* Value2 = QuadricErrorsEdge.Find(Pair2);
					if (Value2 && !ProcessedEdges.Contains(Pair2))
					{
						QuadricErrorsEdge.Remove(Pair2);
					}

					if (TarVIdx == Second || TarVIdx == First || !ValidVertices[TarVIdx])
					{
						continue;
					}

					if (ValidVertices[TarVIdx])
					{
						TPair<int32, int32> NewPair = MakeTuple(First, TarVIdx);
						if (!ProcessedEdges.Contains(NewPair))
						{
							UE_LOG(LogTemp, Warning, TEXT("[%d]: Add pair <%d, %d> with error %lf"),
								Num, NewPair.Key, NewPair.Value, NewValue);
							QuadricErrorsEdge.Add(NewPair, NewValue);
							ProcessedEdges.Add(NewPair);
						}
					}
				}
			}

			// 5. Update Triangles
			for (int32 FirTriIdx : VertToTris[First])
			{
				if (!ValidTriangles[FirTriIdx])
				{
					continue;
				}
				TArray<uint32*> FirAdjVIdxPtrs{ &Indices[FirTriIdx * 3 + 0], &Indices[FirTriIdx * 3 + 1], &Indices[FirTriIdx * 3 + 2] };
				bool bValidTriangle = true;
				for (uint32* SecAdjVIdxPtr : FirAdjVIdxPtrs)
				{
					if (*SecAdjVIdxPtr == Second)
					{
						bValidTriangle = false;
						break;
					}
				}
				if (!bValidTriangle)
				{
					ValidTriangles[FirTriIdx] = false;
					continue;
				}
			}
			for (int32 SecTriIdx : VertToTris[Second])
			{
				if (!ValidTriangles[SecTriIdx])
				{
					continue;
				}
				TArray<uint32*> SecAdjVIdxPtrs{ &Indices[SecTriIdx * 3 + 0], &Indices[SecTriIdx * 3 + 1], &Indices[SecTriIdx * 3 + 2] };
				bool bValidTriangle = true;
				for (uint32* SecAdjVIdxPtr : SecAdjVIdxPtrs)
				{
					if (*SecAdjVIdxPtr == First)
					{
						bValidTriangle = false;
						break;
					}
				}
				if (!bValidTriangle)
				{
					ValidTriangles[SecTriIdx] = false;
					//QuadricMatricesVertex.Remove(SecTriIdx);
					continue;
				}
				for (uint32* SecAdjVIdxPtr : SecAdjVIdxPtrs)
				{
					if (*SecAdjVIdxPtr == Second)
					{
						*SecAdjVIdxPtr = First;
					}
				}
			}
			VertToTris[First].Append(VertToTris[Second]);
			VertToTris.Remove(Second);
			QuadricMatricesVertex.Remove(Second);
			QuadricErrorsEdge.Remove(QuadricErrorPair);
			ValidVertices[Second] = false;
		}

		// 6. Remove Invalid
		TMap<int32, int32> VertexIdxMap;
		TArray<FProcMeshVertex> OriVertices(Vertices);
		TArray<uint32> OriIndices(Indices);
		Vertices.Reset();
		Indices.Reset();
		VertexIdxMap.Reserve(ValidVertices.Num());
		for (int32 VIdx = 0; VIdx < OriVertexNum; ++VIdx)
		{
			if (ValidVertices[VIdx])
			{
				int32 NewVIdx = Vertices.Add(OriVertices[VIdx]);
				VertexIdxMap.Add(VIdx, NewVIdx);
			}
		}
		for (int32 TriIdx = 0; TriIdx < OriTriangleNum; ++TriIdx)
		{
			if (ValidTriangles[TriIdx])
			{
				int32 IIdx0 = TriIdx * 3 + 0;
				int32 IIdx1 = TriIdx * 3 + 1;
				int32 IIdx2 = TriIdx * 3 + 2;

				Indices.Add(VertexIdxMap[OriIndices[IIdx0]]);
				Indices.Add(VertexIdxMap[OriIndices[IIdx1]]);
				Indices.Add(VertexIdxMap[OriIndices[IIdx2]]);
			}
		}
		Mesh->SetProcMeshSection(i, *SectionPtr);
	}
}
