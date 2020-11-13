// Copyright 2020 PacosLelouch, Inc. All Rights Reserved.
// https://github.com/PacosLelouch/

#pragma once

#include "BezierString.h"

template<int32 Dim>
inline TBezierString3<Dim>::TBezierString3(const TBezierString3<Dim>& InSpline)
{
	for (const auto& Pos : InSpline.CtrlPointsList) {
		CtrlPointsList.AddTail(Pos);
	}
}

template<int32 Dim>
inline TBezierString3<Dim>::TBezierString3(const TArray<TBezierCurve<Dim, 3>>& InCurves)
{
	FromCurveArray(InCurves);
}

template<int32 Dim>
inline TBezierString3<Dim>& TBezierString3<Dim>::operator=(const TBezierString3<Dim>& InSpline)
{
	CtrlPointsList.Empty();
	for (const auto& Pos : InSpline.CtrlPointsList) {
		CtrlPointsList.AddTail(Pos);
	}
	return *this;
}

template<int32 Dim>
inline void TBezierString3<Dim>::FromCurveArray(const TArray<TBezierCurve<Dim, 3>>& InCurves)
{
	CtrlPointsList.Empty();
	for (int32 i = 0; i < InCurves.Num(); ++i) {
		TVectorX<Dim+1> PrevCtrlPointPos = TVecLib<Dim>::Homogeneous(InCurves[i].GetPoint(0) * 2. - InCurves[i].GetPoint(1), 1.);
		CtrlPointsList.AddTail(TBezierString3ControlPoint<Dim>(
			InCurves[i].GetPointHomogeneous(0),
			PrevCtrlPointPos,
			InCurves[i].GetPointHomogeneous(1),
			static_cast<double>(i)));
		if (i > 0) {
			if (!TVecLib<Dim+1>::IsNearlyZero(InCurves[i].GetPointHomogeneous(0) - InCurves[i - 1].GetPointHomogeneous(3))) {
				Reset();
				return;
			}
			CtrlPointsList.GetTail()->GetValue().PrevCtrlPointPos = InCurves[i - 1].GetPointHomogeneous(2);
		}
	}
	if (InCurves.Num() > 0) {
		TVectorX<Dim+1> NextCtrlPointPos = TVecLib<Dim>::Homogeneous(InCurves[InCurves.Num() - 1].GetPoint(3) * 2. - InCurves[InCurves.Num() - 1].GetPoint(2), 1.);
		CtrlPointsList.AddTail(TBezierString3ControlPoint<Dim>(
			InCurves[InCurves.Num() - 1].GetPointHomogeneous(3),
			InCurves[InCurves.Num() - 1].GetPointHomogeneous(2),
			NextCtrlPointPos,
			static_cast<double>(InCurves.Num())));
	}
}

template<int32 Dim>
inline typename typename TBezierString3<Dim>::FPointNode* TBezierString3<Dim>::FindNodeByParam(double Param, int32 NthNode) const
{
	int32 Count = 0;
	FPointNode* Node = CtrlPointsList.GetHead();
	while (Node) {
		if (FMath::IsNearlyEqual(Node->GetValue().Param, Param)) {
			if (Count == NthNode) {
				return Node;
			}
			++Count;
		}
		Node = Node->GetNextNode();
	}
	return nullptr;
}

template<int32 Dim>
inline typename TBezierString3<Dim>::FPointNode* TBezierString3<Dim>::FindNodeGreaterThanParam(double Param, int32 NthNode) const
{
	int32 Count = 0;
	FPointNode* Node = CtrlPointsList.GetHead();
	while (Node) {
		if (Node->GetValue().Param > Param) {
			if (Count == NthNode) {
				return Node;
			}
			++Count;
		}
		Node = Node->GetNextNode();
	}
	return nullptr;
}

template<int32 Dim>
inline typename typename TBezierString3<Dim>::FPointNode* TBezierString3<Dim>::FindNodeByPosition(const TVectorX<Dim>& Point, int32 NthNode, double ToleranceSqr) const
{
	int32 Count = 0;
	FPointNode* Node = CtrlPointsList.GetHead();
	while (Node) {
		if (TVecLib<Dim>::SizeSquared(TVecLib<Dim+1>::Projection(Node->GetValue().Pos) - Point) < ToleranceSqr) {
			if (Count == NthNode) {
				return Node;
			}
			++Count;
		}
		Node = Node->GetNextNode();
	}
	return nullptr;
}

template<int32 Dim>
inline void TBezierString3<Dim>::GetCtrlPoints(TArray<TVectorX<Dim+1>>& CtrlPoints) const
{
	CtrlPoints.Empty(CtrlPointsList.Num());
	FPointNode* Node = CtrlPointsList.GetHead();
	while (Node) {
		CtrlPoints.Add(Node->GetValue().Pos);
		Node = Node->GetNextNode();
	}
}

template<int32 Dim>
inline void TBezierString3<Dim>::GetCtrlParams(TArray<double>& CtrlParams) const
{
	CtrlParams.Empty(CtrlPointsList.Num());
	FPointNode* Node = CtrlPointsList.GetHead();
	while (Node) {
		CtrlParams.Add(Node->GetValue().Param);
		Node = Node->GetNextNode();
	}
}

template<int32 Dim>
inline void TBezierString3<Dim>::GetBezierCurves(TArray<TBezierCurve<Dim, 3> >& BezierCurves, TArray<TTuple<double, double> >& ParamRanges) const
{
	FPointNode* Node = CtrlPointsList.GetHead();
	if (!Node) {
		return;
	}
	BezierCurves.Empty(CtrlPointsList.Num() - 1);
	ParamRanges.Empty(CtrlPointsList.Num() - 1);
	FPointNode* NextNode = Node->GetNextNode();
	while (Node && NextNode) {
		const auto& NodeVal = Node->GetValue();
		const auto& NextNodeVal = NextNode->GetValue();
		BezierCurves.Emplace({ NodeVal.Pos, NodeVal.NextCtrlPointPos, NextNodeVal.PrevCtrlPointPos, NextNodeVal.Pos });
		ParamRanges.Emplace(MakeTuple(NodeVal.Param, NextNodeVal.Param));
		Node = Node->GetNextNode();
		NextNode = Node->GetNextNode();
	}
}

