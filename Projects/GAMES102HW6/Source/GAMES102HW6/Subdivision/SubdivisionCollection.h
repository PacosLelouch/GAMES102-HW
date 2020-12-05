// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "SubdivisionBase.h"
#include "Chaikin.h"
#include "ThreeDegreeBSpline.h"
#include "FourPointInterpolation.h"
#include "SubdivisionCollection.generated.h"


UENUM(BlueprintType)
enum class ESubdivisionMethod : uint8
{
	Chaikin,
	ThreeDegreeBSpline,
	FourPointInterpolation,
	All,
};

