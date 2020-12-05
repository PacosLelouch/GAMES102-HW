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

//const FName UArteriesMeshDenoising::LastPosTag = TEXT("LastPos");
const FName UArteriesMeshDenoising::LaplaceBeltramiTag = TEXT("LaplaceBeltrami");
const FName UArteriesMeshDenoising::MeanCurvatureTag = TEXT("MeanCurvature");
const FName UArteriesMeshDenoising::GaussianCurvatureTag = TEXT("GaussianCurvature");

void UArteriesMeshDenoising::MinimalSurfaceInPlace(UArteriesObject* Obj, float MoveRatio, int32 MaxLoopCount)
{
	// 1. Get Boundary
	TSet<FArteriesPoint*> BoundaryPoints;
	UArteriesMeshQuery::FindBoundary(BoundaryPoints, Obj);

	for (int32 Loop = 0; Loop < MaxLoopCount; ++Loop)
	{
		// 2. Update Tags
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

	// 0. Update Tags
	UpdateVertexInfos(Obj);
}

void UArteriesMeshDenoising::UpdateVertexInfos(UArteriesObject* Obj)
{
	if (!Obj)
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

		TArray<FArteriesLink*> SortedLinks = Point.GetSortedLinks();
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
					AreaM += 0.125f * (LengthCur * CotAlpha + LengthPrev * CotCurPrev);
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