template<int32 Dim>
inline void TBezierString3<Dim>::Split(TBezierString3<Dim>& OutFirst, TBezierString3<Dim>& OutSecond, double T) const
{
	OutFirst.Reset();
	OutSecond.Reset();
	TTuple<double, double> ParamRange = GetParamRange();
	if (T >= ParamRange.Get<1>() || T <= ParamRange.Get<0>()) {
		return;
	}

	FPointNode* Node = CtrlPointsList.GetHead();
	TArray<TVectorX<Dim+1> > SplitCurveCtrlPoints;
	SplitCurveCtrlPoints.Reserve(4);
	double SplitT = -1.;
	while (Node) {
		if (Node->GetValue().Param <= T) {
			auto Val = Node->GetValue();
			Val.Param = static_cast<double>(OutFirst.GetCtrlPointNum());
			OutFirst.AddPointAtLast(Val);
			FPointNode* NextNode = Node->GetNextNode();
			if (NextNode && T < NextNode->GetValue().Param) {
				SplitCurveCtrlPoints.Add(Val.Pos);
				SplitCurveCtrlPoints.Add(Val.NextCtrlPointPos);
				SplitCurveCtrlPoints.Add(NextNode->GetValue().PrevCtrlPointPos);
				SplitCurveCtrlPoints.Add(NextNode->GetValue().Pos);
				SplitT = GetNormalizedParam(Node, NextNode, T);
			}
		}
		else {
			auto Val = Node->GetValue();
			Val.Param = static_cast<double>(OutSecond.GetCtrlPointNum() + 1);
			OutSecond.AddPointAtLast(Val);
		}
		Node = Node->GetNextNode();
	}
	if (SplitCurveCtrlPoints.Num() != 4) {
		return;
	}

	TBezierCurve<Dim, 3> NewLeft, NewRight;
	TBezierCurve<Dim, 3> SplitCurve(SplitCurveCtrlPoints);
	TVectorX<Dim+1> SplitPos = SplitCurve.Split(NewLeft, NewRight, SplitT);

	OutFirst.LastNode()->GetValue().NextCtrlPointPos = NewLeft.GetPointHomogeneous(1);
	OutSecond.FirstNode()->GetValue().PrevCtrlPointPos = NewRight.GetPointHomogeneous(2);

	TBezierString3ControlPoint<Dim> Val(
		SplitPos, 
		NewLeft.GetPointHomogeneous(2),
		NewRight.GetPointHomogeneous(1),
		static_cast<double>(OutFirst.GetCtrlPointNum()));
	OutFirst.AddPointAtLast(Val);
	Val.Param = 0.;
	OutSecond.AddPointAtFirst(Val);
}

template<int32 Dim>
inline void TBezierString3<Dim>::AddPointAtLast(const TBezierString3ControlPoint<Dim>& PointStruct)
{
	CtrlPointsList.AddTail(PointStruct);
}

template<int32 Dim>
inline void TBezierString3<Dim>::AddPointAtFirst(const TBezierString3ControlPoint<Dim>& PointStruct)
{
	CtrlPointsList.AddHead(PointStruct);
}

template<int32 Dim>
inline void TBezierString3<Dim>::AddPointAt(const TBezierString3ControlPoint<Dim>& PointStruct, int32 Index)
{
	FPointNode* NodeToInsertBefore = CtrlPointsList.GetHead();
	for (int32 i = 0; i < Index; ++i) {
		if (NodeToInsertBefore) {
			NodeToInsertBefore = NodeToInsertBefore->GetNextNode();
		}
		else {
			break;
		}
	}
	if (NodeToInsertBefore) {
		CtrlPointsList.InsertNode(PointStruct, NodeToInsertBefore);
	}
	else {
		CtrlPointsList.AddTail(PointStruct);
	}
}

template<int32 Dim>
inline typename TBezierString3<Dim>::FPointNode* TBezierString3<Dim>::AddPointWithParamWithoutChangingShape(double T)
{
	TTuple<double, double> ParamRange = GetParamRange();
	if (T >= ParamRange.Get<1>() || T <= ParamRange.Get<0>()) {
		return nullptr;
	}

	FPointNode* Node = CtrlPointsList.GetHead();
	FPointNode* NodeToInsertBefore = nullptr;
	TArray<TVectorX<Dim+1> > SplitCurveCtrlPoints;
	SplitCurveCtrlPoints.Reserve(4);
	while (Node) {
		if (Node->GetValue().Param <= T) {
			const auto& Val = Node->GetValue();
			//Val.Param = static_cast<double>(OutFirst.GetCtrlPointNum());
			//OutFirst.AddPointAtLast(Val);
			FPointNode* NextNode = Node->GetNextNode();
			if (NextNode && T < NextNode->GetValue().Param) {
				NodeToInsertBefore = NextNode;
				SplitCurveCtrlPoints.Add(Val.Pos);
				SplitCurveCtrlPoints.Add(Val.NextCtrlPointPos);
				SplitCurveCtrlPoints.Add(NextNode->GetValue().PrevCtrlPointPos);
				SplitCurveCtrlPoints.Add(NextNode->GetValue().Pos);
				//SplitT = (T - Node->GetValue().Param) / (NextNode->GetValue().Param - Node->GetValue().Param);
				break;
			}
		}
		//else {
		//	auto Val = Node->GetValue();
		//	Val.Param = static_cast<double>(OutSecond.GetCtrlPointNum() + 1);
		//	OutSecond.AddPointAtLast(Node->GetValue());
		//}
		Node = Node->GetNextNode();
	}
	if (SplitCurveCtrlPoints.Num() != 4 || !NodeToInsertBefore) {
		return nullptr;
	}
	FPointNode* NodeToInsertAfter = NodeToInsertBefore->GetPrevNode();

	TBezierCurve<Dim, 3> NewLeft, NewRight;
	TBezierCurve<Dim, 3> SplitCurve(SplitCurveCtrlPoints);
	double TN = GetNormalizedParam(NodeToInsertAfter, NodeToInsertBefore, T);
	TVectorX<Dim+1> SplitPos = SplitCurve.Split(NewLeft, NewRight, TN);

	NodeToInsertAfter->GetValue().NextCtrlPointPos = NewLeft.GetPointHomogeneous(1);
	NodeToInsertBefore->GetValue().PrevCtrlPointPos = NewRight.GetPointHomogeneous(2);

	TBezierString3ControlPoint<Dim> Val(
		SplitPos,
		NewLeft.GetPointHomogeneous(2),
		NewRight.GetPointHomogeneous(1),
		T);
	CtrlPointsList.InsertNode(Val, NodeToInsertBefore);
	return NodeToInsertBefore->GetPrevNode();
}

