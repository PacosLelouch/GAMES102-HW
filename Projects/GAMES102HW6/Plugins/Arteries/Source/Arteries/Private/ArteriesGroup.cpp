// Author: LiJiayu (JerryLi)
// Mail: lijiayu83@gmail.com (fullike@163.com)
// Copyright 2019. All Rights Reserved.

#include "ArteriesGroup.h"
#include "ArteriesObject.h"
inline void FArteriesPointGroup::Add(FArteriesPoint* Point)
{
	Points.Add(Point);
}
inline void FArteriesPointGroup::Remove(FArteriesPoint* Point)
{
	Points.Remove(Point);
}
void FArteriesPointGroup::PreSave(UArteriesObject* Object, TMap<FArteriesPoint*, int>& PointIDs)
{
	IDs.Empty();
	for (int i = 0; i < Points.Num();)
	{
		if (int* Index = PointIDs.Find(Points[i]))
		{
			IDs.Add(*Index);
			i++;
		}
		else
			Points.RemoveAt(i);
	}
}
void FArteriesPointGroup::PostLoad(UArteriesObject* Object)
{
	Points.Empty();
	for (int& Index : IDs)
		Points.Add(&Object->Points[Index]);
}
inline void FArteriesPrimitiveGroup::Add(FArteriesPrimitive* Primitive)
{
	Primitives.Add(Primitive);
}
inline void FArteriesPrimitiveGroup::Remove(FArteriesPrimitive* Primitive)
{
	Primitives.Remove(Primitive);
}
void FArteriesPrimitiveGroup::PreSave(UArteriesObject* Object, TMap<FArteriesPrimitive*, int>& PrimitiveIDs)
{
	IDs.Empty();
	for (int i = 0; i < Primitives.Num();)
	{
		if (int* Index = PrimitiveIDs.Find(Primitives[i]))
		{
			IDs.Add(*Index);
			i++;
		}
		else
			Primitives.RemoveAt(i);
	}
}
void FArteriesPrimitiveGroup::PostLoad(UArteriesObject* Object)
{
	Primitives.Empty();
	for (int& Index : IDs)
		Primitives.Add(&Object->Primitives[Index]);
}
inline void FArteriesEdgeGroup::Add(FArteriesPoint* Key, FArteriesPoint* Value)
{
	Edges.Add(TPair<FArteriesPoint*, FArteriesPoint*>(Key, Value));
}
void FArteriesEdgeGroup::PreSave(UArteriesObject* Object, TMap<FArteriesPoint*, int>& PointIDs)
{
	IDs.Empty();
	for (int i = 0; i < Edges.Num();)
	{
		int* Index0 = PointIDs.Find(Edges[i].Key);
		int* Index1 = PointIDs.Find(Edges[i].Value);
		if (Index0 && Index1)
		{
			IDs.Add(FIntPoint(*Index0, *Index1));
			i++;
		}
		else
			Edges.RemoveAt(i);
	}
}
void FArteriesEdgeGroup::PostLoad(UArteriesObject* Object)
{
	Edges.Empty();
	for (FIntPoint& Index : IDs)
		Edges.Add(TPair<FArteriesPoint*, FArteriesPoint*>(&Object->Points[Index.X], &Object->Points[Index.Y]));
}
TSet<FArteriesPoint*> UArteriesObject::GetPoints(const FString& Str)
{
	TSet<FArteriesPoint*> OutPoints;
	if (Str != "")
	{
		TArray<FString> Strs;
		Str.ParseIntoArray(Strs, TEXT(" "));
		for (FString& S : Strs)
		{
			if (const FArteriesPointGroup* Group = PointGroups.Find(*S))
			{
				for (FArteriesPoint* Point : Group->Points)
					OutPoints.Add(Point);
			}
			else if (S.IsNumeric())
			{
				int Index = TCString<TCHAR>::Atoi(*S);
				if (Points.IsValidIndex(Index))
					OutPoints.Add((FArteriesPoint*)&Points[Index]);
			}
		}
	}
	else
	{
		for (const FArteriesPoint& Point : Points)
			OutPoints.Add((FArteriesPoint*)&Point);
	}
	return MoveTemp(OutPoints);
}

