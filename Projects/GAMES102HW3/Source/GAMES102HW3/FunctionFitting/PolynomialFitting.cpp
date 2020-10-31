// Fill out your copyright notice in the Description page of Project Settings.


#include "PolynomialFitting.h"
#include "Eigen/LU"

FPolynomialFitting::FPolynomialFitting()
	: FFunctionFittingBase()
{
}

FPolynomialFitting::~FPolynomialFitting()
{

}

void FPolynomialFitting::Fit()
{
	int32 Dim = Params.InPoints.Num();
	if (Dim == 0) {
		return;
	}
	Eigen::MatrixXd A(Dim, Dim);
	Eigen::VectorXd B(Dim), X(Dim);
	for (int32 i = 0; i < Dim; ++i) {
		double CurEle = 1;
		for (int32 j = 0; j < Dim; ++j) {
			A(i, j) = CurEle;
			CurEle *= Params.InPoints[i].Get<0>();
		}
		B(i) = Params.InPoints[i].Get<1>();
	}
	X = A.lu().solve(B);
	Params.OutCoefficient.SetNum(X.size());
	for (int32 i = 0; i < X.size(); ++i) {
		Params.OutCoefficient[i] = X(i);
	}
}

double FPolynomialFitting::Evaluate(double X)
{
	if (Params.OutCoefficient.Num() == 0) {
		return 0.;
	}
	double Result = Params.OutCoefficient[0];
	double CurEle = X;
	for (int32 i = 1; i < Params.OutCoefficient.Num(); ++i) {
		Result += CurEle * Params.OutCoefficient[i];
		CurEle *= X;
	}
	return Result;
}