template<int32 Dim>
inline void TBezierString3<Dim>::AdjustCtrlPointParam(double From, double To, int32 NthPointOfFrom)
{
	FPointNode* Node = FindNodeByParam(From, NthPointOfFrom);
	Node->GetValue().Param = To;
	UpdateBezierString(Node);
}

template<int32 Dim>
inline void TBezierString3<Dim>::ChangeCtrlPointContinuous(double From, EEndPointContinuity Continuity, int32 NthPointOfFrom)
{
	FPointNode* Node = FindNodeByParam(From, NthPointOfFrom);
	Node->GetValue().Continuity = Continuity;
}

template<int32 Dim>
inline void TBezierString3<Dim>::AdjustCtrlPointTangent(double From, const TVectorX<Dim>& To, bool bNext, int32 NthPointOfFrom)
{
	FPointNode* Node = FindNodeByParam(From, NthPointOfFrom);
	AdjustCtrlPointTangent(Node, To, bNext, NthPointOfFrom);
}

template<int32 Dim>
inline void TBezierString3<Dim>::AdjustCtrlPointTangent(FPointNode* Node, const TVectorX<Dim>& To, bool bNext, int32 NthPointOfFrom)
{
	// TODO?
	TVectorX<Dim+1>* PosToChangePtr = nullptr;
	TVectorX<Dim+1>* PosToChange2Ptr = nullptr;
	TVectorX<Dim+1>* Pos2ToChangePtr = nullptr;
	TVectorX<Dim+1>* Pos2ToChange2Ptr = nullptr;
	if (bNext) {
		PosToChangePtr = &Node->GetValue().NextCtrlPointPos;
		PosToChange2Ptr = &Node->GetValue().PrevCtrlPointPos;
		if (Node->GetPrevNode()) {
			Pos2ToChange2Ptr = &Node->GetPrevNode()->GetValue().NextCtrlPointPos;
		}
		if (Node->GetNextNode()) {
			Pos2ToChangePtr = &Node->GetNextNode()->GetValue().PrevCtrlPointPos;
		}
	}
	else {
		PosToChange2Ptr = &Node->GetValue().NextCtrlPointPos;
		PosToChangePtr = &Node->GetValue().PrevCtrlPointPos;
		//TODO: Should make the changes slighter?
		if (Node->GetPrevNode()) {
			Pos2ToChangePtr = &Node->GetPrevNode()->GetValue().NextCtrlPointPos;
		}
		if (Node->GetNextNode()) {
			Pos2ToChange2Ptr = &Node->GetNextNode()->GetValue().PrevCtrlPointPos;
		}
	}
	TVectorX<Dim> Adjust = To - TVecLib<Dim+1>::Projection(*PosToChangePtr);
	*PosToChangePtr = TVecLib<Dim>::Homogeneous(To, 1.);
	TVectorX<Dim> PosProj = TVecLib<Dim+1>::Projection(Node->GetValue().Pos);
	EEndPointContinuity Con = Node->GetValue().Continuity;
	if (Con > EEndPointContinuity::C0) {
		TVectorX<Dim> TangentFront = To - PosProj;
		//TVectorX<Dim> Adjust2;
		TVectorX<Dim> CurTangentBack = TVecLib<Dim+1>::Projection(*PosToChange2Ptr) - PosProj;
		TVectorX<Dim> NewTangentBack;
		if (Continuity::IsGeometric(Con)) {
			NewTangentBack = TangentFront.GetSafeNormal() * (-TVecLib<Dim>::Size(CurTangentBack));
		}
		else {
			NewTangentBack = -TangentFront;
		}
		TVectorX<Dim> To2 = PosProj + NewTangentBack;
		*PosToChange2Ptr = TVecLib<Dim>::Homogeneous(To2, 1.);

		if ((Con >= EEndPointContinuity::G2 || Con >= EEndPointContinuity::C2) && Pos2ToChangePtr && Pos2ToChange2Ptr) {
			TVectorX<Dim> Pos22Proj = TVecLib<Dim+1>::Projection(*Pos2ToChange2Ptr);

			TVectorX<Dim> Tangent2Back = Pos22Proj + PosProj - To2 * 2.;
			TVectorX<Dim> NewTangent2Front;
			if (Con >= EEndPointContinuity::C2) {
				NewTangent2Front = Tangent2Back;
			}
			else if (Con >= EEndPointContinuity::G2) {
				TVectorX<Dim> Pos21Proj = TVecLib<Dim+1>::Projection(*Pos2ToChangePtr);
				TVectorX<Dim> Tangent2Front = Pos21Proj + PosProj - To * 2.;
				double RatioSqr = TVecLib<Dim>::SizeSquared(TangentFront) / TVecLib<Dim>::SizeSquared(NewTangentBack);
				NewTangent2Front = Tangent2Back * RatioSqr;
					//Tangent2Back.GetSafeNormal() * (TVecLib<Dim>::Size(Tangent2Front));
			}
			*Pos2ToChangePtr = TVecLib<Dim>::Homogeneous(NewTangent2Front - PosProj + To * 2., 1.);
		}
	}
	UpdateBezierString(Node);
}

