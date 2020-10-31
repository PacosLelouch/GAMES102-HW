// Fill out your copyright notice in the Description page of Project Settings.


#include "RidgeRegression.h"
#include "Eigen/LU"

FRidgeRegression::FRidgeRegression()
	: FFunctionFittingBase()
{
}

FRidgeRegression::~FRidgeRegression()
{
}

void FRidgeRegression::Fit()
{
	int32 Num = Params.InPoints.Num();
	if (Num == 0) {
		return;
	}
	int32 Dim = FMath::Min(Params.InSpanNum, Params.InPoints.Num());
	Eigen::MatrixXd A(Num, Dim), I = Eigen::MatrixXd::Identity(Dim, Dim);
	Eigen::VectorXd B(Num), X(Num);
	for (int32 i = 0; i < Dim; ++i) {
		I(i, i) = 1;
	}
	for (int32 i = 0; i < Num; ++i) {
		double CurEle = 1;
		for (int32 j = 0; j < Dim; ++j) {
			A(i, j) = CurEle;
			CurEle *= Params.InPoints[i].Get<0>();
		}
		B(i) = Params.InPoints[i].Get<1>();
	}
	Eigen::MatrixXd AT = A.transpose();
	X = (AT*A + Params.InLambda*I).lu().solve(AT*B);
	Params.OutCoefficient.SetNum(X.size());
	for (int32 i = 0; i < X.size(); ++i) {
		Params.OutCoefficient[i] = X(i);
	}
}

double FRidgeRegression::Evaluate(double X)
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