TSet<FArteriesPrimitive*> UArteriesObject::GetPrimitives(const FString& Str)
{
	TSet<FArteriesPrimitive*> OutPrimitives;
	if (Str != "")
	{
		TArray<FString> Strs;
		Str.ParseIntoArray(Strs, TEXT(" "));
		for (FString& S : Strs)
		{
			if (const FArteriesPrimitiveGroup* Group = PrimitiveGroups.Find(*S))
			{
				for (FArteriesPrimitive* Prim : Group->Primitives)
					OutPrimitives.Add(Prim);
			}
			else if (S.IsNumeric())
			{
				int Index = TCString<TCHAR>::Atoi(*S);
				if (Primitives.IsValidIndex(Index))
					OutPrimitives.Add((FArteriesPrimitive*)&Primitives[Index]);
			}
		}
	}
	else
	{
		for (const FArteriesPrimitive& Primitive : Primitives)
			OutPrimitives.Add((FArteriesPrimitive*)&Primitive);
	}
	return MoveTemp(OutPrimitives);
}

TSet<FArteriesPrimitive*> UArteriesObject::GetPrimitivesWithTags(const FString& Tags)
{
	TSet<FArteriesPrimitive*> OutPrimitives;
	TArray<FString> Strs;
	Tags.ParseIntoArray(Strs, TEXT(" "));
	for (const FArteriesPrimitive& Primitive : Primitives)
	{
		for (FString& S : Strs)
		{
			if (Primitive.HasInt(*S))
			{
				OutPrimitives.Add((FArteriesPrimitive*)&Primitive);
				break;
			}
		}
	}
	return MoveTemp(OutPrimitives);
}

TSet<TPair<FArteriesPoint*, FArteriesPoint*>> UArteriesObject::GetEdges(const FString& Str)
{
	TSet<TPair<FArteriesPoint*, FArteriesPoint*>> Edges;
	TArray<FString> Strs;
	Str.ParseIntoArray(Strs, TEXT(" "));
	for (FString& S : Strs)
	{
		if (const FArteriesEdgeGroup* Group = EdgeGroups.Find(*S))
		{
			for (const TPair<FArteriesPoint*, FArteriesPoint*>& Edge : Group->Edges)
				Edges.Add(Edge);
		}
		else
		{
			TArray<FString> Numbers;
			S.ParseIntoArray(Numbers, TEXT("-"));
			if (Numbers.Num() == 2 && Numbers[0].IsNumeric() && Numbers[1].IsNumeric())
			{
				int Idx0 = TCString<TCHAR>::Atoi(*Numbers[0]);
				int Idx1 = TCString<TCHAR>::Atoi(*Numbers[1]);
				if (Points.IsValidIndex(Idx0) && Points.IsValidIndex(Idx1))
					Edges.Add(TPair<FArteriesPoint*, FArteriesPoint*>(&Points[Idx0], &Points[Idx1]));
			}
		}
	}
	return MoveTemp(Edges);
}
void UArteriesObject::SetPointGroup(FArteriesPoint* Point, const FName& GroupName, bool State, bool Unique)
{
	if (State)
	{
		if (Unique)
		{
			PointGroups.FindOrAdd(GroupName).Points.AddUnique(Point);
		}
		else
		{
			PointGroups.FindOrAdd(GroupName).Points.Add(Point);
		}
	}
	else if (FArteriesPointGroup* Group = PointGroups.Find(GroupName))
		Group->Points.Remove(Point);
}
void UArteriesObject::SetPointGroupANSI(FArteriesPoint* Point, const char* GroupName, bool State)
{
	SetPointGroup(Point, GroupName, State);
}
void UArteriesObject::SetPrimitiveGroup(FArteriesPrimitive* Primitive, const FName& GroupName, bool State, bool Unique)
{
	if (State)
	{
		if (Unique)
		{
			PrimitiveGroups.FindOrAdd(GroupName).Primitives.AddUnique(Primitive);
		}
		else
		{
			PrimitiveGroups.FindOrAdd(GroupName).Primitives.Add(Primitive);
		}
	}
	else if (FArteriesPrimitiveGroup* Group = PrimitiveGroups.Find(GroupName))
		Group->Primitives.Remove(Primitive);
}
void UArteriesObject::SetPrimitiveGroupANSI(FArteriesPrimitive* Primitive, const char* GroupName, bool State)
{
	SetPrimitiveGroup(Primitive, GroupName, State);
}