template<int32 Dim>
inline void TBezierString3<Dim>::RemovePoint(double Param, int32 NthPointOfFrom)
{
	FPointNode* Node = FindNodeByParam(Param, NthPointOfFrom);
	CtrlPointsList.RemoveNode(Node);
}

template<int32 Dim>
inline void TBezierString3<Dim>::AdjustCtrlPointPos(FPointNode* Node, const TVectorX<Dim>& To, int32 NthPointOfFrom)
{
	TVectorX<Dim> From = TVecLib<Dim+1>::Projection(Node->GetValue().Pos);
	Node->GetValue().Pos = TVecLib<Dim>::Homogeneous(To, 1.);
	EEndPointContinuity Con = Node->GetValue().Continuity;
	if (Con > EEndPointContinuity::C0) {
		TVectorX<Dim> AdjustDiff = To - From;
		Node->GetValue().PrevCtrlPointPos += TVecLib<Dim>::Homogeneous(AdjustDiff, 0.);
		Node->GetValue().NextCtrlPointPos += TVecLib<Dim>::Homogeneous(AdjustDiff, 0.);

		UpdateBezierString(Node);
	}
}

template<int32 Dim>
inline void TBezierString3<Dim>::AddPointAtLast(const TVectorX<Dim>& Point, TOptional<double> Param, double Weight)
{
	double InParam = Param ? Param.GetValue() : (CtrlPointsList.Num() > 0 ? GetParamRange().Get<1>() + 1. : 0.);
	CtrlPointsList.AddTail(TBezierString3ControlPoint<Dim>(TVecLib<Dim>::Homogeneous(Point, Weight), InParam));

	//UpdateBezierString(nullptr);

	FPointNode* Tail = CtrlPointsList.GetTail();
	if (CtrlPointsList.Num() > 1) {
		FPointNode* BeforeTail = Tail->GetPrevNode();
		static constexpr double InvDegreeDbl = 1. / 3.;
		if (CtrlPointsList.Num() > 2) {
			FPointNode* BeforeBeforeTail = BeforeTail->GetPrevNode();
			TVectorX<Dim> BBPosProj = TVecLib<Dim+1>::Projection(BeforeBeforeTail->GetValue().Pos);
			TVectorX<Dim> BPosProj = TVecLib<Dim+1>::Projection(BeforeTail->GetValue().Pos);
			TVectorX<Dim> PosProj = TVecLib<Dim+1>::Projection(Tail->GetValue().Pos);

			TVectorX<Dim> BaseLine = PosProj - BBPosProj;
			double BaseLineFactor = 1. / 6.;
			BeforeTail->GetValue().NextCtrlPointPos = TVecLib<Dim>::Homogeneous(BPosProj + BaseLine * BaseLineFactor);
			if (BeforeTail->GetValue().Continuity > EEndPointContinuity::C0) {
				BeforeTail->GetValue().PrevCtrlPointPos = TVecLib<Dim>::Homogeneous(BPosProj - BaseLine * BaseLineFactor);
			}

			Tail->GetValue().PrevCtrlPointPos = (BeforeTail->GetValue().NextCtrlPointPos + Tail->GetValue().Pos) * 0.5; // ''= 0

			TVectorX<Dim> TangentFront = PosProj - TVecLib<Dim+1>::Projection(Tail->GetValue().PrevCtrlPointPos);
			Tail->GetValue().NextCtrlPointPos = TVecLib<Dim>::Homogeneous(PosProj + TangentFront, 1.);
		}
		else {
			TVectorX<Dim> Diff = TVecLib<Dim+1>::Projection(Tail->GetValue().Pos)
				- TVecLib<Dim+1>::Projection(BeforeTail->GetValue().Pos);
			TVectorX<Dim> TangentFront = Diff * InvDegreeDbl;
			TVectorX<Dim> Pos0 = TVecLib<Dim+1>::Projection(BeforeTail->GetValue().Pos);
			TVectorX<Dim> Pos1 = TVecLib<Dim+1>::Projection(Tail->GetValue().Pos);

			Tail->GetValue().NextCtrlPointPos = TVecLib<Dim>::Homogeneous(Pos1 + TangentFront, 1.);
			BeforeTail->GetValue().NextCtrlPointPos = TVecLib<Dim>::Homogeneous(Pos0 + TangentFront, 1.);
			Tail->GetValue().PrevCtrlPointPos = TVecLib<Dim>::Homogeneous(Pos1 - TangentFront, 1.);
			BeforeTail->GetValue().PrevCtrlPointPos = TVecLib<Dim>::Homogeneous(Pos0 - TangentFront, 1.);
		}
	}

	UpdateBezierString(CtrlPointsList.GetTail());
}

