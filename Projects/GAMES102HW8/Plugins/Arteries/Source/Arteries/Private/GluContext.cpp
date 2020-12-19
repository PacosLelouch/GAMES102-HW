// Author: LiJiayu (JerryLi)
// Mail: lijiayu83@gmail.com (fullike@163.com)
// Copyright 2019. All Rights Reserved.

#include "GluContext.h"
#pragma warning(disable: 4191)

void GluBaseTriangulator::Begin(unsigned int Winding, bool bFlip)
{
	Tess = gluNewTess();
	gluTessProperty(Tess, GLU_TESS_WINDING_RULE, Winding);
	gluTessNormal(Tess, 0, 0, bFlip ? -1 : 1);
	gluTessCallback(Tess, GLU_TESS_VERTEX_DATA, (GLvoid(*) ()) &GluBaseTriangulator::GluVertex);
	gluTessCallback(Tess, GLU_TESS_BEGIN_DATA, (GLvoid(*) ()) &GluBaseTriangulator::GluBegin);
	gluTessCallback(Tess, GLU_TESS_END_DATA, (GLvoid(*) ()) &GluBaseTriangulator::GluEnd);
	gluTessCallback(Tess, GLU_TESS_COMBINE_DATA, (GLvoid(*) ()) &GluBaseTriangulator::GluCombine);
	gluTessBeginPolygon(Tess, this);
}
void GluBaseTriangulator::BeginContour()
{
	gluTessBeginContour(Tess);
}
void GluBaseTriangulator::Vertex(const FVector2D& Position, int Index)
{
	gluTessVertex(Tess, TessVertex(Position).Vertex, (void*)PackIndex(Index));
}
void GluBaseTriangulator::Vertex(const FVector& Position, int Index)
{
	gluTessVertex(Tess, TessVertex(Position).Vertex, (void*)PackIndex(Index));
}
void GluBaseTriangulator::EndContour()
{
	gluTessEndContour(Tess);
}
void GluBaseTriangulator::End()
{
	gluTessEndPolygon(Tess);
	gluDeleteTess(Tess);
}

void GluBaseTriangulator::GluVertex(int64 Index, GluBaseTriangulator* Triangulator)
{
	Triangulator->PrimitiveIndices.Add(UnPackIndex(Index));
}
void GluBaseTriangulator::GluBegin(uint32 Type, GluBaseTriangulator* Triangulator)
{
	Triangulator->Type = Type;
}
void GluBaseTriangulator::GluEnd(GluBaseTriangulator* Triangulator)
{
	Triangulator->Flush();
}
void GluBaseTriangulator::Flush()
{
	switch (Type)
	{
	case GL_TRIANGLES:
		for (int i = 0; i < PrimitiveIndices.Num(); i++)
			AddIndex(PrimitiveIndices[i]);
		break;
	case GL_TRIANGLE_STRIP:
		for (int i = 0; i < PrimitiveIndices.Num() - 2; i++)
		{
			if (i % 2)
			{
				AddIndex(PrimitiveIndices[i + 1]);
				AddIndex(PrimitiveIndices[i]);
				AddIndex(PrimitiveIndices[i + 2]);
			}
			else
			{
				AddIndex(PrimitiveIndices[i]);
				AddIndex(PrimitiveIndices[i + 1]);
				AddIndex(PrimitiveIndices[i + 2]);
			}
		}
		break;
	case GL_TRIANGLE_FAN:
		for (int i = 1; i < PrimitiveIndices.Num() - 1; i++)
		{
			AddIndex(PrimitiveIndices[0]);
			AddIndex(PrimitiveIndices[i]);
			AddIndex(PrimitiveIndices[i + 1]);
		}
		break;
	}
	PrimitiveIndices.Reset();
}
void GluBaseTriangulator::GluCombine(const double NewVertex[3], int64 InData[4], const float InWeight[4], int64* OutData, GluBaseTriangulator* Triangulator)
{
	if (!ValidIndex(InData[2]) || !ValidIndex(InData[3]))
	{
		*OutData = InData[0];
		return;
	}
	int NewIndex = Triangulator->AddVertex(InData, InWeight);
	*OutData = PackIndex(NewIndex);
}
int GluSimpleTriangulator::AddVertex(int64 InData[4], const float InWeight[4])
{
	int NewIndex = Vertices.AddDefaulted();
	FVector2D& v0 = Vertices[UnPackIndex(InData[0])];
	FVector2D& v1 = Vertices[UnPackIndex(InData[1])];
	FVector2D& v2 = Vertices[UnPackIndex(InData[2])];
	FVector2D& v3 = Vertices[UnPackIndex(InData[3])];
	Vertices[NewIndex] = v0 * InWeight[0] + v1 * InWeight[1] + v2 * InWeight[2] + v3 * InWeight[3];
	return NewIndex;
}
int GluTriangulator::AddVertex(int64 InData[4], const float InWeight[4])
{
	int NewIndex = Section.ProcVertexBuffer.AddDefaulted();
	FProcMeshVertex& v0 = Section.ProcVertexBuffer[UnPackIndex(InData[0])];
	FProcMeshVertex& v1 = Section.ProcVertexBuffer[UnPackIndex(InData[1])];
	FProcMeshVertex& v2 = Section.ProcVertexBuffer[UnPackIndex(InData[2])];
	FProcMeshVertex& v3 = Section.ProcVertexBuffer[UnPackIndex(InData[3])];
	FProcMeshVertex& v = Section.ProcVertexBuffer[NewIndex];
	v.Position = v0.Position * InWeight[0] + v1.Position * InWeight[1] + v2.Position * InWeight[2] + v3.Position * InWeight[3];
	v.Tangent = v0.Tangent;
	v.Normal = v0.Normal;
	v.UV0 = v0.UV0 * InWeight[0] + v1.UV0 * InWeight[1] + v2.UV0 * InWeight[2] + v3.UV0 * InWeight[3];
	return NewIndex;
}