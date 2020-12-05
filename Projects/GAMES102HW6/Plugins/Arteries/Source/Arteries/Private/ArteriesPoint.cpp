// Author: LiJiayu (JerryLi)
// Mail: lijiayu83@gmail.com (fullike@163.com)
// Copyright 2019. All Rights Reserved.

#include "ArteriesPoint.h"
#include "ArteriesObject.h"
/*
inline float RadianBetween(FArteriesLink* PrevLink, FArteriesLink* NextLink)
{
	float PrevRadian = PrevLink->Radian;
	float NextRadian = NextLink->Radian;
	if (NextRadian < PrevRadian)
		NextRadian += PI * 2;
	return NextRadian - PrevRadian;
}*/
FArteriesLink::FArteriesLink(FArteriesPoint* Source, FArteriesPoint* InTarget, FArteriesPrimitive* InPrimitive, bool bInverse) :Target(InTarget), Primitive(InPrimitive), Inverse(bInverse)
{
}
inline float FArteriesLink::GetHalfWidth(float DefaultWidth)
{
	return Primitive->GetHalfWidth(DefaultWidth);
}
/*
inline FArteriesLink* FArteriesLink::Prev()
{
	return Opposite()->Next()->Opposite();
}
inline FArteriesLink* FArteriesLink::Opposite()
{
	for (FArteriesLink& L : Target->Links)
	{
		if (L.Primitive == Primitive && L.Inverse != Inverse)
			return &L;
	}
	return NULL;
}
inline FArteriesLink* FArteriesLink::Next()
{
	for (FArteriesLink& L : Target->Links)
	{
		if (L.Primitive == Primitive && L.Inverse == Inverse)
			return &L;
	}
	return NULL;
}*/
void FArteriesPoint::Serialize(FArchive& Ar, UObject* Owner, int Index)
{
	if (Ar.IsLoading() || Ar.IsSaving() || Ar.IsCountingMemory())
	{
		UStruct* Struct = FArteriesPoint::StaticStruct();
		Struct->SerializeTaggedProperties(Ar, (uint8*)this, Struct, NULL);
	}
}
void FArteriesPoint::GetSmoothTangentsForPrimitive(const FArteriesPrimitive* Primitive, FVector& X, FVector& Y, FVector& Z, float SmoothThreshold)
{
	X = Primitive->GetVec3(AVN_TangentX);
	Y = Primitive->GetVec3(AVN_TangentY);
	Z = Primitive->GetVec3(AVN_TangentZ);
	const FVector TangentZ = Z;
	TArray<const FArteriesPrimitive*> Others;
	for (FArteriesLink& L : Links)
	{
		if (L.Primitive != Primitive)
			Others.AddUnique(L.Primitive);
	}
	for (const FArteriesPrimitive* Prim : Others)
	{
		FVector Normal = Prim->GetVec3(AVN_TangentZ);
		if ((TangentZ | Normal) > SmoothThreshold)
		{
			Z += Normal;
			X += Prim->GetVec3(AVN_TangentX);
			Y += Prim->GetVec3(AVN_TangentY);
		}
	}
	X.Normalize();
	Y.Normalize();
	Z.Normalize();
}
bool FArteriesPoint::IsRoundCorner()
{
	if (Links.Num() < 2)
		return false;
	if (Links.Num() > 2)
		return true;
	//将来换成宽度不等！！！
	return Links[0].Primitive != Links[1].Primitive;
}
FArteriesLink* FArteriesPoint::AddLink(FArteriesPoint* Target, FArteriesPrimitive* Primitive, bool bInverse)
{
	FArteriesLink* Link = new FArteriesLink(this, Target, Primitive, bInverse);
	Links.Add(Link);
	return Link;
}
TArray<FArteriesLink*> FArteriesPoint::GetLinks(const FArteriesPoint* Target, const FArteriesPrimitive* Primitive, int Direction)
{
	TArray<FArteriesLink*> OutLinks;
	for (FArteriesLink& L : Links)
	{
		if (Target && L.Target != Target)
			continue;
		if (Primitive && L.Primitive != Primitive)
			continue;
		if (Direction >= 0 && L.Inverse != !!Direction)
			continue;
		OutLinks.Add(&L);
	}
	return MoveTemp(OutLinks);
}
TArray<FArteriesLink*> FArteriesPoint::GetSortedLinks()
{
	TArray<FArteriesLink*> OutLinks;
	for (FArteriesLink& L : Links)
	{
		if (L.Inverse || OutLinks.Contains(&L))
			continue;
		int InsertIndex = OutLinks.Num();
		OutLinks.Add(&L);
		//往前找
		{
			FArteriesLink* Link = &L;
			while (true)
			{
				if (Link->Inverse)
				{
					TArray<FArteriesLink*> Results = GetLinks(Link->Target, NULL, 0);
					Link = Results.Num() ? Results[0] : NULL;
				}
				else
				{
					TArray<FArteriesLink*> Results = GetLinks(NULL, Link->Primitive, 1);
					Link = Results.Num() ? Results[0] : NULL;
				}
				if (!Link || OutLinks.Contains(Link))
					break;
				OutLinks.Insert(Link, InsertIndex);
			}
		}
		//往后找
		{
			FArteriesLink* Link = &L;
			while (true)
			{
				if (Link->Inverse)
				{
					TArray<FArteriesLink*> Results = GetLinks(NULL, Link->Primitive, 1);
					Link = Results.Num() ? Results[0] : NULL;
				}
				else
				{
					TArray<FArteriesLink*> Results = GetLinks(Link->Target, NULL, 0);
					Link = Results.Num() ? Results[0] : NULL;
				}
				if (!Link || OutLinks.Contains(Link))
					break;
				OutLinks.Add(Link);
			}
		}
	}
	return MoveTemp(OutLinks);
}
TArray<FArteriesPoint*> FArteriesPoint::GetTargets()
{
	TArray<FArteriesPoint*> Targets;
	for (FArteriesLink& L : Links)
		Targets.AddUnique(L.Target);
	return MoveTemp(Targets);
}
TArray<FArteriesPrimitive*> FArteriesPoint::GetPrimitives() const
{
	TArray<FArteriesPrimitive*> Primitives;
	for (const FArteriesLink& L : Links)
		Primitives.AddUnique(L.Primitive);
	return MoveTemp(Primitives);
}
int FArteriesPoint::DeleteLinks(const FArteriesPoint* Target, const FArteriesPrimitive* Primitive, int Direction)
{
	int Count = 0;
	for (int i = Links.Num()-1; i >= 0; i--)
	{
		FArteriesLink& L = Links[i];
		if (Target && L.Target != Target)
			continue;
		if (Primitive && L.Primitive != Primitive)
			continue;
		if (Direction >= 0 && L.Inverse != !!Direction)
			continue;
		Links.RemoveAt(i);
		Count++;
	}
	return Count;
}
void FArteriesPoint::SortLinksForRoads()
{
	TMap<FArteriesLink*, float> Radians;
	for (FArteriesLink& L : Links)
	{
		FVector Direction = GetLinkDirection(&L);
		Radians.Add(&L, FMath::Atan2(Direction.X, Direction.Y));
	}
	Sort(Links.GetData(), Links.Num(), [&](const FArteriesLink& L0, const FArteriesLink& L1) {return Radians[&L0] < Radians[&L1]; });
}
int FArteriesPoint::GetLinkIndex(FArteriesLink* Link)
{
	for (int i = 0; i < Links.Num(); i++)
	{
		if (&Links[i] == Link)
			return i;
	}
	return INDEX_NONE;
}
FArteriesLink* FArteriesPoint::GetPrevLink(FArteriesLink* Link)
{
	int Index = GetLinkIndex(Link);
	return &Links[(Index - 1 + Links.Num()) % Links.Num()];
}
FArteriesLink* FArteriesPoint::GetNextLink(FArteriesLink* Link)
{
	int Index = GetLinkIndex(Link);
	return &Links[(Index + 1) % Links.Num()];
}
float FArteriesPoint::RadianBetween(FArteriesLink* PrevLink, FArteriesLink* NextLink)
{
	FVector v0 = GetLinkDirection(PrevLink);
	FVector v1 = GetLinkDirection(NextLink);
	//返回值从0到2PI
	return FMath::Atan2((v0^v1).Z, v0 | v1) + PI;
}
float FArteriesPoint::GetCutDistance(FArteriesLink* Link, float DefaultWidth)
{
	FArteriesLink* PrevLink = GetPrevLink(Link);
	FArteriesLink* NextLink = GetNextLink(Link);
	float PrevRadian = RadianBetween(PrevLink, Link)/2;
	float NextRadian = RadianBetween(Link, NextLink)/2;
	float Cut = Link->GetHalfWidth(DefaultWidth);
	static float Sin22_5 = 0.3826834f;
	return FMath::Max((Cut + PrevLink->GetHalfWidth(DefaultWidth)) / FMath::Max(Sin22_5, FMath::Sin(PrevRadian)), (Cut + NextLink->GetHalfWidth(DefaultWidth)) / FMath::Max(Sin22_5, FMath::Sin(NextRadian)));
}