template<int32 Dim>
inline void TBezierString3<Dim>::AddPointAtFirst(const TVectorX<Dim>& Point, TOptional<double> Param, double Weight)
{
	double InParam = Param ? Param.GetValue() : (CtrlPointsList.Num() > 0 ? GetParamRange().Get<0>() - 1. : 0.);
	CtrlPointsList.AddHead(TBezierString3ControlPoint<Dim>(TVecLib<Dim>::Homogeneous(Point, Weight), InParam));

	//UpdateBezierString(nullptr);

	FPointNode* Head = CtrlPointsList.GetHead();
	if (CtrlPointsList.Num() > 1) {
		FPointNode* AfterHead = Head->GetNextNode();
		static constexpr double InvDegreeDbl = 1. / 3.;
		if (CtrlPointsList.Num() > 2) {
			FPointNode* AfterAfterHead = AfterHead->GetNextNode();

			TVectorX<Dim> AAPosProj = TVecLib<Dim+1>::Projection(AfterAfterHead->GetValue().Pos);
			TVectorX<Dim> APosProj = TVecLib<Dim+1>::Projection(AfterHead->GetValue().Pos);
			TVectorX<Dim> PosProj = TVecLib<Dim+1>::Projection(Head->GetValue().Pos);

			TVectorX<Dim> BaseLine = AAPosProj - PosProj;
			double BaseLineFactor = 1. / 6.;
			AfterHead->GetValue().PrevCtrlPointPos = TVecLib<Dim>::Homogeneous(APosProj - BaseLine * BaseLineFactor);
			if (AfterHead->GetValue().Continuity > EEndPointContinuity::C0) {
				AfterHead->GetValue().NextCtrlPointPos = TVecLib<Dim>::Homogeneous(APosProj + BaseLine * BaseLineFactor);
			}

			Head->GetValue().NextCtrlPointPos = (AfterHead->GetValue().PrevCtrlPointPos + Head->GetValue().Pos) * 0.5; // ''= 0

			TVectorX<Dim> TangentFront = TVecLib<Dim+1>::Projection(Head->GetValue().NextCtrlPointPos) - PosProj;
			Head->GetValue().PrevCtrlPointPos = TVecLib<Dim>::Homogeneous(PosProj - TangentFront, 1.);
		}
		else {
			TVectorX<Dim> Diff = TVecLib<Dim+1>::Projection(AfterHead->GetValue().Pos)
				- TVecLib<Dim+1>::Projection(Head->GetValue().Pos);
			TVectorX<Dim> TangentFront = Diff * InvDegreeDbl;
			TVectorX<Dim> Pos0 = TVecLib<Dim+1>::Projection(Head->GetValue().Pos);
			TVectorX<Dim> Pos1 = TVecLib<Dim+1>::Projection(AfterHead->GetValue().Pos);

			Head->GetValue().PrevCtrlPointPos = TVecLib<Dim>::Homogeneous(Pos0 - TangentFront, 1.);
			AfterHead->GetValue().PrevCtrlPointPos = TVecLib<Dim>::Homogeneous(Pos1 - TangentFront, 1.);
			Head->GetValue().NextCtrlPointPos = TVecLib<Dim>::Homogeneous(Pos0 + TangentFront, 1.);
			AfterHead->GetValue().NextCtrlPointPos = TVecLib<Dim>::Homogeneous(Pos1 + TangentFront, 1.);
		}
	}

	UpdateBezierString(CtrlPointsList.GetHead());
}

template<int32 Dim>
inline void TBezierString3<Dim>::AddPointAt(const TVectorX<Dim>& Point, TOptional<double> Param, int32 Index, double Weight)
{
	double InParam = Param ? Param.GetValue() : (CtrlPointsList.Num() > 0 ? GetParamRange().Get<1>() + 1. : 0.);
	TBezierString3ControlPoint<Dim> PointStruct(TVecLib<Dim>::Homogeneous(Point, Weight), InParam);

	FPointNode* NodeToInsertBefore = CtrlPointsList.GetHead();
	for (int32 i = 0; i < Index; ++i) {
		if (NodeToInsertBefore) {
			NodeToInsertBefore = NodeToInsertBefore->GetNextNode();
		}
		else {
			break;
		}
	}
	if (NodeToInsertBefore) {
		CtrlPointsList.InsertNode(PointStruct, NodeToInsertBefore);
		UpdateBezierString(nullptr);
		//UpdateBezierString(NodeToInsertBefore->GetPrevNode());
	}
	else {
		CtrlPointsList.AddTail(PointStruct);
		UpdateBezierString(nullptr);
		//UpdateBezierString(CtrlPointsList.GetTail());
	}
}

template<int32 Dim>
inline void TBezierString3<Dim>::RemovePointAt(int32 Index)
{
	FPointNode* Node = CtrlPointsList.GetHead();
	for (int32 i = 0; i < Index; ++i) {
		if (Node) {
			Node = Node->GetNextNode();
		}
	}
	CtrlPointsList.RemoveNode(Node);
}

template<int32 Dim>
inline void TBezierString3<Dim>::RemovePoint(const TVectorX<Dim>& Point, int32 NthPointOfFrom)
{
	FPointNode* Node = FindNodeByPosition(Point, NthPointOfFrom);
	CtrlPointsList.RemoveNode(Node);
}

template<int32 Dim>
inline void TBezierString3<Dim>::AdjustCtrlPointPos(const TVectorX<Dim>& From, const TVectorX<Dim>& To, int32 NthPointOfFrom)
{
	FPointNode* Node = FindNodeByPosition(From, NthPointOfFrom);
	AdjustCtrlPointPos(Node, To, NthPointOfFrom);
}

template<int32 Dim>
inline void TBezierString3<Dim>::Reverse()
{
	if (!CtrlPointsList.GetHead() || !CtrlPointsList.GetTail()) {
		return;
	}
	TDoubleLinkedList<TBezierString3ControlPoint<Dim> > NewList;
	double SumParam = CtrlPointsList.GetHead()->GetValue().Param + CtrlPointsList.GetTail()->GetValue().Param;
	for (const auto& Point : CtrlPointsList) {
		NewList.AddHead(Point);
		NewList.GetHead()->GetValue().Param = SumParam - NewList.GetHead()->GetValue().Param;
		std::swap(NewList.GetHead()->GetValue().PrevCtrlPointPos, NewList.GetHead()->GetValue().NextCtrlPointPos);
	}
	CtrlPointsList.Empty();
	for (const auto& Point : NewList) {
		CtrlPointsList.AddTail(Point);
	}
}

