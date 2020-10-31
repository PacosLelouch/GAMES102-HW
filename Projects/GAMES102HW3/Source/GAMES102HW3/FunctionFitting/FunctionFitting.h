#pragma once

#include "PolynomialFitting.h"
#include "GaussianFitting.h"
#include "PolynomialRegression.h"
#include "RidgeRegression.h"
#include "FunctionFitting.generated.h"

UENUM(BlueprintType)
enum class EFunctionFittingMethod : uint8
{
	PolynomialFitting,
	GaussianFitting,
	PolynomialRegression,
	RidgeRegression,
	All,
};
