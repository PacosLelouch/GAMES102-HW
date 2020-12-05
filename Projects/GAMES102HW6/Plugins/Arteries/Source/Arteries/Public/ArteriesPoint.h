// Author: LiJiayu (JerryLi)
// Mail: lijiayu83@gmail.com (fullike@163.com)
// Copyright 2019. All Rights Reserved.

#pragma once
#include "ArteriesElement.h"
#include "ArteriesPoint.generated.h"

class UArteriesObject;
struct FArteriesPoint;
struct FArteriesPrimitive;
struct ARTERIES_API FArteriesLink
{
	FArteriesLink(FArteriesPoint* Source, FArteriesPoint* InTarget, FArteriesPrimitive* InPrimitive, bool bInverse);
	float GetHalfWidth(float DefaultWidth);
/*
	FArteriesLink* Prev();
	FArteriesLink* Opposite();
	FArteriesLink* Next();
*/	
	FArteriesPoint* Target;
	FArteriesPrimitive* Primitive;
	bool Inverse;
};
inline FIntVector GetIntVector(const FVector& Position, float Scale)
{
	return FIntVector(FMath::RoundToInt(Position.X / Scale), FMath::RoundToInt(Position.Y / Scale), FMath::RoundToInt(Position.Z / Scale));
}
inline FVector GetTangentYFromX(const FVector& TangentX)
{
	FVector Y = FVector::UpVector ^ TangentX;
	float Size = Y.Size();
	//Sin(1)
	return Size < 0.0174524f ? (TangentX ^ FVector::ForwardVector).GetSafeNormal() : Y / Size;
}
inline FVector GetTangentYFromZ(const FVector& TangentZ)
{
	FVector Y = TangentZ ^ FVector::ForwardVector;
	float Size = Y.Size();
	//Sin(1)
	return Size < 0.0174524f ? (FVector::UpVector ^ TangentZ).GetSafeNormal() : Y / Size;
}
inline FMatrix GetTransform(const FVector& InPosition, const FVector& InDirection, float ScaleY = 1)
{
	FVector Y = GetTangentYFromX(InDirection);
	return FMatrix(InDirection, Y*ScaleY, (InDirection ^ Y).GetSafeNormal(), InPosition);
}
inline FMatrix GetPlaneTransform(const FVector& InPosition, const FVector& InNormal)
{
	FVector Y = GetTangentYFromZ(InNormal);
	return FMatrix((Y ^ InNormal).GetSafeNormal(), Y, InNormal, InPosition);
}

USTRUCT(BlueprintType)
struct ARTERIES_API FArteriesPoint : public FArteriesElement
{
	FArteriesPoint() {}
	FArteriesPoint(const FVector& InPosition) :Position(InPosition) {}
	GENERATED_USTRUCT_BODY()
	virtual UStruct* GetStruct() { return FArteriesPoint::StaticStruct(); }
	virtual struct FArteriesPoint* ToPoint() { return this; }
	void Serialize(FArchive& Ar, UObject* Owner, int Index);
	void GetSmoothTangentsForPrimitive(const FArteriesPrimitive* Primitive, FVector& X, FVector& Y, FVector& Z, float SmoothThreshold);
	FIntVector IntKey(float Scale) const { return GetIntVector(Position, Scale); }
	bool IsRoundCorner();
	bool IsSelfIntersect() const
	{
		return Links.Num() > 2 && GetPrimitives().Num() == 1;
	}
	int NumLinks() { return Links.Num(); }
	FArteriesLink* AddLink(FArteriesPoint* Target, FArteriesPrimitive* Primitive, bool bInverse);
	TArray<FArteriesLink*> GetLinks(const FArteriesPoint* Target, const FArteriesPrimitive* Primitive, int Direction);
	TArray<FArteriesLink*> GetSortedLinks();
	TArray<FArteriesPoint*> GetTargets();
	TArray<FArteriesPrimitive*> GetPrimitives() const;
	int DeleteLinks(const FArteriesPoint* Target, const FArteriesPrimitive* Primitive, int Direction);
	void SortLinksForRoads();
	int GetLinkIndex(FArteriesLink* Link);
	FArteriesLink* GetPrevLink(FArteriesLink* Link);
	FArteriesLink* GetNextLink(FArteriesLink* Link);
	FTransform GetTransform() const
	{
		if (HasVec3(AVN_TangentX))
		{
			FVector X = GetVec3(AVN_TangentX);
			FVector Y = GetTangentYFromX(X);
			return FTransform(X, Y, (X ^ Y).GetSafeNormal(), Position);
		}
		return FTransform(Position);
	}
	float GetLinkLength(const FArteriesLink* Link, FVector* OutDirection=NULL)
	{
		FVector Direction = Link->Target->Position - Position;
		float Size = Direction.Size();
		if (OutDirection)
			*OutDirection = Direction / Size;
		return Size;
	}
	FVector GetLinkDirection(const FArteriesLink* Link) { return (Link->Target->Position - Position).GetSafeNormal(); }
	float RadianBetween(FArteriesLink* PrevLink, FArteriesLink* NextLink);
	float GetCutDistance(FArteriesLink* Link, float DefaultWidth);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ArteriesPoint)
	FVector Position;
	TIndirectArray<FArteriesLink> Links;
};

class FArteriesPointArray : public TIndirectArray<FArteriesPoint>
{
public:
};
