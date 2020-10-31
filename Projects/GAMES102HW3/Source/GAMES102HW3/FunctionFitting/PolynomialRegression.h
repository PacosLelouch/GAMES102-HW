// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FunctionFittingBase.h"

struct GAMES102HW3_API FPolynomialRegressionParams : public FFittingParamsBase
{
	int32 InSpanNum = 5;
	TArray<double> OutCoefficient;
};

/**
 *
 */
class GAMES102HW3_API FPolynomialRegression : public FFunctionFittingBase
{
public:
	FPolynomialRegression();

	virtual ~FPolynomialRegression() override;

	virtual void Fit() override;

	virtual double Evaluate(double X) override;

public:
	FPolynomialRegressionParams Params;
};
