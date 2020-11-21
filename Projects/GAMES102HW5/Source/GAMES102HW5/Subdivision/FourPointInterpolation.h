// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SubdivisionBase.h"

struct GAMES102HW5_API FFourPointInterpolationParams : public FSubdivisionParamsBase
{
	double InAlpha = 0.0625;
};

class GAMES102HW5_API FFourPointInterpolation : public FSubdivisionBase
{
public:
	FFourPointInterpolation()
	{
		Params = MakeUnique<FFourPointInterpolationParams>();
	}

	virtual ~FFourPointInterpolation() {}

	virtual void Subdivide(int32 Times = 2) override
	{
		FFourPointInterpolationParams& ParamsRef = *static_cast<FFourPointInterpolationParams*>(Params.Get());
		ParamsRef.OutPoints.Empty(ParamsRef.InPoints.Num() << Times);
		ParamsRef.OutPoints.SetNumZeroed(ParamsRef.InPoints.Num() << Times);
		FMemory::Memcpy(ParamsRef.OutPoints.GetData(), ParamsRef.InPoints.GetData(), ParamsRef.InPoints.Num() * sizeof(TTuple<double, double>));
		int32 Num = ParamsRef.InPoints.Num();
		if (Num == 0)
		{
			return;
		}

		double Alpha = Num <= 2 ? 0. : ParamsRef.InAlpha;

		for (int32 t = 0; t < Times; ++t)
		{
			auto TempPointPrev = ParamsRef.OutPoints[Num - 1];
			for (int32 i = Num - 1; i >= 0; --i)
			{
				if ((i && i + 1 < Num && !ParamsRef.bInClosed) || ParamsRef.bInClosed)
				{
					int32 NextI = (i + 1 == Num) ? (ParamsRef.bInClosed ? 0 : Num - 1) : i + 1;
					auto PrevPoint = (i == 0) ? (ParamsRef.bInClosed ? TempPointPrev : ParamsRef.OutPoints[0]) : ParamsRef.OutPoints[i - 1];
					auto Average = MakeTuple(
						ParamsRef.OutPoints[i].Get<0>() * 0.5 + ParamsRef.OutPoints[NextI].Get<0>() * 0.5,
						ParamsRef.OutPoints[i].Get<1>() * 0.5 + ParamsRef.OutPoints[NextI].Get<1>() * 0.5);

					if ((i + 2 < Num && !ParamsRef.bInClosed) || ParamsRef.bInClosed)
					{
						int32 NNI = (i + 2 >= Num) ? (ParamsRef.bInClosed ? i + 2 - Num : Num - 1) : i + 2;
						auto Average2 = MakeTuple(
							PrevPoint.Get<0>() * 0.5 + ParamsRef.OutPoints[NNI].Get<0>() * 0.5,
							PrevPoint.Get<1>() * 0.5 + ParamsRef.OutPoints[NNI].Get<1>() * 0.5);
						ParamsRef.OutPoints[(i << 1) + 1] = MakeTuple(
							Average.Get<0>() + (Average.Get<0>() - Average2.Get<0>()) * Alpha,
							Average.Get<1>() + (Average.Get<1>() - Average2.Get<1>()) * Alpha);
					}
					else
					{
						ParamsRef.OutPoints[(i << 1) + 1] = MakeTuple(
							Average.Get<0>() * (1. + Alpha),
							Average.Get<1>() * (1. + Alpha));
					}
				}
				else
				{
					ParamsRef.OutPoints[(i << 1) + 1] = ParamsRef.OutPoints[i];
				}
				ParamsRef.OutPoints[(i << 1)] = ParamsRef.OutPoints[i];
			}
			Num <<= 1;
		}
	}
};