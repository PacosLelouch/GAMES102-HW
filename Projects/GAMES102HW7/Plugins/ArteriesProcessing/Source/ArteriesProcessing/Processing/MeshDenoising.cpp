// Copyright 2020 PacosLelouch, Inc. All Rights Reserved.
// https://github.com/PacosLelouch/

#include "MeshDenoising.h"
#include "../Query/MeshQuery.h"
#include "ArteriesObject.h"
#include "ProceduralMeshComponent.h"
#include "KismetProceduralMeshLibrary.h"
#include "Materials/MaterialInterface.h"
#include "Engine/StaticMesh.h"
#include "PhysicsEngine/BodySetup.h"
#include "Eigen/Sparse"

//const FName UArteriesMeshDenoising::LastPosTag = TEXT("LastPos");
const FName UArteriesMeshDenoising::LaplaceBeltramiTag = TEXT("LaplaceBeltrami");
const FName UArteriesMeshDenoising::MeanCurvatureTag = TEXT("MeanCurvature");
const FName UArteriesMeshDenoising::GaussianCurvatureTag = TEXT("GaussianCurvature");

void UArteriesMeshDenoising::MinimalSurfaceLocalInPlace(UArteriesObject* Obj, float MoveRatio, int32 MaxLoopCount)
{
	if (!IsValid(Obj))
	{
		return;
	}

	// 1. Get Boundary
	TSet<FArteriesPoint*> BoundaryPoints;
	UArteriesMeshQuery::FindBoundary(BoundaryPoints, Obj);

	for (int32 Loop = 0; Loop < MaxLoopCount; ++Loop)
	{
		// 2. Update Infos
		UpdateVertexInfos(Obj);

		// 3. Update Positions
		for (FArteriesPoint& Point : Obj->Points)
		{
			if (BoundaryPoints.Contains(&Point))
			{
				continue;
			}

			Point.Position -= Point.GetVec3(LaplaceBeltramiTag) * MoveRatio * 2.f;
		}
	}

	// 0. Update Infos
	UpdateVertexInfos(Obj);
}

static void AddVectorToTriple(TArray<Eigen::Triplet<double> >& Triplets, int32 RowPackNum, int32 ColPackNum, int32 RowStart, int32 ColStart, double Value, int32 Dim = 3)
{
	for (int32 i = 0; i < Dim; ++i)
	{
		Triplets.Emplace(RowStart + RowPackNum * i, ColStart + ColPackNum * i, Value);
	}
}

//static void AddVectorToTriple(TArray<Eigen::Triplet<double> >& Triplets, int32 RowPackNum, int32 ColPackNum, int32 RowPack, int32 ColPack, const FVector& Value)
//{
//	Triplets.Emplace(RowPack, ColPack, Value.X);
//	Triplets.Emplace(RowPack + RowPackNum, ColPack + ColPackNum, Value.Y);
//	Triplets.Emplace(RowPack + (RowPackNum << 1), ColPack + (ColPackNum << 1), Value.Z);
//}

static void AddVectorToMatrix(Eigen::SparseMatrix<double>& Matrix, int32 RowPackNum, int32 ColPackNum, int32 RowStart, int32 ColStart, double Value, int32 Dim = 3)
{
	for (int32 i = 0; i < Dim; ++i)
	{
		Matrix.insert(RowStart + RowPackNum * i, ColStart + ColPackNum * i) = Value;
	}
}

//static void AddVectorToMatrix(Eigen::SparseMatrix<double>& Matrix, int32 RowPackNum, int32 ColPackNum, int32 RowPack, int32 ColPack, const FVector& Value)
//{
//	Matrix.insert(RowPack, ColPack) = Value.X;
//	Matrix.insert(RowPack + RowPackNum, ColPack + ColPackNum) = Value.Y;
//	Matrix.insert(RowPack + (RowPackNum << 1), ColPack + (ColPackNum << 1)) = Value.Z;
//}

