// Fill out your copyright notice in the Description page of Project Settings.


#pragma once

#include "CoreMinimal.h"
#include "Parametrization.generated.h"

UENUM(BlueprintType)
enum class EParametrizationMethod : uint8
{
	Uniform,
	Chordal,
	Centripetal,
	Foley,
	All,
};

// Start Base

struct GAMES102HW3_API FParametrizationParamsBase
{
	TArray<TTuple<double, double> > OutPointsParamX;
	TArray<TTuple<double, double> > OutPointsParamY;
	TArray<TTuple<double, double> > InPointsXY;
};

class GAMES102HW3_API FParametrizationBase
{
public:
	virtual ~FParametrizationBase() {}
	virtual void Parametrize(double Start = 0., double End = 1.) = 0;
};

struct GAMES102HW3_API FUniformParametrizationParams : public FParametrizationParamsBase
{
};

// End Base

// Start Uniform

class GAMES102HW3_API FUniformParametrization : public FParametrizationBase
{
public:
	virtual ~FUniformParametrization() {}
	virtual void Parametrize(double Start = 0., double End = 1.) override
	{
		Params.OutPointsParamX.Reserve(Params.InPointsXY.Num());
		Params.OutPointsParamY.Reserve(Params.InPointsXY.Num());
		int32 SegNum = Params.InPointsXY.Num() - 1;
		for (int32 i = 0; i < Params.InPointsXY.Num(); ++i) {
			double Alpha = SegNum == 0 ? 0. : static_cast<double>(i) / SegNum;
			double T = Start + (End - Start) * Alpha;
			Params.OutPointsParamX.Emplace(MakeTuple(T, Params.InPointsXY[i].Get<0>()));
			Params.OutPointsParamY.Emplace(MakeTuple(T, Params.InPointsXY[i].Get<1>()));
		}
	}
public:
	FUniformParametrizationParams Params;
};

// End Uniform

// Start Chordal

struct GAMES102HW3_API FChordalParametrizationParams : public FParametrizationParamsBase
{
};

class GAMES102HW3_API FChordalParametrization : public FParametrizationBase
{
public:
	virtual ~FChordalParametrization() {}
	virtual void Parametrize(double Start = 0., double End = 1.) override
	{
		Params.OutPointsParamX.Reserve(Params.InPointsXY.Num());
		Params.OutPointsParamY.Reserve(Params.InPointsXY.Num());
		double TotalS = 0.;
		for (int32 i = 0; i < Params.InPointsXY.Num(); ++i) {
			if (i > 0) {
				const auto& CurPoint = Params.InPointsXY[i];
				const auto& PrevPoint = Params.InPointsXY[i - 1];
				double DX = CurPoint.Get<0>() - PrevPoint.Get<0>();
				double DY = CurPoint.Get<1>() - PrevPoint.Get<1>();
				double Dist = sqrt(DX * DX + DY * DY);
				TotalS += Dist;
			}
			Params.OutPointsParamX.Emplace(MakeTuple(TotalS, Params.InPointsXY[i].Get<0>()));
			Params.OutPointsParamY.Emplace(MakeTuple(TotalS, Params.InPointsXY[i].Get<1>()));
		}
		double InvTotalS = FMath::IsNearlyZero(TotalS) ? 1. : 1. / TotalS;
		for (int32 i = 0; i < Params.InPointsXY.Num(); ++i) {
			Params.OutPointsParamX[i].Get<0>() *= InvTotalS;
			Params.OutPointsParamX[i].Get<0>() = Params.OutPointsParamX[i].Get<0>() * (End - Start) + Start;
			Params.OutPointsParamY[i].Get<0>() *= InvTotalS;
			Params.OutPointsParamY[i].Get<0>() = Params.OutPointsParamY[i].Get<0>() * (End - Start) + Start;
		}
	}
public:
	FChordalParametrizationParams Params;
};

// End Chordal

// Start Centripetal

struct GAMES102HW3_API FCentripetalParametrizationParams : public FParametrizationParamsBase
{
};

class GAMES102HW3_API FCentripetalParametrization : public FParametrizationBase
{
public:
	virtual ~FCentripetalParametrization() {}
	virtual void Parametrize(double Start = 0., double End = 1.) override
	{
		Params.OutPointsParamX.Reserve(Params.InPointsXY.Num());
		Params.OutPointsParamY.Reserve(Params.InPointsXY.Num());
		double TotalS = 0.;
		for (int32 i = 0; i < Params.InPointsXY.Num(); ++i) {
			if (i > 0) {
				const auto& CurPoint = Params.InPointsXY[i];
				const auto& PrevPoint = Params.InPointsXY[i - 1];
				double DX = CurPoint.Get<0>() - PrevPoint.Get<0>();
				double DY = CurPoint.Get<1>() - PrevPoint.Get<1>();
				double Dist = sqrt(DX * DX + DY * DY);
				TotalS += sqrt(Dist);
			}
			Params.OutPointsParamX.Emplace(MakeTuple(TotalS, Params.InPointsXY[i].Get<0>()));
			Params.OutPointsParamY.Emplace(MakeTuple(TotalS, Params.InPointsXY[i].Get<1>()));
		}
		double InvTotalS = FMath::IsNearlyZero(TotalS) ? 1. : 1. / TotalS;
		for (int32 i = 0; i < Params.InPointsXY.Num(); ++i) {
			Params.OutPointsParamX[i].Get<0>() *= InvTotalS;
			Params.OutPointsParamX[i].Get<0>() = Params.OutPointsParamX[i].Get<0>() * (End - Start) + Start;
			Params.OutPointsParamY[i].Get<0>() *= InvTotalS;
			Params.OutPointsParamY[i].Get<0>() = Params.OutPointsParamY[i].Get<0>() * (End - Start) + Start;
		}
	}
public:
	FCentripetalParametrizationParams Params;
};

