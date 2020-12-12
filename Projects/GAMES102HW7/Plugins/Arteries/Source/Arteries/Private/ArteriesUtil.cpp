// Author: LiJiayu (JerryLi)
// Mail: lijiayu83@gmail.com (fullike@163.com)
// Copyright 2019. All Rights Reserved.

#include "ArteriesUtil.h"
#include "Containers/Queue.h"
static void deCasteljauSplit(const FVector& P0, const FVector& P1, const FVector& P2, const FVector& P3, FVector OutCurveParams[7])
{
	FVector L1 = (P0 + P1) * 0.5f;
	FVector M = (P1 + P2) * 0.5f;
	FVector R2 = (P2 + P3) * 0.5f;
	FVector L2 = (L1 + M) * 0.5f;
	FVector R1 = (M + R2) * 0.5f;
	FVector L3R0 = (L2 + R1) * 0.5f;
	OutCurveParams[0] = P0;
	OutCurveParams[1] = L1;
	OutCurveParams[2] = L2;
	OutCurveParams[3] = L3R0;
	OutCurveParams[4] = R1;
	OutCurveParams[5] = R2;
	OutCurveParams[6] = P3;
}
void FCubicCurve::Split(FCubicCurve& C0, FCubicCurve& C1)
{
	FVector Out[7];
	deCasteljauSplit(P0, P1, P2, P3, Out);
	C0 = FCubicCurve(Out);
	C1 = FCubicCurve(Out + 3);
}
FVector FCubicCurve::Intersection(const FCubicCurve& Other, const FMatrix& ProjectMatrix)
{
	struct FCollisionNode
	{
		FCollisionNode() {}
		FCollisionNode(const FCubicCurve& InC0, const FCubicCurve& InC1) :C0(InC0), C1(InC1) {}
		void Split(FCollisionNode& N0, FCollisionNode& N1, FCollisionNode& N2, FCollisionNode& N3)
		{
			FVector Out0[7];
			FVector Out1[7];
			deCasteljauSplit(C0.P0, C0.P1, C0.P2, C0.P3, Out0);
			deCasteljauSplit(C1.P0, C1.P1, C1.P2, C1.P3, Out1);
			N0.C0 = FCubicCurve(Out0);
			N0.C1 = FCubicCurve(Out1);

			N1.C0 = FCubicCurve(Out0);
			N1.C1 = FCubicCurve(Out1+3);
			
			N2.C0 = FCubicCurve(Out0+3);
			N2.C1 = FCubicCurve(Out1);
			
			N3.C0 = FCubicCurve(Out0+3);
			N3.C1 = FCubicCurve(Out1+3);
		}
		bool Intersect(const FMatrix& ProjMatrix)
		{
			return C0.GetBox(ProjMatrix).Intersect(C1.GetBox(ProjMatrix));
		}
		bool SmallEnough()
		{
			return C0.SmallEnough() && C1.SmallEnough();
		}
		FVector GetCross()
		{
			return (C0.Center() + C1.Center()) / 2;
		}
		void CheckDist()
		{
			float Dist = FVector::DistSquared(C0.Center(), C1.Center());
			if (Dist > 10.f)
			{
				int k = 0;
			}
		}
		FCubicCurve C0;
		FCubicCurve C1;
	};
	TQueue<FCollisionNode> Nodes;
	Nodes.Enqueue(FCollisionNode(*this, Other));
	while (!Nodes.IsEmpty())
	{
		FCollisionNode Node;
		Nodes.Dequeue(Node);
		if (Node.SmallEnough())
		{
			Node.CheckDist();
			return Node.GetCross();
		}
		FCollisionNode N0, N1, N2, N3;
		Node.Split(N0, N1, N2, N3);
		if (N0.Intersect(ProjectMatrix)) Nodes.Enqueue(N0);
		if (N1.Intersect(ProjectMatrix)) Nodes.Enqueue(N1);
		if (N2.Intersect(ProjectMatrix)) Nodes.Enqueue(N2);
		if (N3.Intersect(ProjectMatrix)) Nodes.Enqueue(N3);
	}
	return FVector::ZeroVector;
}
//https://stackoverflow.com/questions/4778147/sample-random-point-in-triangle
FVector RandomPointInTriangle(const FVector& A, const FVector& B, const FVector& C)
{
	float r1 = FMath::Sqrt(FMath::FRand());
	float r2 = FMath::FRand();
	return (1.0f - r1)*A + r1 * (1.0f - r2)*B + r1 * r2*C;
}

float GetTriangleArea(const FVector& v0, const FVector& v1, const FVector& v2)
{
	FVector Normal = (v1 - v0) ^ (v2 - v0);
	return Normal.Size() / 2;
}

FVector2D CalculateCentroid(const TArray<FVector2D>& Points)
{
	FVector2D centroid(0, 0);
	float signedArea = 0.0;
	float x0 = 0.0; // Current vertex X
	float y0 = 0.0; // Current vertex Y
	float x1 = 0.0; // Next vertex X
	float y1 = 0.0; // Next vertex Y
	float a = 0.0;  // Partial signed area
					 // For all vertices except last
	int i = 0;
	for (i = 0; i < Points.Num()-1; i++)
	{
		x0 = Points[i].X;
		y0 = Points[i].Y;
		x1 = Points[i + 1].X;
		y1 = Points[i + 1].Y;
		a = x0 * y1 - x1 * y0;
		signedArea += a;
		centroid.X += (x0 + x1)*a;
		centroid.Y += (y0 + y1)*a;
	}
	// Do last vertex
	x0 = Points[i].X;
	y0 = Points[i].Y;
	x1 = Points[0].X;
	y1 = Points[0].Y;
	a = x0 * y1 - x1 * y0;
	signedArea += a;
	centroid.X += (x0 + x1)*a;
	centroid.Y += (y0 + y1)*a;
	signedArea *= 0.5;
	centroid.X /= (6.0*signedArea);
	centroid.Y /= (6.0*signedArea);
	return centroid;
}