static void PackVector(Eigen::VectorXd& EigenVector, int32 RowPackNum, int32 RowStart, const FVector& Value, int32 Dim = 3)
{
	for (int32 i = 0; i < Dim; ++i)
	{
		EigenVector(RowStart + RowPackNum * i) = Value[i];
	}
	for (int32 i = Dim; i < 3; ++i)
	{
		EigenVector(RowStart + RowPackNum * i) = 0.;
	}
	//EigenVector(RowStart) = Value.X;
	//EigenVector(RowStart + RowPackNum) = Value.Y;
	//EigenVector(RowStart + (RowPackNum << 1)) = Value.Z;
}

static void UnpackVector(FVector& Value, int32 RowPackNum, int32 RowStart, const Eigen::VectorXd& EigenVector, int32 Dim = 3)
{
	for (int32 i = 0; i < Dim; ++i)
	{
		Value[i] = EigenVector(RowStart + RowPackNum * i);
	}
	//Value.X = EigenVector(RowStart);
	//Value.Y = EigenVector(RowStart + RowPackNum);
	//Value.Z = EigenVector(RowStart + (RowPackNum << 1));
}

void UArteriesMeshDenoising::MinimalSurfaceGlobalSoftInPlace(UArteriesObject* Obj, float MoveRatio)
{
	if (!IsValid(Obj))
	{
		return;
	}

	// 1. Get Boundary
	TSet<FArteriesPoint*> BoundaryPoints;
	UArteriesMeshQuery::FindBoundary(BoundaryPoints, Obj);

	int32 ColPackNum = Obj->NumPoints();
	TMap<FArteriesPoint*, int32> IndexMap;
	IndexMap.Reserve(ColPackNum);
	for (int32 i = 0; i < Obj->NumPoints(); ++i)
	{
		IndexMap.Add(&Obj->Points[i], i);
	}

	// 2. Make Sparse Matrix
	Eigen::SparseMatrix<double> Matrix(ColPackNum * 3 + BoundaryPoints.Num() * 3, ColPackNum * 3); // NumRows: Points.Num + Bound.Num, NumCols: Points.Num
	Eigen::VectorXd Sigma(ColPackNum * 3 + BoundaryPoints.Num() * 3);

	for (FArteriesPoint& Point : Obj->Points)
	{
		int32 CurIndex = IndexMap[&Point];

		TArray<FArteriesPoint*> Neighborhood;
		TArray<FVector> NormalizedEdgeVec;
		TArray<float> CotNext;
		TArray<float> CotPrev;

		Neighborhood.Reserve(Point.Links.Num());
		NormalizedEdgeVec.Reserve(Point.Links.Num());
		CotNext.Reserve(Point.Links.Num());
		CotPrev.Reserve(Point.Links.Num());

		TArray<FArteriesLink*> SortedLinks = Point.GetSortedLinks(true, false);
		for (FArteriesLink* LinkP : SortedLinks)
		{
			FArteriesLink& Link = *LinkP;
			if (!Neighborhood.Contains(Link.Target))
			{
				Neighborhood.Add(Link.Target);
				NormalizedEdgeVec.Add((Point.Position - Link.Target->Position).GetSafeNormal());
			}
		}

		if (true)//(Neighborhood.Num() > 2)
		{
			for (int32 i = 0; i < Neighborhood.Num(); ++i)
			{
				int32 PrevI = (i == 0) ? Neighborhood.Num() - 1 : i - 1;
				int32 NextI = (i + 1 == Neighborhood.Num()) ? 0 : i + 1;

				const FVector& NormOC = -NormalizedEdgeVec[i];
				FVector PC = Neighborhood[i]->Position - Neighborhood[PrevI]->Position;
				FVector NC = Neighborhood[i]->Position - Neighborhood[NextI]->Position;
				FVector NormPC = PC.GetSafeNormal();
				FVector NormNC = NC.GetSafeNormal();

				auto CosPrev = NormPC | NormOC, SinPrev = (NormPC ^ NormOC).Size();
				auto CosNext = NormNC | NormOC, SinNext = (NormNC ^ NormOC).Size();
				CotPrev.Add(CosPrev / SinPrev);
				CotNext.Add(CosNext / SinNext);
			}
		}

		double TotalWeightCot = 0.;
		if (true)//(Neighborhood.Num() > 2)
		{
			bool bBoundary = BoundaryPoints.Contains(&Point);
			int32 StartI = bBoundary ? 1 : 0;
			int32 EndI = bBoundary ? Neighborhood.Num() - 1 : Neighborhood.Num();
			for (int32 i = StartI; i < EndI; ++i)
			{
				int32 PrevI = (i == 0) ? Neighborhood.Num() - 1 : i - 1;
				int32 NextI = (i + 1 == Neighborhood.Num()) ? 0 : i + 1;

				// Laplace-Beltrami
				auto CotAlpha = CotNext[PrevI], CotBeta = CotPrev[NextI];

				double WeightCot = CotAlpha + CotBeta;

				FArteriesPoint* Neighbor = Neighborhood[i];
				int32 NeighborIndex = IndexMap[Neighbor];
				AddVectorToMatrix(Matrix, ColPackNum, ColPackNum, CurIndex, NeighborIndex, -WeightCot);
				TotalWeightCot += WeightCot;
			}
		}
		//PackVector(Sigma, ColPackNum, CurIndex, Point.Position * (1.f - MoveRatio));
		PackVector(Sigma, ColPackNum, CurIndex, FVector::ZeroVector);
		AddVectorToMatrix(Matrix, ColPackNum, ColPackNum, CurIndex, CurIndex, TotalWeightCot);
	}

	// 2.5 Add Boundary Constraints
	int32 AdditionalRowNum = 0;
	for (FArteriesPoint* BoundaryPoint : BoundaryPoints)
	{
		int32 BoundaryIndex = IndexMap[BoundaryPoint];
		PackVector(Sigma, 1, ColPackNum * 3 + AdditionalRowNum * 3, BoundaryPoint->Position);
		AddVectorToMatrix(Matrix, 1, ColPackNum, ColPackNum * 3 + AdditionalRowNum * 3, BoundaryIndex, 1.);
		++AdditionalRowNum;
	}

	// 3. Solve Sparse Matrix
	//Matrix.setFromTriplets(Triplets.GetData(), Triplets.GetData() + Triplets.Num());
	Eigen::SparseMatrix<double> MartixT = Matrix.transpose();
	Eigen::SimplicialCholesky<Eigen::SparseMatrix<double>> Cholesky(MartixT * Matrix);
	Eigen::VectorXd NewVs = Cholesky.solve(MartixT * Sigma);

	// 4. Update Positions
	for (FArteriesPoint& Point : Obj->Points)
	{
		//if (BoundaryPoints.Contains(&Point))
		//{
		//	continue;
		//}
		UnpackVector(Point.Position, ColPackNum, IndexMap[&Point], NewVs);
	}

	// 0. Update Infos
	UpdateVertexInfos(Obj);
}