// End Centripetal

// Start Foley

struct GAMES102HW3_API FFoleyParametrizationParams : public FParametrizationParamsBase
{
};

class GAMES102HW3_API FFoleyParametrization : public FParametrizationBase
{
public:
	virtual ~FFoleyParametrization() {}
	virtual void Parametrize(double Start = 0., double End = 1.) override
	{
		static const double PI_Dbl = acos(-1);
		static const double HALF_PI_Dbl = asin(1);
		Params.OutPointsParamX.Reserve(Params.InPointsXY.Num());
		Params.OutPointsParamY.Reserve(Params.InPointsXY.Num());
		TArray<double> Dists;
		Dists.Reserve(Params.InPointsXY.Num() > 0 ? Params.InPointsXY.Num() - 1 : 0);
		for (int32 i = 0; i + 1 < Params.InPointsXY.Num(); ++i) {
			const auto& NextPoint = Params.InPointsXY[i + 1];
			const auto& CurPoint = Params.InPointsXY[i];
			double DX = NextPoint.Get<0>() - CurPoint.Get<0>();
			double DY = NextPoint.Get<1>() - CurPoint.Get<1>();
			double Dist = sqrt(DX * DX + DY * DY);
			Dists.Add(Dist);
		}
		double TotalS = 0.;
		for (int32 i = 0; i < Params.InPointsXY.Num(); ++i) {
			const auto& CurPoint = Params.InPointsXY[i];
			double DS = 0.;
			if (i > 0) {
				const auto& PrevPoint = Params.InPointsXY[i - 1];
				DS += Dists[i - 1];
				if (i > 1) {
					const auto& PPPoint = Params.InPointsXY[i - 2];
					TTuple<double, double> PrevDir = MakeTuple(
						(PrevPoint.Get<0>() - PPPoint.Get<0>()) / Dists[i - 2],
						(PrevPoint.Get<1>() - PPPoint.Get<1>()) / Dists[i - 2]);
					TTuple<double, double> CurDir = MakeTuple(
						(CurPoint.Get<0>() - PrevPoint.Get<0>()) / Dists[i - 1],
						(CurPoint.Get<1>() - PrevPoint.Get<1>()) / Dists[i - 1]);
					double CurAngle = FMath::Min(
						HALF_PI_Dbl,
						PI_Dbl - acos(PrevDir.Get<0>() * CurDir.Get<0>() + PrevDir.Get<1>() * CurDir.Get<1>())
						);
					DS += 1.5 * (Dists[i - 2] * Dists[i - 1]) / (Dists[i - 2] + Dists[i - 1]);
				}
				if (i + 1 < Params.InPointsXY.Num()) {
					const auto& NextPoint = Params.InPointsXY[i + 1];
					TTuple<double, double> CurDir = MakeTuple(
						(CurPoint.Get<0>() - PrevPoint.Get<0>()) / Dists[i - 1],
						(CurPoint.Get<1>() - PrevPoint.Get<1>()) / Dists[i - 1]);
					TTuple<double, double> NextDir = MakeTuple(
						(NextPoint.Get<0>() - CurPoint.Get<0>()) / Dists[i],
						(NextPoint.Get<1>() - CurPoint.Get<1>()) / Dists[i]);
					double NextAngle = FMath::Min(
						HALF_PI_Dbl,
						PI_Dbl - acos(NextPoint.Get<0>() * CurDir.Get<0>() + NextPoint.Get<1>() * CurDir.Get<1>())
					);
					DS += 1.5 * (Dists[i] * Dists[i - 1]) / (Dists[i] + Dists[i - 1]);
				}
			}
			TotalS += DS;
			Params.OutPointsParamX.Emplace(MakeTuple(TotalS, CurPoint.Get<0>()));
			Params.OutPointsParamY.Emplace(MakeTuple(TotalS, CurPoint.Get<1>()));
		}
		double InvTotalS = FMath::IsNearlyZero(TotalS) ? 1. : 1. / TotalS;
		for (int32 i = 0; i < Params.InPointsXY.Num(); ++i) {
			Params.OutPointsParamX[i].Get<0>() *= InvTotalS;
			Params.OutPointsParamX[i].Get<0>() = Params.OutPointsParamX[i].Get<0>() * (End - Start) + Start;
			Params.OutPointsParamY[i].Get<0>() *= InvTotalS;
			Params.OutPointsParamY[i].Get<0>() = Params.OutPointsParamY[i].Get<0>() * (End - Start) + Start;
		}
	}
public:
	FFoleyParametrizationParams Params;
};

// End Foley
