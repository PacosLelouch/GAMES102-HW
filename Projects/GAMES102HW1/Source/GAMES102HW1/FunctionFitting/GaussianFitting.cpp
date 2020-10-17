// Fill out your copyright notice in the Description page of Project Settings.


#include "GaussianFitting.h"
#include "Eigen/LU"

FGaussianFitting::FGaussianFitting()
	: FFunctionFittingBase()
{
}

FGaussianFitting::~FGaussianFitting()
{

}

void FGaussianFitting::Fit()
{
	int32 Dim = Params.InPoints.Num();
	if (Dim == 0) {
		return;
	}
	Eigen::MatrixXd A(Dim + 1 , Dim + 1);
	Eigen::VectorXd B(Dim + 1), X(Dim + 1);
	
	TTuple<double, double> NewPoint = MakeTuple<double, double>(0, 0);
	for (int32 i = 0; i < Dim; ++i) {
		A(i, 0) = 1;// Params.InPoints[i].Get<0>();
		for (int32 j = 1; j <= Dim; ++j) {
			A(i, j) = GaussianKernel(Params.InPoints[i].Get<0>(), j - 1);
		}
		B(i) = Params.InPoints[i].Get<1>();
		if (Dim > 1) {
			NewPoint.Get<0>() += Params.InPoints[i].Get<0>() / Dim;
			NewPoint.Get<1>() += Params.InPoints[i].Get<1>() / Dim;
		}
	}
	A(Dim, 0) = 1;// NewPoint.Get<0>();
	for (int32 j = 1; j <= Dim; ++j) {
		A(Dim, j) = GaussianKernel(NewPoint.Get<0>(), j - 1);
	}
	B(Dim) = NewPoint.Get<1>();

	X = A.lu().solve(B);
	Params.OutCoefficient.SetNum(X.size());
	for (int32 i = 0; i < X.size(); ++i) {
		Params.OutCoefficient[i] = X(i);
	}
}

double FGaussianFitting::Evaluate(double X)
{
	if (Params.OutCoefficient.Num() == 0) {
		return 0.;
	}
	double Result = Params.OutCoefficient[0];
	for (int32 i = 1; i < Params.OutCoefficient.Num(); ++i) {
		Result += GaussianKernel(X, i - 1) * Params.OutCoefficient[i];
	}
	return Result;
}

double FGaussianFitting::GaussianKernel(double X, int32 i)
{
	double DX = X - Params.InPoints[i].Get<0>();
	return FMath::Exp(-(DX * DX) * 0.5 / (Params.InSigma * Params.InSigma));
};
