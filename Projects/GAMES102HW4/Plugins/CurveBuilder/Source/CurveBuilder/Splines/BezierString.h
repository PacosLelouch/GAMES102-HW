// Copyright 2020 PacosLelouch, Inc. All Rights Reserved.
// https://github.com/PacosLelouch/

#pragma once

#include "SplineBase.h"
#include "Containers/List.h"
#include "Utils/LinearAlgebraUtils.h"
#include "Utils/NumericalCalculationUtils.h"
#include "Curves/BezierCurve.h"
#include "CurveOperations/BezierOperations.h"
#include "Utils/Continuity.h"
THIRD_PARTY_INCLUDES_START
#include "Eigen/LU"
THIRD_PARTY_INCLUDES_END

template<int32 Dim>
struct TBezierString3ControlPoint
{
	TBezierString3ControlPoint() : Pos(TVecLib<Dim+1>::Zero()), PrevCtrlPointPos(TVecLib<Dim+1>::Zero()), NextCtrlPointPos(TVecLib<Dim+1>::Zero()), Param(0.) {}
	TBezierString3ControlPoint(const TVectorX<Dim+1>& InPos, double InParam = 0.) : Pos(InPos), PrevCtrlPointPos(TVecLib<Dim+1>::Zero()), NextCtrlPointPos(TVecLib<Dim+1>::Zero()), Param(InParam) {}
	TBezierString3ControlPoint(const TVectorX<Dim+1>& InPos, const TVectorX<Dim+1>& PrevPos, const TVectorX<Dim+1>& NextPos, double InParam = 0.) : Pos(InPos), PrevCtrlPointPos(PrevPos), NextCtrlPointPos(NextPos), Param(InParam) {}
	TBezierString3ControlPoint(const TBezierString3ControlPoint<Dim>& InP) : Pos(InP.Pos), PrevCtrlPointPos(InP.PrevCtrlPointPos), NextCtrlPointPos(InP.NextCtrlPointPos), Param(InP.Param) {}
	TBezierString3ControlPoint<Dim>& operator=(const TBezierString3ControlPoint<Dim>& InP) { Pos = InP.Pos; PrevCtrlPointPos = InP.PrevCtrlPointPos; NextCtrlPointPos = InP.NextCtrlPointPos; Param = InP.Param; return *this; }

	TVectorX<Dim+1> Pos;
	TVectorX<Dim+1> PrevCtrlPointPos, NextCtrlPointPos;
	double Param;
	EEndPointContinuity Continuity = EEndPointContinuity::G2;
};

// BezierString3
template<int32 Dim>
class TBezierString3 : TSplineBase<Dim>
{
	using TSplineBase<Dim>::TSplineBase;
public:
	using FPointNode = typename TDoubleLinkedList<TBezierString3ControlPoint<Dim> >::TDoubleLinkedListNode;
public:
	FORCEINLINE TBezierString3() {}

	FORCEINLINE TBezierString3(const TBezierString3<Dim>& InSpline);

	FORCEINLINE TBezierString3(const TArray<TBezierCurve<Dim, 3> >& InCurves);

	FORCEINLINE TBezierString3<Dim>& operator=(const TBezierString3<Dim>& InSpline);

	FORCEINLINE void FromCurveArray(const TArray<TBezierCurve<Dim, 3> >& InCurves);

	FORCEINLINE void Reset() { CtrlPointsList.Empty(); }

	FORCEINLINE void RemakeC2() { UpdateBezierString(nullptr); }

	virtual ~TBezierString3() { CtrlPointsList.Empty(); }

	FORCEINLINE int32 GetCtrlPointNum() const
	{
		return CtrlPointsList.Num();
	}

	FORCEINLINE FPointNode* FirstNode() const
	{
		return CtrlPointsList.GetHead();
	}

	FORCEINLINE FPointNode* LastNode() const
	{
		return CtrlPointsList.GetTail();
	}

public:
	FPointNode* FindNodeByParam(double Param, int32 NthNode = 0) const;

