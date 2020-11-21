// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

struct GAMES102HW5_API FSubdivisionParamsBase
{
	TArray<TTuple<double, double> > InPoints;
	TArray<TTuple<double, double> > OutPoints;
	bool bInClosed = true;
};

class GAMES102HW5_API FSubdivisionBase
{
public:
	virtual ~FSubdivisionBase() {}

	virtual void Subdivide(int32 Times = 2) = 0;

	TUniquePtr<FSubdivisionParamsBase> Params;
};
