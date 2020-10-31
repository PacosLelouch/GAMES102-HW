// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FunctionFittingBase.h"

struct GAMES102HW3_API FRidgeRegressionParams : public FFittingParamsBase
{
	int32 InSpanNum = 5;
	double InLambda = 1;
	TArray<double> OutCoefficient;
};

/**
 *
 */
class GAMES102HW3_API FRidgeRegression : public FFunctionFittingBase
{
public:
	FRidgeRegression();

	virtual ~FRidgeRegression() override;

	virtual void Fit() override;

	virtual double Evaluate(double X) override;

public:
	FRidgeRegressionParams Params;
};
