// Author: LiJiayu (JerryLi)
// Mail: lijiayu83@gmail.com (fullike@163.com)
// Copyright 2019. All Rights Reserved.

#pragma once
#include "ProceduralMeshComponent.h"
#include "../GluTess/glutess.h"
#define TESS_WINDING_ODD            100130
#define TESS_WINDING_NONZERO        100131
#define TESS_WINDING_POSITIVE       100132
#define TESS_WINDING_NEGATIVE       100133
#define TESS_WINDING_ABS_GEQ_TWO    100134

struct TessVertex
{
	TessVertex(const FVector2D& P)
	{
		Vertex[0] = P.X;
		Vertex[1] = P.Y;
		Vertex[2] = 0;
	}
	TessVertex(const FVector& P)
	{
		Vertex[0] = P.X;
		Vertex[1] = P.Y;
		Vertex[2] = P.Z;
	}
	double Vertex[3];
};
class ARTERIES_API GluBaseTriangulator
{
public:
	virtual ~GluBaseTriangulator() {}
	void Begin(unsigned int Winding = TESS_WINDING_ODD, bool bFlip = false);
	void BeginContour();
	void Vertex(const FVector2D& Position, int Index);
	void Vertex(const FVector& Position, int Index);
	void EndContour();
	void End();
protected:
	static void GluVertex(int64 Index, GluBaseTriangulator* Triangulator);
	static void GluBegin(uint32 Type, GluBaseTriangulator* Triangulator);
	static void GluEnd(GluBaseTriangulator* Triangulator);
	static void GluCombine(const double NewVertex[3], int64 InData[4], const float InWeight[4], int64* OutData, GluBaseTriangulator* Triangulator);
	static int64 PackIndex(int Index)
	{
		return Index | 0xFFFFFFFF00000000;
	}
	static bool ValidIndex(int64 Index)
	{
		return(Index & 0xFFFFFFFF00000000) != 0;
	}
	static int UnPackIndex(int64 Index)
	{
		return Index & 0x00000000FFFFFFFF;
	}
	virtual int AddVertex(int64 InData[4], const float InWeight[4]) = 0;
	virtual void AddIndex(uint32 Index) = 0;
	void Flush();
	GLUtesselator* Tess;
	uint32 Type;
	TArray<int> PrimitiveIndices;
};
class ARTERIES_API GluSimpleTriangulator : public GluBaseTriangulator
{
public:
	GluSimpleTriangulator(TArray<uint32>& InIndices, TArray<FVector2D>& InVertices) :Indices(InIndices), Vertices(InVertices){}
protected:
	virtual int AddVertex(int64 InData[4], const float InWeight[4]);
	virtual void AddIndex(uint32 Index) { Indices.Add(Index); }
	TArray<uint32>& Indices;
	TArray<FVector2D>& Vertices;
};
class ARTERIES_API GluTriangulator : public GluBaseTriangulator
{
public:
	GluTriangulator(FProcMeshSection& InSection) :Section(InSection) {}
protected:
	virtual int AddVertex(int64 InData[4], const float InWeight[4]);
	virtual void AddIndex(uint32 Index) { Section.ProcIndexBuffer.Add(Index); }
	FProcMeshSection& Section;
};