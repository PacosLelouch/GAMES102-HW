// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SubdivisionBase.h"

struct GAMES102HW6_API FThreeDegreeBSplineParams : public FSubdivisionParamsBase
{
};

class GAMES102HW6_API FThreeDegreeBSpline : public FSubdivisionBase
{
public:
	FThreeDegreeBSpline()
	{
		Params = MakeUnique<FThreeDegreeBSplineParams>();
	}

	virtual ~FThreeDegreeBSpline() {}

	virtual void Subdivide(int32 Times = 2) override
	{
		FThreeDegreeBSplineParams& ParamsRef = *static_cast<FThreeDegreeBSplineParams*>(Params.Get());
		ParamsRef.OutPoints.Empty(ParamsRef.InPoints.Num() << Times);
		ParamsRef.OutPoints.SetNumZeroed(ParamsRef.InPoints.Num() << Times);
		FMemory::Memcpy(ParamsRef.OutPoints.GetData(), ParamsRef.InPoints.GetData(), ParamsRef.InPoints.Num() * sizeof(TTuple<double, double>));
		int32 Num = ParamsRef.InPoints.Num();
		if (Num == 0)
		{
			return;
		}
		for (int32 t = 0; t < Times; ++t)
		{
			TTuple<double, double> TempPointPrev = ParamsRef.OutPoints[Num - 1];
			for (int32 i = Num - 1; i >= 0; --i)
			{
				if ((i + 1 < Num && !ParamsRef.bInClosed) || ParamsRef.bInClosed)
				{
					int32 NextI = (i == Num - 1) ? 0 : i + 1;
					ParamsRef.OutPoints[(i << 1) + 1] = MakeTuple(
						ParamsRef.OutPoints[i].Get<0>() * 0.5 + ParamsRef.OutPoints[NextI].Get<0>() * 0.5,
						ParamsRef.OutPoints[i].Get<1>() * 0.5 + ParamsRef.OutPoints[NextI].Get<1>() * 0.5);
				}
				else
				{
					ParamsRef.OutPoints[(i << 1) + 1] = ParamsRef.OutPoints[i];
				}

				if ((i && !ParamsRef.bInClosed) || ParamsRef.bInClosed)
				{
					int32 NextI = (i == Num - 1) ? (ParamsRef.bInClosed ? 0 : Num - 1) : i + 1;
					if (i == 0)
					{
						ParamsRef.OutPoints[(i << 1)] = MakeTuple(
							TempPointPrev.Get<0>() * 0.125 + ParamsRef.OutPoints[i].Get<0>() * 0.75 + ParamsRef.OutPoints[NextI].Get<0>() * 0.125,
							TempPointPrev.Get<1>() * 0.125 + ParamsRef.OutPoints[i].Get<1>() * 0.75 + ParamsRef.OutPoints[NextI].Get<1>() * 0.125);
					}
					else
					{
						ParamsRef.OutPoints[(i << 1)] = MakeTuple(
							ParamsRef.OutPoints[i - 1].Get<0>() * 0.125 + ParamsRef.OutPoints[i].Get<0>() * 0.75 + ParamsRef.OutPoints[NextI].Get<0>() * 0.125,
							ParamsRef.OutPoints[i - 1].Get<1>() * 0.125 + ParamsRef.OutPoints[i].Get<1>() * 0.75 + ParamsRef.OutPoints[NextI].Get<1>() * 0.125);
					}
				}
				else
				{
					ParamsRef.OutPoints[(i << 1)] = ParamsRef.OutPoints[i];
				}
			}
			Num <<= 1;
		}
	}
};