template<int32 Dim>
inline TVectorX<Dim> TBezierString3<Dim>::GetPosition(double T) const
{
	int32 ListNum = CtrlPointsList.Num();
	if (ListNum == 0) {
		return TVecLib<Dim>::Zero();
	}
	else if (ListNum == 1) {
		return TVecLib<Dim+1>::Projection(CtrlPointsList.GetHead()->GetValue().Pos);
	}
	const auto& ParamRange = GetParamRange();
	if (ParamRange.Get<0>() >= T) {
		TVecLib<Dim+1>::Projection(CtrlPointsList.GetHead()->GetValue().Pos);
	}
	else if(T >= ParamRange.Get<1>()) {
		return TVecLib<Dim+1>::Projection(CtrlPointsList.GetTail()->GetValue().Pos);
	}

	FPointNode* EndNode = FindNodeGreaterThanParam(T);
	FPointNode* StartNode = EndNode->GetPrevNode() ? EndNode->GetPrevNode() : EndNode;
	double TN = GetNormalizedParam(StartNode, EndNode, T);
	TBezierCurve<Dim, 3> TargetCurve = MakeBezierCurve(StartNode, EndNode);
	return TargetCurve.GetPosition(TN);
}

template<int32 Dim>
inline TVectorX<Dim> TBezierString3<Dim>::GetTangent(double T) const
{
	//if (constexpr(Degree <= 0)) {
	//	return TVecLib<Dim>::Zero();
	//}
	FPointNode* EndNode = FindNodeGreaterThanParam(T);
	if (!EndNode) {
		EndNode = CtrlPointsList.GetTail();
	}
	FPointNode* StartNode = EndNode->GetPrevNode();
	double TN = GetNormalizedParam(StartNode, EndNode, T);
	TBezierCurve<Dim, 3> Curve = MakeBezierCurve(StartNode, EndNode);
	TBezierCurve<Dim, 2> Hodograph;
	Curve.CreateHodograph(Hodograph);
	
	TVectorX<Dim> Tangent = Hodograph.GetPosition(TN);
	return Tangent.IsNearlyZero() ? Hodograph.GetTangent(TN) : Tangent;
}

template<int32 Dim>
inline double TBezierString3<Dim>::GetPrincipalCurvature(double T, int32 Principal) const
{
	FPointNode* EndNode = FindNodeGreaterThanParam(T);
	if (!EndNode) {
		EndNode = CtrlPointsList.GetTail();
	}
	FPointNode* StartNode = EndNode->GetPrevNode();
	double TN = GetNormalizedParam(StartNode, EndNode, T);
	TBezierCurve<Dim, 3> Curve = MakeBezierCurve(StartNode, EndNode);
	TBezierCurve<Dim, 2> Hodograph;
	Curve.CreateHodograph(Hodograph);
	TBezierCurve<Dim, 1> Hodograph2;
	Hodograph.CreateHodograph(Hodograph2);

	return TVecLib<Dim>::PrincipalCurvature(Hodograph.GetPosition(TN), Hodograph2.GetPosition(TN), Principal);
}

template<int32 Dim>
inline double TBezierString3<Dim>::GetCurvature(double T) const
{
	FPointNode* EndNode = FindNodeGreaterThanParam(T);
	FPointNode* StartNode = EndNode->GetPrevNode();
	double TN = GetNormalizedParam(StartNode, EndNode, T);
	TBezierCurve<Dim, 3> Curve = MakeBezierCurve(StartNode, EndNode);
	TBezierCurve<Dim, 2> Hodograph;
	Curve.CreateHodograph(Hodograph);
	TBezierCurve<Dim, 1> Hodograph2;
	Hodograph.CreateHodograph(Hodograph2);

	return TVecLib<Dim>::Curvature(Hodograph.GetPosition(TN), Hodograph2.GetPosition(TN));
}

template<int32 Dim>
inline void TBezierString3<Dim>::ToPolynomialForm(TArray<TArray<TVectorX<Dim+1>>>& OutPolyForms) const
{
	FPointNode* Node = CtrlPointsList.GetHead();
	if (!Node) {
		return;
	}
	OutPolyForms.Empty(CtrlPointsList.Num() - 1);
	while (Node && Node->GetNextNode()) {
		FPointNode* Next = Node->GetNextNode();

		TArray<TVectorX<Dim+1> >& NewArray = OutPolyForms.AddDefaulted_GetRef();
		NewArray.SetNum(4);
		TBezierCurve<Dim, 3> NewBezier = MakeBezierCurve(Node, Next);
		NewBezier.ToPolynomialForm(NewArray.GetData());

		Node = Next;
	}
}


template<int32 Dim>
inline TTuple<double, double> TBezierString3<Dim>::GetParamRange() const
{
	if (CtrlPointsList.GetHead() && CtrlPointsList.GetTail()) {
		return MakeTuple(CtrlPointsList.GetHead()->GetValue().Param, CtrlPointsList.GetTail()->GetValue().Param);
	}
	return MakeTuple(0., 0.);
}

template<int32 Dim>
inline bool TBezierString3<Dim>::FindParamByPosition(double& OutParam, const TVectorX<Dim>& InPos, double ToleranceSqr) const
{
	FPointNode* Node = CtrlPointsList.GetHead();
	if (!Node) {
		return false;
	}
	TOptional<double> CurParam;
	TOptional<double> CurDistSqr;
	while (Node && Node->GetNextNode()) {
		FPointNode* Next = Node->GetNextNode();

		TBezierCurve<Dim, 3> NewBezier = MakeBezierCurve(Node, Next);
		double NewParamNormal = -1.;
		if (NewBezier.FindParamByPosition(NewParamNormal, InPos, ToleranceSqr)) {
			double NewParam = Node->GetValue().Param * (1. - NewParamNormal) + Next->GetValue().Param * NewParamNormal;
			if (CurParam) {
				TVectorX<Dim> NewPos = NewBezier.GetPosition(NewParamNormal);
				double NewDistSqr = TVecLib<Dim>::SizeSquared(NewPos - InPos);
				if (NewDistSqr < CurDistSqr.GetValue()) {
					CurDistSqr = NewDistSqr;
					CurParam = NewParam;
				}

			}
			else {
				TVectorX<Dim> CurPos = NewBezier.GetPosition(NewParamNormal);
				CurDistSqr = TVecLib<Dim>::SizeSquared(CurPos - InPos);
				CurParam = NewParam;
			}
		}

		Node = Next;
	}

	if (CurParam) {
		OutParam = CurParam.GetValue();
		return true;
	}
	return false;
}

