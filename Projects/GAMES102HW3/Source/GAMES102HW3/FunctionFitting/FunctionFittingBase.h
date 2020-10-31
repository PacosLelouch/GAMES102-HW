// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

struct GAMES102HW3_API FFittingParamsBase
{
	TArray<TTuple<double, double> > InPoints;
};

/**
 *
 */
class GAMES102HW3_API FFunctionFittingBase
{
public:
	virtual ~FFunctionFittingBase() {}

	virtual void Fit() = 0;

	virtual double Evaluate(double X) = 0;

	virtual bool Sample(TArray<TTuple<double, double> >& OutPoints, double MinX, double MaxX, double Step);

	virtual void CreateUniformXs(TArray<double>& Xs, double MinX, double MaxX, double Step);
};