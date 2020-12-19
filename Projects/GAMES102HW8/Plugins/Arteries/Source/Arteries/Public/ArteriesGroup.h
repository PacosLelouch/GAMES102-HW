// Author: LiJiayu (JerryLi)
// Mail: lijiayu83@gmail.com (fullike@163.com)
// Copyright 2019. All Rights Reserved.

#pragma once
#include "ArteriesGroup.generated.h"

struct FArteriesPoint;
struct FArteriesPrimitive;
class UArteriesObject;

USTRUCT()
struct ARTERIES_API FArteriesGroup
{
	GENERATED_USTRUCT_BODY()
	UPROPERTY(EditAnywhere, Category = ArteriesGroup)
	TArray<int> IDs;
};
USTRUCT()
struct ARTERIES_API FArteriesPointGroup : public FArteriesGroup
{
	GENERATED_USTRUCT_BODY()
	void Add(FArteriesPoint* Point);
	void Remove(FArteriesPoint* Point);
	void PreSave(UArteriesObject* Object, TMap<FArteriesPoint*, int>& PointIDs);
	void PostLoad(UArteriesObject* Object);
	TArray<FArteriesPoint*> Points;
};
USTRUCT()
struct ARTERIES_API FArteriesPrimitiveGroup : public FArteriesGroup
{
	GENERATED_USTRUCT_BODY()
	void Add(FArteriesPrimitive* Primitive);
	void Remove(FArteriesPrimitive* Primitive);
	void PreSave(UArteriesObject* Object, TMap<FArteriesPrimitive*, int>& PrimitiveIDs);
	void PostLoad(UArteriesObject* Object);
	TArray<FArteriesPrimitive*> Primitives;
};
USTRUCT()
struct ARTERIES_API FArteriesEdgeGroup
{
	GENERATED_USTRUCT_BODY()
	void Add(FArteriesPoint* Key, FArteriesPoint* Value);
	void PreSave(UArteriesObject* Object, TMap<FArteriesPoint*, int>& PointIDs);
	void PostLoad(UArteriesObject* Object);
	UPROPERTY(EditAnywhere, Category = ArteriesGroup)
	TArray<FIntPoint> IDs;
	TArray<TPair<FArteriesPoint*, FArteriesPoint*>> Edges;
};