template<int32 Dim>
inline double TBezierString3<Dim>::GetNormalizedParam(
	const typename TBezierString3<Dim>::FPointNode* StartNode,
	const typename TBezierString3<Dim>::FPointNode* EndNode, 
	double T) const
{
	double De = EndNode->GetValue().Param - StartNode->GetValue().Param;
	return FMath::IsNearlyZero(De) ? 0.5 : (T - StartNode->GetValue().Param) / De;
}

template<int32 Dim>
inline TBezierCurve<Dim, 3> TBezierString3<Dim>::MakeBezierCurve(
	const typename TBezierString3<Dim>::FPointNode* StartNode, 
	const typename TBezierString3<Dim>::FPointNode* EndNode) const
{
	TVectorX<Dim+1> Array[] {
		StartNode->GetValue().Pos,
		StartNode->GetValue().NextCtrlPointPos,
		EndNode->GetValue().PrevCtrlPointPos,
		EndNode->GetValue().Pos };
	return TBezierCurve<Dim, 3>(Array);
}

template<int32 Dim>
inline void TBezierString3<Dim>::UpdateBezierString(typename TBezierString3<Dim>::FPointNode* NodeToUpdateFirst)
{
	// Check?
	for (FPointNode* Node = CtrlPointsList.GetHead(); Node && Node->GetNextNode(); Node = Node->GetNextNode())
	{
		ensureAlways(Node->GetValue().Param <= Node->GetNextNode()->GetValue().Param);
	}

	if (!NodeToUpdateFirst) {
		// Interpolate all
		TArray<TVectorX<Dim+1> > EndPoints;
		GetCtrlPoints(EndPoints);
		if (EndPoints.Num() < 2) {
			return;
		}
		TArray<TBezierCurve<Dim, 3> > Beziers;
		TBezierOperationsDegree3<Dim>::InterpolationC2WithBorder2ndDerivative(Beziers, EndPoints);
		FromCurveArray(Beziers);
		return;
	}

	for (FPointNode* PrevNode = NodeToUpdateFirst->GetPrevNode(); PrevNode; PrevNode = PrevNode->GetPrevNode()) {
		if (!AdjustPointByStaticPointReturnShouldSpread(PrevNode, true)) {
			break;
		}
	}

	for (FPointNode* NextNode = NodeToUpdateFirst->GetNextNode(); NextNode; NextNode = NextNode->GetNextNode()) {
		if (!AdjustPointByStaticPointReturnShouldSpread(NextNode, false)) {
			break;
		}
	}

	//static const TMap<EEndPointContinuity, int32> TypeMap{
	//	{EEndPointContinuity::C0, 0},
	//	{EEndPointContinuity::C1, 1},
	//	{EEndPointContinuity::G1, 1},
	//	{EEndPointContinuity::C2, 2},
	//	{EEndPointContinuity::G2, 2},
	//};
	//int32 PointToAdjustEachSide = 2;//TypeMap[NodeToUpdateFirst->GetValue().Continuity];
}