// Bug
void UArteriesMeshDenoising::MinimalSurfaceGlobalHardInPlace(UArteriesObject* Obj, float MoveRatio)
{
	if (!IsValid(Obj))
	{
		return;
	}

	// 1. Get Boundary
	TSet<FArteriesPoint*> BoundaryPoints;
	UArteriesMeshQuery::FindBoundary(BoundaryPoints, Obj);

	MinimalSurfaceValueGlobalHardInPlace(Obj, BoundaryPoints, 3, NAME_None);

	// 0. Update Infos
	UpdateVertexInfos(Obj);
}

void UArteriesMeshDenoising::UpdateVertexInfos(UArteriesObject* Obj)
{
	if (!IsValid(Obj))
	{
		return;
	}

	for (FArteriesPoint& Point : Obj->Points)
	{
		//Point.SetVec3(LastPosTag, Point.Position);

		TArray<FArteriesPoint*> Neighborhood;
		TArray<FVector> NormalizedEdgeVec;
		TArray<float> CotNext;
		TArray<float> CotPrev;

		Neighborhood.Reserve(Point.Links.Num());
		NormalizedEdgeVec.Reserve(Point.Links.Num());
		CotNext.Reserve(Point.Links.Num());
		CotPrev.Reserve(Point.Links.Num());

		TArray<FArteriesLink*> SortedLinks = Point.GetSortedLinks(true, false);
		//auto Area = 0.f;
		for (FArteriesLink* LinkP : SortedLinks)
		{
			FArteriesLink& Link = *LinkP;
			if (!Neighborhood.Contains(Link.Target))
			{
				Neighborhood.Add(Link.Target);
				//auto CurArea = Link.Primitive->CalculateArea();
				//Area += CurArea;
				NormalizedEdgeVec.Add((Point.Position - Link.Target->Position).GetSafeNormal());
			}
		}

		if (true)//(Neighborhood.Num() > 2)
		{
			for (int32 i = 0; i < Neighborhood.Num(); ++i)
			{
				int32 PrevI = (i == 0) ? Neighborhood.Num() - 1 : i - 1;
				int32 NextI = (i + 1 == Neighborhood.Num()) ? 0 : i + 1;

				const FVector& NormOC = -NormalizedEdgeVec[i];
				FVector PC = Neighborhood[i]->Position - Neighborhood[PrevI]->Position;
				FVector NC = Neighborhood[i]->Position - Neighborhood[NextI]->Position;
				FVector NormPC = PC.GetSafeNormal();
				FVector NormNC = NC.GetSafeNormal();

				auto CosPrev = NormPC | NormOC, SinPrev = (NormPC ^ NormOC).Size();
				auto CosNext = NormNC | NormOC, SinNext = (NormNC ^ NormOC).Size();
				CotPrev.Add(CosPrev / SinPrev);
				CotNext.Add(CosNext / SinNext);
			}
		}

		FVector LaplaceBeltrami(0.f);

		auto AreaM = 0.f;
		auto GaussBonnet = 0.f;

		if (true)//(Neighborhood.Num() > 2)
		{
			GaussBonnet = PI * 2.f;
			for (int32 i = 0; i < Neighborhood.Num(); ++i)
			{
				int32 PrevI = (i == 0) ? Neighborhood.Num() - 1 : i - 1;
				int32 NextI = (i + 1 == Neighborhood.Num()) ? 0 : i + 1;

				// Laplace-Beltrami
				//const FVector& NormPO = NormalizedEdgeVec[PrevI];
				//const FVector& NormNO = NormalizedEdgeVec[NextI];
				//FVector PC = Neighborhood[i]->Position - Neighborhood[PrevI]->Position;
				//FVector NC = Neighborhood[i]->Position - Neighborhood[NextI]->Position;
				//FVector NormPC = PC.GetSafeNormal();
				//FVector NormNC = NC.GetSafeNormal();

				//auto CosAlpha = NormPC | NormPO, SinAlpha = (NormPC ^ NormPO).Size();
				//auto CosBeta = NormNC | NormNO, SinBeta = (NormNC ^ NormNO).Size();
				//auto CotAlpha = CosAlpha / SinAlpha, CotBeta = CosBeta / SinBeta;
				auto CotAlpha = CotNext[PrevI], CotBeta = CotPrev[NextI];

				FVector CO = Point.Position - Neighborhood[i]->Position;

				LaplaceBeltrami += CO * (CotAlpha + CotBeta);

				// Gaussian-Bonnet
				const FVector& NormPO = NormalizedEdgeVec[PrevI];
				const FVector& NormCO = NormalizedEdgeVec[i];
				FVector PO = Point.Position - Neighborhood[PrevI]->Position;
				auto CosTheta = NormPO | NormCO;
				auto AngleTheta = FMath::Acos(CosTheta);
				GaussBonnet -= AngleTheta;

				auto CotCurPrev = CotPrev[i];
				auto LengthCur = CO | NormCO;
				auto LengthPrev = PO | NormPO;
				
				// Mixed Voronoi Cell
				if (CosTheta >= 0.f)
				{
					AreaM += 0.125f * (LengthCur * LengthCur * CotAlpha + LengthPrev * LengthPrev * CotCurPrev);
				}
				else
				{
					AreaM += 0.25f * (PO ^ CO).Size();
				}
			}
		}

		//LaplaceBeltrami /= (2.f * Area);
		if (AreaM > 0.f)
		{
			LaplaceBeltrami /= (2.f * AreaM);
		}
		Point.SetVec3(LaplaceBeltramiTag, LaplaceBeltrami);
		FVector Normal = LaplaceBeltrami.IsNearlyZero() ? FVector::ZeroVector : LaplaceBeltrami.GetSafeNormal();
		Point.SetVec3(AVN_TangentZ, Normal);
		auto MeanCurvature = Normal | LaplaceBeltrami;
		Point.SetFloat(MeanCurvatureTag, MeanCurvature);

		if (AreaM > 0.f)
		{
			GaussBonnet /= AreaM;
		}
		Point.SetFloat(GaussianCurvatureTag, GaussBonnet);
	}
}

