// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FunctionFittingBase.h"

struct GAMES102HW3_API FGaussianFittingParams : public FFittingParamsBase
{
	double InSigma = 1.;
	TArray<double> OutCoefficient;
};

/**
 *
 */
class GAMES102HW3_API FGaussianFitting : public FFunctionFittingBase
{
public:
	FGaussianFitting();

	virtual ~FGaussianFitting() override;

	virtual void Fit() override;

	virtual double Evaluate(double X) override;

	double GaussianKernel(double x, int32 i);

public:
	FGaussianFittingParams Params;
};