template<int32 Dim>
inline bool TBezierString3<Dim>::AdjustPointByStaticPointReturnShouldSpread(TBezierString3<Dim>::FPointNode* Node, bool bFromNext)
{
	EEndPointContinuity Con = Node->GetValue().Continuity;
	if (Con == EEndPointContinuity::C0) {
		return false;
	}

	auto GetNextCtrlPointPosOrReverse = [bFromNext](FPointNode* Node) -> TVectorX<Dim> {
		if (bFromNext) {
			return TVecLib<Dim+1>::Projection(Node->GetValue().NextCtrlPointPos);
		}
		else {
			return TVecLib<Dim+1>::Projection(Node->GetValue().PrevCtrlPointPos);
		}
	};
	auto GetPrevCtrlPointPosOrReverse = [bFromNext](FPointNode* Node) -> TVectorX<Dim> {
		if (bFromNext) {
			return TVecLib<Dim+1>::Projection(Node->GetValue().PrevCtrlPointPos);
		}
		else {
			return TVecLib<Dim+1>::Projection(Node->GetValue().NextCtrlPointPos);
		}
	};
	auto GetNextNodeOrReverse = [bFromNext](FPointNode* Node)->FPointNode* {
		if (bFromNext) {
			return Node->GetNextNode();
		}
		else {
			return Node->GetPrevNode();
		}
	};
	auto GetPrevNodeOrReverse = [bFromNext](FPointNode* Node)->FPointNode* {
		if (bFromNext) {
			return Node->GetPrevNode();
		}
		else {
			return Node->GetNextNode();
		}
	};
	auto SetPrevCtrlPointPosOrReverse = [bFromNext](FPointNode* Node, const TVectorX<Dim>& Target) {
		if (bFromNext) {
			Node->GetValue().PrevCtrlPointPos = TVecLib<Dim>::Homogeneous(Target, 1.);
		}
		else {
			Node->GetValue().NextCtrlPointPos = TVecLib<Dim>::Homogeneous(Target, 1.);
		}
	};
	auto SetNextCtrlPointPosOrReverse = [bFromNext](FPointNode* Node, const TVectorX<Dim>& Target) {
		if (bFromNext) {
			Node->GetValue().NextCtrlPointPos = TVecLib<Dim>::Homogeneous(Target, 1.);
		}
		else {
			Node->GetValue().PrevCtrlPointPos = TVecLib<Dim>::Homogeneous(Target, 1.);
		}
	};

	//if (bFromNext) {
	TVectorX<Dim> NextProj = GetNextCtrlPointPosOrReverse(Node);
	TVectorX<Dim> CurProj = TVecLib<Dim+1>::Projection(Node->GetValue().Pos);
	TVectorX<Dim> PrevProj = GetPrevCtrlPointPosOrReverse(Node);
	TVectorX<Dim> TangentFront = NextProj - CurProj, TangentBack = PrevProj - CurProj;
	if (Continuity::IsGeometric(Con)) {
		double Dot = TVecLib<Dim>::Dot(TangentFront, TangentBack);
		if (Con == EEndPointContinuity::G1 &&
			FMath::IsNearlyEqual(Dot * Dot, TVecLib<Dim>::SizeSquared(TangentFront) * TVecLib<Dim>::SizeSquared(TangentBack))) {
			return false;
		} // P' = k * Q'
		TVectorX<Dim> NewTangentBack = TangentFront.GetSafeNormal() * (-TVecLib<Dim>::Size(TangentBack));
		TVectorX<Dim> NewPrevProj = CurProj + NewTangentBack;
		SetPrevCtrlPointPosOrReverse(Node, NewPrevProj);

		FPointNode* NextNode = GetNextNodeOrReverse(Node);
		FPointNode* PrevNode = GetPrevNodeOrReverse(Node);
		if (NextNode && PrevNode && Con == EEndPointContinuity::G2) { // P'' = k^2 * Q'' + j * Q'
			TVectorX<Dim> PPProj = GetNextCtrlPointPosOrReverse(PrevNode);
			TVectorX<Dim> NNProj = GetPrevCtrlPointPosOrReverse(NextNode);
			TVectorX<Dim> Tangent2Front = CurProj - NextProj * 2. + NNProj;
			TVectorX<Dim> Tangent2Back = CurProj - NewPrevProj * 2. + PPProj;
			double RatioSqr = TVecLib<Dim>::SizeSquared(NewTangentBack) / TVecLib<Dim>::SizeSquared(TangentFront);

			TVectorX<Dim> T2Diff = Tangent2Front * RatioSqr - Tangent2Back;
			double DotDiff = TVecLib<Dim>::Dot(T2Diff, TangentFront);
			double CmpDiff = DotDiff * DotDiff - TVecLib<Dim>::SizeSquared(T2Diff) * TVecLib<Dim>::SizeSquared(TangentFront);

			if (FMath::IsNearlyZero(CmpDiff)) {
				return true;
			}

			double Ratio2 = TVecLib<Dim>::Size(T2Diff) / TVecLib<Dim>::Size(TangentFront);

			//if (FMath::IsNearlyEqual(Dot2 * Dot2, TVecLib<Dim>::SizeSquared(Tangent2Front) * TVecLib<Dim>::SizeSquared(Tangent2Back))) {
			//	return true;
			//}

			TVectorX<Dim> NewTangent2Back = Tangent2Front * RatioSqr;// -TangentFront * Ratio2; // TEST?
				//Tangent2Front.GetSafeNormal()* (TVecLib<Dim>::Size(Tangent2Back));
			TVectorX<Dim> NewPPProj = NewTangent2Back - CurProj + NewPrevProj * 2.;
			SetNextCtrlPointPosOrReverse(PrevNode, NewPPProj);

			//TVectorX<Dim> NewT2Diff = Tangent2Front * RatioSqr - NewTangent2Back;
			//double NewDotDiff = TVecLib<Dim>::Dot(NewT2Diff, TangentFront);
			//double NewCmpDiff = NewDotDiff * NewDotDiff - TVecLib<Dim>::SizeSquared(NewT2Diff) * TVecLib<Dim>::SizeSquared(TangentFront);
			//if (!FMath::IsNearlyZero(NewCmpDiff)) {
			//	check(false);
			//}
		}
	}
	else {
		if (Con == EEndPointContinuity::C1 && TVecLib<Dim>::IsNearlyZero(TangentFront + TangentBack)) {
			return false;
		}
		TVectorX<Dim> NewTangentBack = TangentFront * (-1.);
		TVectorX<Dim> NewPrevProj = CurProj + NewTangentBack;
		SetPrevCtrlPointPosOrReverse(Node, NewPrevProj);
		//Node->GetValue().PrevCtrlPointPos = TVecLib<Dim>::Homogeneous(NewPrevProj, 1.);

		FPointNode* NextNode = GetNextNodeOrReverse(Node); //Node->GetNextNode();
		FPointNode* PrevNode = GetPrevNodeOrReverse(Node); //Node->GetPrevNode();
		if (NextNode && PrevNode && Con == EEndPointContinuity::C2) {
			TVectorX<Dim> PPProj = GetNextCtrlPointPosOrReverse(PrevNode); //TVecLib<Dim+1>::Projection(PrevNode->GetValue().NextCtrlPointPos);
			TVectorX<Dim> NNProj = GetPrevCtrlPointPosOrReverse(NextNode); //TVecLib<Dim+1>::Projection(NextNode->GetValue().PrevCtrlPointPos);
			TVectorX<Dim> Tangent2Front = CurProj - NextProj * 2. + NNProj;
			TVectorX<Dim> Tangent2Back = CurProj - NewPrevProj * 2. + PPProj;
			if (TVecLib<Dim>::IsNearlyZero(Tangent2Front - Tangent2Back)) {
				return true;
			}
			TVectorX<Dim> NewTangent2Back = Tangent2Front * (1.);
			TVectorX<Dim> NewPPProj = NewTangent2Back - CurProj + NewPrevProj * 2.;
			SetNextCtrlPointPosOrReverse(PrevNode, NewPPProj);
			//PrevNode->GetValue().NextCtrlPointPos = TVecLib<Dim>::Homogeneous(NewPPProj);
		}
	}
	//}
	//else { // FromPrev
	//	FPointNode* PrevNode = Node->GetPrevNode();
	//	//TODO
	//}
	return true;
}