	FPointNode* FindNodeGreaterThanParam(double Param, int32 NthNode = 0) const;

	FPointNode* FindNodeByPosition(const TVectorX<Dim>& Point, int32 NthNode = 0, double ToleranceSqr = 1.) const;

	void GetCtrlPoints(TArray<TVectorX<Dim+1> >& CtrlPoints) const;

	void GetCtrlParams(TArray<double>& CtrlParams) const;

	void GetBezierCurves(TArray<TBezierCurve<Dim, 3> >& BezierCurves, TArray<TTuple<double, double> >& ParamRanges) const;

public:

	virtual void Split(TBezierString3<Dim>& OutFirst, TBezierString3<Dim>& OutSecond, double T) const;

	virtual void AddPointAtLast(const TBezierString3ControlPoint<Dim>& PointStruct);

	virtual void AddPointAtFirst(const TBezierString3ControlPoint<Dim>& PointStruct);

	virtual void AddPointAt(const TBezierString3ControlPoint<Dim>& PointStruct, int32 Index = 0);

	virtual FPointNode* AddPointWithParamWithoutChangingShape(double T);

	virtual void AdjustCtrlPointParam(double From, double To, int32 NthPointOfFrom = 0);

	virtual void ChangeCtrlPointContinuous(double From, EEndPointContinuity Continuity, int32 NthPointOfFrom = 0);

	virtual void AdjustCtrlPointTangent(double From, const TVectorX<Dim>& To, bool bNext = true, int32 NthPointOfFrom = 0);

	virtual void AdjustCtrlPointTangent(FPointNode* Node, const TVectorX<Dim>& To, bool bNext = true, int32 NthPointOfFrom = 0);

	virtual void RemovePoint(double Param, int32 NthPointOfFrom = 0);

	virtual void AdjustCtrlPointPos(FPointNode* Node, const TVectorX<Dim>& To, int32 NthPointOfFrom = 0);

public:
	virtual void AddPointAtLast(const TVectorX<Dim>& Point, TOptional<double> Param = TOptional<double>(), double Weight = 1.) override;

	virtual void AddPointAtFirst(const TVectorX<Dim>& Point, TOptional<double> Param = TOptional<double>(), double Weight = 1.) override;

	virtual void AddPointAt(const TVectorX<Dim>& Point, TOptional<double> Param = TOptional<double>(), int32 Index = 0, double Weight = 1.) override;

	virtual void RemovePointAt(int32 Index = 0) override;

	virtual void RemovePoint(const TVectorX<Dim>& Point, int32 NthPointOfFrom = 0) override;

	virtual void AdjustCtrlPointPos(const TVectorX<Dim>& From, const TVectorX<Dim>& To, int32 NthPointOfFrom = 0) override;

	virtual void Reverse() override;

	virtual TVectorX<Dim> GetPosition(double T) const override;

	virtual TVectorX<Dim> GetTangent(double T) const override;

	virtual double GetPrincipalCurvature(double T, int32 Principal = 0) const override;

	virtual double GetCurvature(double T) const override;

	virtual void ToPolynomialForm(TArray<TArray<TVectorX<Dim+1> > >& OutPolyForms) const override;

	virtual TTuple<double, double> GetParamRange() const override;

	virtual bool FindParamByPosition(double& OutParam, const TVectorX<Dim>& InPos, double ToleranceSqr = 1.) const override;

protected:
	TDoubleLinkedList<TBezierString3ControlPoint<Dim> > CtrlPointsList;

	double GetNormalizedParam(const FPointNode* StartNode, const FPointNode* EndNode, double T) const;

	TBezierCurve<Dim, 3> MakeBezierCurve(const FPointNode* StartNode, const FPointNode* EndNode) const;

	void UpdateBezierString(FPointNode* NodeToUpdateFirst = nullptr);

	bool AdjustPointByStaticPointReturnShouldSpread(FPointNode* Node, bool bFromNext = true);
};

#include "BezierString.inl"