void UArteriesMeshDenoising::MinimalSurfaceValueGlobalHardInPlace(UArteriesObject* Obj, const TSet<FArteriesPoint*>& BoundaryPoints, int32 Dim, FName Tag)
{
	auto GetValueFunc = [Tag, Dim](FArteriesPoint& Point) -> FVector
	{
		if (Dim == 2)
		{
			return FVector(Point.Vec2Values[Tag], 0.f);
		}
		else if (Dim == 3 && Tag != NAME_None)
		{
			return Point.Vec3Values[Tag];
		}
		return Point.Position;
	};

	auto GetValueRefFunc = [Tag, Dim](FArteriesPoint& Point) -> FVector&
	{
		if (Dim == 2)
		{
			return (FVector&)Point.Vec2Values[Tag];
		}
		else if (Dim == 3 && Tag != NAME_None)
		{
			return Point.Vec3Values[Tag];
		}
		return Point.Position;
	};

	int32 ColPackNum = Obj->NumPoints() - BoundaryPoints.Num();
	TMap<FArteriesPoint*, int32> IndexMap;
	IndexMap.Reserve(ColPackNum);
	for (int32 i = 0; i < Obj->NumPoints(); ++i)
	{
		if (BoundaryPoints.Contains(&Obj->Points[i]))
		{
			continue;
		}

		IndexMap.Add(&Obj->Points[i], IndexMap.Num());
	}

	// 2. Make Sparse Matrix
	//Eigen::SparseMatrix<double> Matrix(ColPackNum * 3, ColPackNum * 3); // NumRows: Points.Num + Bound.Num, NumCols: Points.Num
	//Eigen::VectorXd Sigma(ColPackNum * 3);
	Eigen::SparseMatrix<double> Matrix(ColPackNum * 3, ColPackNum * 3); // NumRows: Points.Num + Bound.Num, NumCols: Points.Num
	Eigen::VectorXd Sigma(ColPackNum * 3);

	for (FArteriesPoint& Point : Obj->Points)
	{
		//int32 CurIndex = IndexMap[&Point];

		if (BoundaryPoints.Contains(&Point))
		{
			//PackVector(Sigma, ColPackNum, CurIndex, GetValueFunc(Point));
			//AddVectorToMatrix(Matrix, ColPackNum, ColPackNum, CurIndex, CurIndex, 1.);
			continue;
		}
		else
		{
			int32 CurIndex = IndexMap[&Point];
			TArray<FArteriesPoint*> Neighborhood;
			TArray<FVector> NormalizedEdgeVec;
			TArray<float> CotNext;
			TArray<float> CotPrev;

			Neighborhood.Reserve(Point.Links.Num());
			NormalizedEdgeVec.Reserve(Point.Links.Num());
			CotNext.Reserve(Point.Links.Num());
			CotPrev.Reserve(Point.Links.Num());

			TArray<FArteriesLink*> SortedLinks = Point.GetSortedLinks(true, false);
			for (FArteriesLink* LinkP : SortedLinks)
			{
				FArteriesLink& Link = *LinkP;
				if (!Neighborhood.Contains(Link.Target))
				{
					Neighborhood.Add(Link.Target);
					NormalizedEdgeVec.Add((Point.Position - Link.Target->Position).GetSafeNormal());
				}
			}

			if (true)//(Neighborhood.Num() > 2)
			{
				for (int32 i = 0; i < Neighborhood.Num(); ++i)
				{
					int32 PrevI = (i == 0) ? Neighborhood.Num() - 1 : i - 1;
					int32 NextI = (i + 1 == Neighborhood.Num()) ? 0 : i + 1;

					const FVector& NormOC = -NormalizedEdgeVec[i];
					FVector PC = Neighborhood[i]->Position - Neighborhood[PrevI]->Position;
					FVector NC = Neighborhood[i]->Position - Neighborhood[NextI]->Position;
					FVector NormPC = PC.GetSafeNormal();
					FVector NormNC = NC.GetSafeNormal();

					auto CosPrev = NormPC | NormOC, SinPrev = (NormPC ^ NormOC).Size();
					auto CosNext = NormNC | NormOC, SinNext = (NormNC ^ NormOC).Size();
					CotPrev.Add(CosPrev / SinPrev);
					CotNext.Add(CosNext / SinNext);
				}
			}

			double TotalWeightCot = 0.;
			FVector CurSigma = FVector::ZeroVector;
			if (true)//(Neighborhood.Num() > 2)
			{
				for (int32 i = 0; i < Neighborhood.Num(); ++i)
				{
					int32 PrevI = (i == 0) ? Neighborhood.Num() - 1 : i - 1;
					int32 NextI = (i + 1 == Neighborhood.Num()) ? 0 : i + 1;

					// Laplace-Beltrami
					auto CotAlpha = CotNext[PrevI], CotBeta = CotPrev[NextI];

					double WeightCot = CotAlpha + CotBeta;

					FArteriesPoint* Neighbor = Neighborhood[i];
					if (BoundaryPoints.Contains(Neighbor))
					{
						auto Value = GetValueFunc(*Neighbor);
						CurSigma += Value * WeightCot;
					}
					else
					{
						int32 NeighborIndex = IndexMap[Neighbor];
						AddVectorToMatrix(Matrix, ColPackNum, ColPackNum, CurIndex, NeighborIndex, -WeightCot, 3);
					}
					TotalWeightCot += WeightCot;
				}
			}

			PackVector(Sigma, ColPackNum, CurIndex, CurSigma, 3);
			AddVectorToMatrix(Matrix, ColPackNum, ColPackNum, CurIndex, CurIndex, TotalWeightCot, 3);
		}
	}

	// 3. Solve Sparse Matrix
	Eigen::SimplicialCholesky<Eigen::SparseMatrix<double>> Cholesky(Matrix);
	Eigen::VectorXd NewVs = Cholesky.solve(Sigma);

	// 4. Update Positions
	for (FArteriesPoint& Point : Obj->Points)
	{
		if (BoundaryPoints.Contains(&Point))
		{
			continue;
		}
		UnpackVector(GetValueRefFunc(Point), ColPackNum, IndexMap[&Point], NewVs, Dim);
	}
}
