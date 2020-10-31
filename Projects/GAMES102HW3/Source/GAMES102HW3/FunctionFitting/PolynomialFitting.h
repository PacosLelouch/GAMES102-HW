// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FunctionFittingBase.h"

struct GAMES102HW3_API FPolynomialFittingParams : public FFittingParamsBase
{
	TArray<double> OutCoefficient;
};

/**
 * 
 */
class GAMES102HW3_API FPolynomialFitting : public FFunctionFittingBase
{
public:
	FPolynomialFitting();

	virtual ~FPolynomialFitting() override;

	virtual void Fit() override;

	virtual double Evaluate(double X) override;
	
public:
	FPolynomialFittingParams Params;
};
