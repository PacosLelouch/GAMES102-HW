// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "FunctionFittingBase.h"

bool FFunctionFittingBase::Sample(TArray<TTuple<double, double> >& OutPoints, double MinX, double MaxX, double Step)
{
	if (MinX > MaxX || Step <= 0) {
		return false;
	}
	TArray<double> Xs;
	CreateUniformXs(Xs, MinX, MaxX, Step);
	for (double X : Xs) {
		OutPoints.Emplace(X, Evaluate(X));
	}
	return true;
}

void FFunctionFittingBase::CreateUniformXs(TArray<double>& Xs, double MinX, double MaxX, double Step)
{
	double Range = MaxX - MinX;
	//if (Range < 0 || Step <= 0) {
	//	return;
	//}
	int32 StepCount = FMath::CeilToInt(Range / Step), XCount = StepCount + 1;
	Xs.Empty(XCount);
	double RealStep = Range / static_cast<double>(StepCount);
	double Current = MinX;
	for (int32 i = 0; i < XCount; ++i) {
		Xs.Add(Current);
		Current += RealStep;
	}
}
