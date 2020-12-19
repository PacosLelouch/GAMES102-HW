// Author: LiJiayu (JerryLi)
// Mail: lijiayu83@gmail.com (fullike@163.com)
// Copyright 2019. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"

struct FCubicCurve
{
	FCubicCurve() {}
	FCubicCurve(FVector* InPoints)
	{
		P0 = InPoints[0];
		P1 = InPoints[1];
		P2 = InPoints[2];
		P3 = InPoints[3];
	}
	void Split(FCubicCurve& C0, FCubicCurve& C1);
	FVector Intersection(const FCubicCurve& Other, const FMatrix& ProjectMatrix);
	FVector GetPosition(float A) const
	{
		const float IA = 1.f - A;
		const float IA2 = IA * IA;
		const float IA3 = IA * IA2;
		const float A2 = A * A;
		const float A3 = A2 * A;
		return IA3 * P0 + 3 * IA2*A*P1 + 3 * IA*A2*P2 + A3 * P3;

	//	return (((2 * A3) - (3 * A2) + 1) * P0) + ((A3 - (2 * A2) + A) * T0) + ((A3 - A2) * T1) + (((-2 * A3) + (3 * A2)) * P1);



	//	return FMath::CubicInterp(P0, P1 - P0, P3, P3 - P2, A);
	}
	FBox2D GetBox(const FMatrix& ProjectMatrix)
	{
		FBox2D Box(EForceInit::ForceInit);
		FVector PP0 = ProjectMatrix.TransformPosition(P0);
		FVector PP1 = ProjectMatrix.TransformPosition(P1);
		FVector PP2 = ProjectMatrix.TransformPosition(P2);
		FVector PP3 = ProjectMatrix.TransformPosition(P3);
		Box += (const FVector2D&)PP0;
		Box += (const FVector2D&)PP1;
		Box += (const FVector2D&)PP2;
		Box += (const FVector2D&)PP3;
		return Box;
	}
	bool SmallEnough()
	{
		return P0.Equals(P3, 0.01f);
	}
	FVector Center()
	{
		return (P0 + P3) / 2;
	}
	FVector P0;
	FVector P1;
	FVector P2;
	FVector P3;
};
ARTERIES_API FVector RandomPointInTriangle(const FVector& A, const FVector& B, const FVector& C);
ARTERIES_API float GetTriangleArea(const FVector& v0, const FVector& v1, const FVector& v2);
ARTERIES_API FVector2D CalculateCentroid(const TArray<FVector2D>& Points);