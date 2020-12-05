// Author: LiJiayu (JerryLi)
// Mail: lijiayu83@gmail.com (fullike@163.com)
// Copyright 2019. All Rights Reserved.

#pragma once
#include "ArteriesElement.generated.h"

#define IMPLEMENT_TYPE_FUNC(Type,Name)													\
	bool Has##Name(const FName& Key) const {return Name##Values.Contains(Key);}			\
	Type Get##Name(const FName& Key) const {return Name##Values.Contains(Key) ? Name##Values[Key] : Type();}		\
	void Set##Name(const FName& Key, const Type& Value){Name##Values.Add(Key, Value);}		\
	void Del##Name(const FName& Key){Name##Values.Remove(Key);}								\
	bool Has##Name##ANSI(const char* Key) const {return Name##Values.Contains(Key);}		\
	Type Get##Name##ANSI(const char* Key) const {return Name##Values.Contains(Key) ? Name##Values[Key] : Type();}	\
	void Set##Name##ANSI(const char* Key, const Type& Value){Name##Values.Add(Key, Value);}	\
	void Del##Name##ANSI(const char* Key){Name##Values.Remove(Key);}

USTRUCT()
struct ARTERIES_API FArteriesElement
{
	GENERATED_USTRUCT_BODY()
	FArteriesElement() {}
	virtual ~FArteriesElement() {}
	virtual UStruct* GetStruct() { return NULL; }
	virtual struct FArteriesPoint* ToPoint() { return NULL; }
	virtual struct FArteriesPrimitive* ToPrimitive() { return NULL; }

	void CopyValues(const FArteriesElement& Other, FTransform* Transform = NULL)
	{
		IntValues = Other.IntValues;
		FloatValues = Other.FloatValues;
		DoubleValues = Other.DoubleValues;
		Vec2Values = Other.Vec2Values;
		Vec3Values = Other.Vec3Values;
		StrValues = Other.StrValues;
		if (Transform)
		{
			for (auto It = Vec3Values.CreateIterator(); It; ++It)
				It->Value = Transform->TransformVector(It->Value);
		}
	}

	IMPLEMENT_TYPE_FUNC(int, Int)
	IMPLEMENT_TYPE_FUNC(float, Float)
	IMPLEMENT_TYPE_FUNC(double, Double)
	IMPLEMENT_TYPE_FUNC(FVector2D, Vec2)
	IMPLEMENT_TYPE_FUNC(FVector, Vec3)
	IMPLEMENT_TYPE_FUNC(FString, Str)

	UPROPERTY(EditAnywhere, Category = ArteriesElement)
	TMap<FName, int> IntValues;
	UPROPERTY(EditAnywhere, Category = ArteriesElement)
	TMap<FName, float> FloatValues;
	UPROPERTY(EditAnywhere, Category = ArteriesElement)
	TMap<FName, FVector2D> Vec2Values;
	UPROPERTY(EditAnywhere, Category = ArteriesElement)
	TMap<FName, FVector> Vec3Values;
	UPROPERTY(EditAnywhere, Category = ArteriesElement)
	TMap<FName, FString> StrValues;
	UPROPERTY(EditAnywhere, Category = ArteriesElement)
	TMap<FName, double> DoubleValues;

private:
	FArteriesElement(const FArteriesElement &) {} // ×èÖ¹copying  
//	FArteriesElement& operator=(const FArteriesElement &) { return *this; }
};

#define AVN_TangentX	"TangentX"
#define AVN_TangentY	"TangentY"
#define AVN_TangentZ	"TangentZ"
#define AVN_Color		"Color"
#define AVN_UV0			"UV0"
#define AVN_UV1			"UV1"
#define AVN_UV2			"UV2"
#define AVN_UV3			"UV3"
#define AVN_UVScale		"UVScale"
#define AVN_EdgeDist	"EdgeDist"
#define AVN_OuterID		"OuterID"
#define AVN_Smooth		"Smooth"
#define AVN_Collision	"Collision"
#define AVN_Visible		"Visible"