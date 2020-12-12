// Author: LiJiayu (JerryLi)
// Mail: lijiayu83@gmail.com (fullike@163.com)
// Copyright 2019. All Rights Reserved.

#include "ArteriesObject.h"
#include "ArteriesUtil.h"
#include "ArteriesActor.h"
#include "Arteries.h"
#include "ArteriesCustomVersion.h"
#include "GluContext.h"
#include "LuaContext.h"
#include "Engine/StaticMesh.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Developer/Profiler/Public/ProfilerCommon.h"
#define JC_VORONOI_IMPLEMENTATION
#include "Voronoi/jc_voronoi.h"

#define ARTERIES_UFUNCTION_RETURN(x)	return SubmitResult(x/*,ANSI_TO_TCHAR(__func__)*/);

UArteriesObject::UArteriesObject(const FObjectInitializer& ObjectInitializer) :UObject(ObjectInitializer)
{

}
UArteriesObject* UArteriesObject::New_Impl()
{
	UArteriesObject* Obj = NewObject<UArteriesObject>(GetTransientPackage());
	return Obj;
}
UArteriesObject* UArteriesObject::SubmitResult(UArteriesObject* Obj)
{
#if WITH_EDITORONLY_DATA
	if (AArteriesActor* Owner = FArteriesRunnable::GetThreadOwner())
	{
		const FFrame* StackFrame = FBlueprintExceptionTracker::Get().ScriptStack.Last();
		const int32 BreakpointOffset = StackFrame->Code - StackFrame->Node->Script.GetData() - 1;
		UClass* FunctionOwner = StackFrame->Node->GetOwnerClass();
		UBlueprintGeneratedClass* GeneratedClass = Cast<UBlueprintGeneratedClass>(FunctionOwner);
		UEdGraphNode* BlueprintNode = GeneratedClass->DebugData.FindSourceNodeFromCodeLocation(StackFrame->Node, BreakpointOffset, true);
		Owner->OnResultSubmitted(BlueprintNode, Obj);
	}
#endif
	return Obj;
}
void UArteriesObject::GetTangents(int FaceIndex, FVector* X, FVector* Y, FVector* Z)
{
	int ZIndex[6] = { 0,0,1,1,2,2 };
	int ZSign[6] = { 1,-1,1,-1,1,-1 };
	int XIndex[6] = { 1,1,0,0,1,1 };
	int XSign[6] = { -1,1,1,-1,1,-1 };
	FVector TangentZ = FVector::ZeroVector;
	TangentZ[ZIndex[FaceIndex]] = ZSign[FaceIndex];
	FVector TangentX = FVector::ZeroVector;
	TangentX[XIndex[FaceIndex]] = XSign[FaceIndex];
	FVector TangentY = (TangentZ ^ TangentX).GetSafeNormal();
	if (X)*X = TangentX;
	if (Y)*Y = TangentY;
	if (Z)*Z = TangentZ;
}
FTransform UArteriesObject::Flatten(const FTransform& Transform)
{
	FVector Loc = Transform.GetLocation();
	FVector X = Transform.GetScaledAxis(EAxis::X);
	X.Z = 0;
	X = X.GetSafeNormal();
	return FTransform(X, (FVector::UpVector ^ X).GetSafeNormal(), FVector::UpVector, Loc);
}
UArteriesObject* UArteriesObject::New()
{
	UArteriesObject* Obj = New_Impl();
	ARTERIES_UFUNCTION_RETURN(Obj)
}
UArteriesObject* UArteriesObject::Line(FVector Origin, FVector Direction, float Length, int NumPoints)
{
	UArteriesObject* Obj = New_Impl();
	Obj->AddLine(Origin, Direction, Length, NumPoints);
	ARTERIES_UFUNCTION_RETURN(Obj)
}
UArteriesObject* UArteriesObject::Grid(FVector Origin, FRotator Rotation, FVector2D Size, int NumPointsX, int NumPointsY)
{
	UArteriesObject* Obj = New_Impl();
	Obj->AddGrid(Origin, Size, NumPointsX, NumPointsY);
	Obj->Transform_Impl(FTransform(Rotation));
	ARTERIES_UFUNCTION_RETURN(Obj)
}
UArteriesObject* UArteriesObject::Circle(FVector Origin, FRotator Rotation, FVector2D Radius, int NumPoints)
{
	UArteriesObject* Obj = New_Impl();
	Obj->AddCircle(Origin, Radius, NumPoints);
	Obj->Transform_Impl(FTransform(Rotation));
	ARTERIES_UFUNCTION_RETURN(Obj)
}

UArteriesObject* UArteriesObject::Sphere(FVector Origin, FRotator Rotation, FVector Radius, int Rows, int Columns)
{
	UArteriesObject* Obj = New_Impl();
	Obj->AddPoint(Origin + FVector(0, 0, -Radius.Z));// , FVector2D(0, 0));
	Obj->AddPoint(Origin + FVector(0, 0, +Radius.Z));// , FVector2D(0, 100));
	Rows -= 1;
	for (int i = 0; i <= Columns; i++)
	{
		float AlphaX = float(i) / Columns;
		float Phi = FMath::Lerp(0.f, PI * 2, AlphaX);
		float CosPhi = FMath::Cos(Phi);
		float SinPhi = FMath::Sin(Phi);
		for (int j = 0; j <= Rows; j++)
		{
			float AlphaY = float(j) / Rows;
			float Theta = FMath::Lerp(PI*1.5f, PI*0.5f, AlphaY);
			float CosTheta = FMath::Cos(Theta);
			float SinTheta = FMath::Sin(Theta);
			FVector Position = Origin + FVector(Radius.X * CosTheta * CosPhi, Radius.Y * CosTheta * SinPhi, Radius.Z * SinTheta);
		//	FVector TangentX =-FVector(-Radius.X * SinPhi, Radius.Y * CosPhi, 0).GetSafeNormal();
		//	FVector TangentY =-FVector(-Radius.X * SinTheta * CosPhi, -Radius.Y * SinTheta * SinPhi, Radius.Z * CosTheta).GetSafeNormal();
			if (i != Columns && j != 0 && j != Rows)
				Obj->AddPoint(Position);// , FVector2D(AlphaX * 100, AlphaY * 100));
		}
	}
	int BasePoint = 2;
	int BaseVertex = 0;
	int NumPoints = Rows - 1;
	int NumVertices = Rows + 1;
	for (int i = 0; i < Columns; i++)
	{
		int PointThisLine = BasePoint + i * NumPoints;
		int PointNextLine = (i == Columns - 1) ? BasePoint : PointThisLine + NumPoints;
		for (int j = 0; j < Rows; j++)
		{
			FArteriesPrimitive* Prim = Obj->AddPrimitive();
			int P0 = (j == Rows - 1) ? 1 : PointNextLine + (j);
			int P1 = (j == Rows - 1) ? 1 : PointThisLine + (j);
			int P2 = (j == 0) ? 0 : PointThisLine + (j-1);
			int P3 = (j == 0) ? 0 : PointNextLine + (j-1);
			if (j == Rows - 1)
			{
				Prim->Add(&Obj->Points[P1]);
				Prim->Add(&Obj->Points[P2]);
				Prim->Add(&Obj->Points[P3]);
			}
			else if (j == 0)
			{
				Prim->Add(&Obj->Points[P0]);
				Prim->Add(&Obj->Points[P1]);
				Prim->Add(&Obj->Points[P2]);
			}
			else
			{
				Prim->Add(&Obj->Points[P0]);
				Prim->Add(&Obj->Points[P1]);
				Prim->Add(&Obj->Points[P2]);
				Prim->Add(&Obj->Points[P3]);
			}
			Prim->MakeClose();
		}
	}
	Obj->CleanPoints();
	Obj->Transform_Impl(FTransform(Rotation));
	ARTERIES_UFUNCTION_RETURN(Obj)
}

UArteriesObject* UArteriesObject::Tube(FVector Origin, FRotator Rotation, FVector2D Radius, float Height, int Rows, int Columns)
{
	UArteriesObject* Profile = New_Impl();
	FVector End(0, Radius.Y - Radius.X, Height);
	float Length = End.Size();
	Profile->AddLine(FVector::ZeroVector, End / Length, Length, Rows);
	UArteriesObject* Backbone = New_Impl();
	Backbone->AddCircle(Origin, FVector2D(Radius.X, Radius.X), Columns);
	UArteriesObject* Obj = Profile->Sweep_Impl(Backbone);
	Obj->Transform_Impl(FTransform(Rotation));
	ARTERIES_UFUNCTION_RETURN(Obj)
}

UArteriesObject* UArteriesObject::Torus(FVector Origin, FRotator Rotation, FVector2D Radius, int Rows, int Columns)
{
	UArteriesObject* Profile = New_Impl();
	Profile->AddCircle(FVector::ZeroVector, FVector2D(Radius.Y, Radius.Y), Rows);
	Profile->Transform_Impl(FTransform(FRotator(90,0,0)));
	UArteriesObject* Backbone = New_Impl();
	Backbone->AddCircle(Origin, FVector2D(Radius.X, Radius.X), Columns);
	UArteriesObject* Obj = Profile->Sweep_Impl(Backbone);
	Obj->Transform_Impl(FTransform(Rotation));
	ARTERIES_UFUNCTION_RETURN(Obj)
}

UArteriesObject* UArteriesObject::Box(FVector Origin, FRotator Rotation, FVector Size, int NumPointsX, int NumPointsY, int NumPointsZ)
{
	UArteriesObject* Obj = New_Impl();
	FVector Half = Size / 2;
	FArteriesPointMapper Mapper(Obj);
	FName Groups[6] = {"PosX", "NegX", "PosY", "NegY", "PosZ", "NegZ"};
	for (int i = 0; i < 6; i++)
	{
		FArteriesPrimitive* Prim = Obj->AddPrimitive();
		FVector TangentX, TangentY, TangentZ;
		GetTangents(i, &TangentX, &TangentY, &TangentZ);
		float X = FMath::Abs(TangentX | Half);
		float Y = FMath::Abs(TangentY | Half);
		float Z = FMath::Abs(TangentZ | Half);
		FMatrix TangentToWorld(TangentX, TangentY, TangentZ, TangentZ * Z);
		FVector2D UV0(-X, -Y);
		FVector2D UV1(+X, -Y);
		FVector2D UV2(+X, +Y);
		FVector2D UV3(-X, +Y);
		Prim->Add(Mapper.GetPoint(TangentToWorld.TransformPosition(FVector(UV0, 0))));
		Prim->Add(Mapper.GetPoint(TangentToWorld.TransformPosition(FVector(UV1, 0))));
		Prim->Add(Mapper.GetPoint(TangentToWorld.TransformPosition(FVector(UV2, 0))));
		Prim->Add(Mapper.GetPoint(TangentToWorld.TransformPosition(FVector(UV3, 0))));
		Prim->MakeClose();
		Obj->SetPrimitiveGroup(Prim, Groups[i], 1);
	}
	int NumSegments[3] = { NumPointsX - 1,NumPointsY - 1,NumPointsZ - 1 };
	for (int i = 0; i < 3; i++)
	{
		FVector TangentZ;
		GetTangents(i*2, NULL, NULL, &TangentZ);
		for (int j = 1; j < NumSegments[i]; j++)
		{
			float Alpha = float(j) / NumSegments[i];
			float Dist = FMath::Lerp(-Half[i], Half[i], Alpha);
			FName& PosGroup = Groups[i * 2];
			FName& NegGroup = Groups[i * 2 + 1];
			Obj->Clip_Impl(PosGroup.ToString(), FPlane(TangentZ, Dist), PosGroup, PosGroup);
			Obj->Clip_Impl(NegGroup.ToString(), FPlane(TangentZ, Dist), NegGroup, NegGroup);
		}
	}
	Obj->Transform_Impl(FTransform(Rotation, Origin));
	ARTERIES_UFUNCTION_RETURN(Obj)
}

UArteriesObject* UArteriesObject::Merge(UArteriesObject* Obj0, UArteriesObject* Obj1, UArteriesObject* Obj2, UArteriesObject* Obj3, UArteriesObject* Obj4)
{
	UArteriesObject* Obj = New_Impl();
	if (Obj0)Obj->Merge_Impl(Obj0);
	if (Obj1)Obj->Merge_Impl(Obj1);
	if (Obj2)Obj->Merge_Impl(Obj2);
	if (Obj3)Obj->Merge_Impl(Obj3);
	if (Obj4)Obj->Merge_Impl(Obj4);
	ARTERIES_UFUNCTION_RETURN(Obj)
}

UArteriesObject* UArteriesObject::MergeArray(const TArray<UArteriesObject*>& Objs)
{
	UArteriesObject* Obj = New_Impl();
	for (UArteriesObject* O : Objs)
		Obj->Merge_Impl(O);
	ARTERIES_UFUNCTION_RETURN(Obj)
}
UArteriesObject* UArteriesObject::Add(UObject* Source, FTransform Transform)
{
	if (UArteriesObject* Object = Cast<UArteriesObject>(Source))
		Merge_Impl(Object, &Transform);
	else if (UStaticMesh* Mesh = Cast<UStaticMesh>(Source))
		InstancesMap.FindOrAdd(Mesh).Add(Transform);
	ARTERIES_UFUNCTION_RETURN(this)
}
UArteriesObject* UArteriesObject::Arc(const FString& Groups, int NumSegments, float Angle)
{
	UArteriesObject* Obj = Copy_Impl();
	TSet<TPair<FArteriesPoint*, FArteriesPoint*>> Edges = Obj->GetEdges(Groups);
	for (TPair<FArteriesPoint*, FArteriesPoint*>& Edge : Edges)
	{
		FArteriesPoint* Start = Edge.Key;
		FArteriesPoint* End = Edge.Value;
		FVector TangentX = Start->Position - End->Position;
		float Size = TangentX.Size();
		TangentX /= Size;
		float Half = Size / 2;
		float Radian = FMath::DegreesToRadians(Angle);
		float Sign;
		FVector TangentY;
		if (Radian > 0)
		{
			if (Radian > PI)
			{
				TangentY = -GetTangentYFromX(TangentX);
				Sign = -1;
			}
			else
			{
				TangentY = GetTangentYFromX(TangentX);
				Sign = 1;
			}
		}
		else
		{
			if (Radian < -PI)
			{
				TangentY = GetTangentYFromX(TangentX);
				Sign = -1;
			}
			else
			{
				TangentY = -GetTangentYFromX(TangentX);
				Sign = 1;
			}
		}
		float Radius = FMath::Abs(Half / FMath::Sin(Radian / 2)) * Sign;
		float HalfRadian = FMath::Abs(Radian) / 2 * Sign;
		FVector Center = (Start->Position + End->Position) / 2 - TangentY * FMath::Sqrt(Radius*Radius - Half * Half);
		FVector FaceNormal = (TangentX ^ TangentY).GetSafeNormal();
		TArray<FArteriesPoint*> NewPoints;
		for (int i = 1; i < NumSegments; i++)
		{
			float Alpha = float(i) / NumSegments;
			float R = FMath::Lerp(-HalfRadian, HalfRadian, Alpha);
			FVector Direction = FQuat(FaceNormal, R).RotateVector(TangentY).GetSafeNormal();
			NewPoints.Add(Obj->AddPoint(Center + Direction * Radius));
		}
		TArray<FArteriesLink*> Links = Start->GetLinks(End, NULL, -1);
		for (FArteriesLink* Link : Links)
		{
			FArteriesPrimitive* Prim = Link->Primitive;
			int Index = Link->Inverse ? Prim->Find(Edge.Value) : Prim->Find(Edge.Key);
			for (int i = 0; i < NewPoints.Num(); i++)
				Prim->Insert(NewPoints[i], Index + i + 1);
		}
	}
	ARTERIES_UFUNCTION_RETURN(Obj)
}
UArteriesObject* UArteriesObject::Copy()
{
	UArteriesObject* Obj = Copy_Impl();
	ARTERIES_UFUNCTION_RETURN(Obj)
}
UArteriesObject* UArteriesObject::CopyAndTransform(int NumCopies, FTransform Transform)
{
	UArteriesObject* Obj = Copy_Impl();
	FTransform Trans = FTransform::Identity;
	for (int i = 1; i < NumCopies; i++)
	{
		Trans *= Transform;
		Obj->Merge_Impl(this, &Trans);
	}
	ARTERIES_UFUNCTION_RETURN(Obj)
}
UArteriesObject* UArteriesObject::CopyToPoints(const FString& Groups, UObject* Source, FTransform LocalTransform)
{
	UArteriesObject* Obj = New_Impl();
	TSet<FArteriesPoint*> Sets = GetPoints(Groups);
	for (FArteriesPoint* Point : Sets)
	{
		FTransform Transform = Point->GetTransform();
		FTransform Trans = LocalTransform * Transform;
		if (UArteriesObject* Object = Cast<UArteriesObject>(Source))
			Obj->Merge_Impl(Object, &Trans);
		else if (UStaticMesh* Mesh = Cast<UStaticMesh>(Source))
			Obj->InstancesMap.FindOrAdd(Mesh).Add(Trans);
	}
	ARTERIES_UFUNCTION_RETURN(Obj)
}
UArteriesObject* UArteriesObject::Execute(const EArteriesGroupType ForEach, const FString& Code)
{
	UArteriesObject* Obj = Copy_Impl();
	ULuaContext* LuaContext = FArteriesRunnable::GetLuaContext();
	checkf(LuaContext, TEXT("Can't execute lua code in GameThread"));
	auto ANSICode = StringCast<ANSICHAR>(*Code);
	LuaContext->Begin();
	LuaContext->Push(Obj, "obj");
	switch (ForEach)
	{
	case EArteriesGroupType::Point:
	{
		int NumPoints = Obj->Points.Num();
		for (int i = 0; i < NumPoints; i++)
		{
			LuaContext->Push(&Obj->Points[i], "self");
			LuaContext->Execute(ANSICode.Get());
		}
		break;
	}
	case EArteriesGroupType::Primitive:
	{
		int NumPrimitives = Obj->Primitives.Num();
		for (int i = 0; i < NumPrimitives; i++)
		{
			LuaContext->Push(&Obj->Primitives[i], "self");
			LuaContext->Execute(ANSICode.Get());
		}
		break;
	}
	case EArteriesGroupType::Object:
		LuaContext->Execute(ANSICode.Get());
		break;
	}
	LuaContext->End();
	ARTERIES_UFUNCTION_RETURN(Obj)
}
UArteriesObject* UArteriesObject::Carve(EArteriesAlignType StartAlign, float StartU, EArteriesAlignType EndAlign, float EndU)
{
	UArteriesObject* Obj = New_Impl();
	FArteriesPointMapper Mapper(Obj);
	for (const FArteriesPrimitive& Primitive : Primitives)
	{
		TArray<float> Dists = Primitive.GetDists();
		float Length = Dists.Last();
		float Half = Length / 2;
		float StartDist=0, EndDist=Length;
		switch (StartAlign)
		{
		case EArteriesAlignType::Start:
			StartDist = StartU; break;
		case EArteriesAlignType::Center:
			StartDist = Half + StartU; break;
		case EArteriesAlignType::End:
			StartDist = Length + StartU; break;
		}
		switch (EndAlign)
		{
		case EArteriesAlignType::Start:
			EndDist = EndU; break;
		case EArteriesAlignType::Center:
			EndDist = Half + EndU; break;
		case EArteriesAlignType::End:
			EndDist = Length + EndU; break;
		}
		if (StartDist < Length && EndDist > 0)
		{
			int32 StartIndex = FMath::Max(0, FBinaryFindIndex::LessEqual(Dists, StartDist));
			int32 EndIndex = FMath::Min(Dists.Num() - 1, FBinaryFindIndex::GreaterEqual(Dists, EndDist));
			float StartAlpha = (StartDist - Dists[StartIndex]) / (Dists[StartIndex + 1] - Dists[StartIndex]);
			float EndAlpha = (EndDist - Dists[EndIndex - 1]) / (Dists[EndIndex] - Dists[EndIndex - 1]);
			auto CheckSinglePoint = [&](FArteriesPrimitive* NewPrim)
			{
				if (NewPrim->NumPoints() == 1)
				{
					NewPrim->MakeClose();
					NewPrim->SetVec3(AVN_TangentX, (Primitive.Points[EndIndex]->Position - Primitive.Points[StartIndex]->Position).GetSafeNormal());
				}
			};
			if (EndDist >= StartDist)
			{
				FArteriesPrimitive* NewPrim = Obj->AddPrimitive();
				NewPrim->Add(Mapper.GetPointLerp(Primitive.Points[StartIndex], Primitive.Points[StartIndex + 1], StartAlpha));
				for (int i = StartIndex + (StartAlpha >= 0); i <= EndIndex - (EndAlpha <= 1); i++)
					NewPrim->Add(Mapper.GetPoint(Primitive.Points[i]));
				NewPrim->Add(Mapper.GetPointLerp(Primitive.Points[EndIndex - 1], Primitive.Points[EndIndex], EndAlpha));
				CheckSinglePoint(NewPrim);
			}
			else
			{
				FArteriesPrimitive* NewPrim = Obj->AddPrimitive();
				NewPrim->Add(Mapper.GetPointLerp(Primitive.Points[StartIndex], Primitive.Points[StartIndex + 1], StartAlpha));
				for (int i = StartIndex + (StartAlpha >= 0); i < Primitive.NumPoints(); i++)
					NewPrim->Add(Mapper.GetPoint(Primitive.Points[i]));
				if (!Primitive.bClosed)
				{
					CheckSinglePoint(NewPrim);
					NewPrim = Obj->AddPrimitive();
				}
				for (int i = 0; i <= EndIndex - (EndAlpha <= 1); i++)
					NewPrim->Add(Mapper.GetPoint(Primitive.Points[i]));
				NewPrim->Add(Mapper.GetPointLerp(Primitive.Points[EndIndex - 1], Primitive.Points[EndIndex], EndAlpha));
				CheckSinglePoint(NewPrim);
			}
		}
	}
	ARTERIES_UFUNCTION_RETURN(Obj)
}
UArteriesObject* UArteriesObject::BreakPoints(const FString& Groups)
{
	UArteriesObject* Obj = New_Impl();
	FArteriesPointMapper Mapper(Obj);
	TSet<FArteriesPrimitive*> Sets = GetPrimitives(Groups);
	for (FArteriesPrimitive* Primitive : Sets)
	{
		for (int i = 0; i < Primitive->NumSegments(); i++)
		{
			FArteriesPrimitive* NewPrim = Obj->AddPrimitive();
			NewPrim->Add(Mapper.GetPoint(Primitive->GetPoint(i)));
			NewPrim->Add(Mapper.GetPoint(Primitive->NextPoint(i)));
		}
	}
	ARTERIES_UFUNCTION_RETURN(Obj)
}
TArray<jcv_point> Scatter_Impl(TArray<FVector>& Points, jcv_rect& bounding_box, int Seed, int Count, float Density, int Iterations)
{
	TArray<float> Areas;
	TArray<int> Triangles;
	float TotalArea = 0;
	int v0 = 0;
	for (int v1 = 1; v1 < Points.Num() - 1; v1++)
	{
		int v2 = v1 + 1;
		Triangles.Add(v0);
		Triangles.Add(v1);
		Triangles.Add(v2);
		TotalArea += GetTriangleArea(Points[v0], Points[v1], Points[v2]);
		Areas.Add(TotalArea);
	}
	TArray<jcv_point> Sites;
	if (Count == 0 && Density > 1.f) Count = FMath::RoundToInt(TotalArea / Density);
	for (int i = 0; i < Count; i++)
	{
		float Area = FMath::FRand() * TotalArea;
		const int32 Triangle = FBinaryFindIndex::GreaterEqual(Areas, Area) * 3;
		FVector Position = RandomPointInTriangle(Points[Triangles[Triangle]], Points[Triangles[Triangle + 1]], Points[Triangles[Triangle + 2]]);
		jcv_point& point = Sites[Sites.AddUninitialized()];
		point.x = Position.X;
		point.y = Position.Y;
	}

	for (int i = 0; i < Iterations; i++)
	{
		jcv_diagram diagram;
		memset(&diagram, 0, sizeof(jcv_diagram));
		jcv_diagram_generate(Sites.Num(), Sites.GetData(), &bounding_box, &diagram);
		const jcv_site* sites = jcv_diagram_get_sites(&diagram);
		for (int j = 0; j < diagram.numsites; j++)
		{
			TArray<FVector2D> Polygon;
			jcv_graphedge* graph_edge = sites[j].edges;
			while (graph_edge)
			{
				Polygon.Add(FVector2D(graph_edge->pos[0].x, graph_edge->pos[0].y));
				graph_edge = graph_edge->next;
			}
			jcv_point& point = Sites[j];
			FVector2D Centroid = CalculateCentroid(Polygon);
			point.x = Centroid.X;
			point.y = Centroid.Y;
		}
		jcv_diagram_free(&diagram);
	}
	return MoveTemp(Sites);
}
UArteriesObject* UArteriesObject::Scatter(const FString& Groups, int Seed, int Count, float Density, int Iterations)
{
	UArteriesObject* Obj = New_Impl();
	FMath::RandInit(Seed);
	TSet<FArteriesPrimitive*> Sets = GetPrimitives(Groups);
	for (const FArteriesPrimitive* Primitive : Sets)
	{
		if (Primitive->IsClosed())
		{
			((FArteriesPrimitive*)Primitive)->CalculateTangents();
			FMatrix LocalToWorld = Primitive->GetLocalToWorldTransform();
			FMatrix WorldToLocal = LocalToWorld.Inverse();

			TArray<FVector> Pts;
			FBox Box(EForceInit::ForceInit);
			for (FArteriesPoint* Point : Primitive->Points)
			{
				FVector LocalPosition = WorldToLocal.TransformPosition(Point->Position);
				Pts.Add(LocalPosition);
				Box += LocalPosition;
			}
			jcv_rect bounding_box = { { Box.Min.X, Box.Min.Y }, { Box.Max.X, Box.Max.Y } };

			TArray<jcv_point> Sites = Scatter_Impl(Pts, bounding_box, Seed, Count, Density, Iterations);
			for (jcv_point& Site : Sites)
				Obj->AddPoint(LocalToWorld.TransformPosition(FVector(Site.x, Site.y, 0)));
		}
	}
	ARTERIES_UFUNCTION_RETURN(Obj)
}
UArteriesObject* UArteriesObject::Voronoi(const FString& Groups, int Seed, int Count, float Density, int Iterations)
{
	UArteriesObject* Obj = New_Impl();
	FMath::RandInit(Seed);
	FArteriesPointMapper Mapper(Obj);
	TSet<FArteriesPrimitive*> Sets = GetPrimitives(Groups);
	for (const FArteriesPrimitive* Primitive : Sets)
	{
		if (Primitive->IsClosed())
		{
			((FArteriesPrimitive*)Primitive)->CalculateTangents();
			FMatrix LocalToWorld = Primitive->GetLocalToWorldTransform();
			FMatrix WorldToLocal = LocalToWorld.Inverse();

			TArray<FVector> Pts;
			FBox Box(EForceInit::ForceInit);
			for (FArteriesPoint* Point : Primitive->Points)
			{
				FVector LocalPosition = WorldToLocal.TransformPosition(Point->Position);
				Pts.Add(LocalPosition);
				Box += LocalPosition;
			}
			jcv_rect bounding_box = { { Box.Min.X, Box.Min.Y }, { Box.Max.X, Box.Max.Y } };

			TArray<jcv_point> Sites = Scatter_Impl(Pts, bounding_box, Seed, Count, Density, Iterations);
			jcv_diagram diagram;
			memset(&diagram, 0, sizeof(jcv_diagram));
			jcv_diagram_generate(Sites.Num(), Sites.GetData(), &bounding_box, &diagram);
			const jcv_site* sites = jcv_diagram_get_sites(&diagram);
			for (int i = 0; i < diagram.numsites; i++)
			{
				FArteriesPrimitive* Prim = Obj->AddPrimitive();
				jcv_graphedge* graph_edge = sites[i].edges;
				int Intersects[2] = { INDEX_NONE, INDEX_NONE };
				FVector2D Pt0(graph_edge->pos[0].x, graph_edge->pos[0].y);
				bool bInside0 = IsPointInside(Pts, Pt0);
				while (graph_edge)
				{
					if (bInside0)
						Prim->Add(Mapper.GetPoint(LocalToWorld.TransformPosition(FVector(Pt0, 0))));
					FVector2D Pt1(graph_edge->pos[1].x, graph_edge->pos[1].y);
					bool bInside1 = IsPointInside(Pts, Pt1);
					if (bInside0 != bInside1)
					{
						float T;
						Intersects[bInside1] = Intersect(Pts, Pt0, Pt1, T);
						if (bInside0)
							Prim->Add(Mapper.GetPoint(LocalToWorld.TransformPosition(FVector(FMath::Lerp(Pt0, Pt1, T), 0))));
						if (Intersects[!bInside1] != INDEX_NONE)
						{
							for (int j = Intersects[0]; j != Intersects[1];)
							{
								int NextIndex = (j + 1) % Primitive->NumPoints();
								Prim->Add(Mapper.GetPoint(Primitive->GetPoint(NextIndex)->Position));
								j = NextIndex;
							}
						}
						if (bInside1)
							Prim->Add(Mapper.GetPoint(LocalToWorld.TransformPosition(FVector(FMath::Lerp(Pt0, Pt1, T), 0))));
					}
					if (bInside1)
						Prim->Add(Mapper.GetPoint(LocalToWorld.TransformPosition(FVector(Pt1, 0))));
					Pt0 = Pt1;
					bInside0 = bInside1;
					graph_edge = graph_edge->next;
				}
				if (!Prim->Points.Num())
					Obj->DeletePrimitive(Prim);
				else
					Prim->MakeClose();
			}
			jcv_diagram_free(&diagram);
		}
	}
	ARTERIES_UFUNCTION_RETURN(Obj)
}
UArteriesObject* UArteriesObject::SubDivide(const FString& Groups, float MinLength)
{
	UArteriesObject* Obj = Copy_Impl();
	FArteriesPointMapper Mapper(Obj);
	TSet<FArteriesPrimitive*> Sets = Obj->GetPrimitives(Groups);
	for (FArteriesPrimitive* Prim : Sets)
	{
		int Index = Prim->LongestSegment();
		FArteriesPoint* This = Prim->GetPoint(Index);
		FArteriesPoint* Next = Prim->NextPoint(Index);
		FVector Direction = (Next->Position - This->Position).GetSafeNormal();
		FMatrix Matrix = GetTransform(This->Position, Direction);
		struct FDivideNode
		{
			FDivideNode(FArteriesPrimitive* InPrim, const FMatrix& InMatrix) :Box(EForceInit::ForceInit), Primitive(InPrim)
			{
				FMatrix InvMatrix = InMatrix.InverseFast();
				for (FArteriesPoint* Point : Primitive->Points)
					Box += FVector2D(InvMatrix.TransformPosition(Point->Position));
				/*
				InPrim->SetVec3("OBBSize", FVector(Box.GetSize(),0));
				InPrim->SetVec3("OBBCenter", Matrix.TransformPosition(FVector(Box.GetCenter(),0)));
				InPrim->SetVec3("TangentX", Matrix.GetScaledAxis(EAxis::X));*/
			}
			void Split(FArteriesPointMapper& InMapper, const FMatrix& InMatrix, TArray<FArteriesPoint*>& PPrim, TArray<FArteriesPoint*>& NPrim)
			{
				FVector2D Center, Extents;
				Box.GetCenterAndExtents(Center, Extents);
				FVector2D Origin = Center + Extents * FMath::RandRange(-0.5f, +0.5f);
				FVector Dir = Extents.X > Extents.Y ? FVector(1, 0, 0) : FVector(0, 1, 0);
				FPlane Plane(InMatrix.TransformPosition(FVector(Origin, 0)), InMatrix.TransformVector(Dir));
				Primitive->Clip(InMapper, Plane, PPrim, NPrim);
			}
			bool NeedSplit(float InMin)
			{
				FVector2D Size = Box.GetSize();
				return Size.X > InMin || Size.Y > InMin;
			}
			FBox2D Box;
			FArteriesPrimitive* Primitive;
		};
		TArray<FDivideNode> Nodes;
		Nodes.Add(FDivideNode(Prim, Matrix));
		for (int i = 0; i < Nodes.Num(); i++)
		{
			if (Nodes[i].NeedSplit(MinLength))
			{
				TArray<FArteriesPoint*> PPrim;
				TArray<FArteriesPoint*> NPrim;
				Nodes[i].Split(Mapper, Matrix, PPrim, NPrim);
				Nodes.Add(FDivideNode(Obj->AddPrimitive(PPrim,true), Matrix));
				Nodes.Add(FDivideNode(Obj->AddPrimitive(NPrim,true), Matrix));
				Obj->DeletePrimitive(Nodes[i].Primitive);
			}
		}		
	}
	ARTERIES_UFUNCTION_RETURN(Obj)
}
UArteriesObject* UArteriesObject::Blast(const FString& Groups, const FString& Tags, bool DeleteNonSelected)
{
	UArteriesObject* Obj = Copy_Impl();
	TSet<FArteriesPrimitive*> Prims = Groups != "" ? Obj->GetPrimitives(Groups) : Obj->GetPrimitivesWithTags(Tags);
	if (DeleteNonSelected)
	{
		for (int i = Obj->Primitives.Num() - 1; i >= 0; i--)
		{
			if (!Prims.Contains(&Obj->Primitives[i]))
				Obj->DeletePrimitive(i);
		}
	}
	else
	{
		for (auto It = Prims.CreateIterator(); It; ++It)
			Obj->DeletePrimitive(*It);
	}
	Obj->CleanPoints();
	ARTERIES_UFUNCTION_RETURN(Obj)
}
UArteriesObject* UArteriesObject::Facet(float Tolerance)
{
	UArteriesObject* Obj = Copy_Impl();
	for (FArteriesPrimitive& Prim : Obj->Primitives)
	{
		for (int i = !Prim.IsClosed(); i < Prim.NumSegments();)
		{
			FArteriesPoint* Prev = Prim.PrevPoint(i);
			FArteriesPoint* This = Prim.GetPoint(i);
			FArteriesPoint* Next = Prim.NextPoint(i);
			FVector PrevDirection = (This->Position - Prev->Position).GetSafeNormal();
			FVector NextDirection = (Next->Position - This->Position).GetSafeNormal();
			float Dot = PrevDirection | NextDirection;
			if (FMath::IsNearlyEqual(Dot, 1, Tolerance))
				Prim.Delete(i);
			else
				i++;
		}
	}
	Obj->CleanPoints();
	ARTERIES_UFUNCTION_RETURN(Obj)
}
UArteriesObject* UArteriesObject::Triangulate()
{
	UArteriesObject* Obj = Copy_Impl();
	for (int i = Obj->Primitives.Num() - 1; i >= 0; i--)
	{
		FArteriesPrimitive& Prim = Obj->Primitives[i];
		if (!Prim.bClosed)
			continue;
		TArray<FVector2D> Vertices;
		TArray<uint32> Indices;
		GluSimpleTriangulator Triangulator(Indices, Vertices);
		FMatrix ToLocal = Prim.GetLocalToWorldTransform().Inverse();
		for (FArteriesPoint* Point : Prim.Points)
		{
			FVector Pos = ToLocal.TransformPosition(Point->Position);
			Vertices.Add((FVector2D&)Pos);
		}
		Triangulator.Begin(TESS_WINDING_ODD);
		Triangulator.BeginContour();
		for (int j = 0; j < Vertices.Num(); j++)
			Triangulator.Vertex(Vertices[j], j);
		Triangulator.EndContour();
		Triangulator.End();
		for (int j = 0; j < Indices.Num(); j += 3)
		{
			FArteriesPrimitive* NewPrim = Obj->AddPrimitive();
			for (int k = 0; k < 3; k++)
				NewPrim->Add(Prim.Points[Indices[j+k]]);
			NewPrim->MakeClose();
		}
		Obj->DeletePrimitive(i);
	}
	ARTERIES_UFUNCTION_RETURN(Obj)
}
UArteriesObject* UArteriesObject::Divide(float Tolerance)
{
	UArteriesObject* Obj = Copy_Impl();
	for (FArteriesPrimitive& Primitive : Obj->Primitives)
		Primitive.CalculateNormal();
	TSet<TPair<FArteriesPoint*, FArteriesPoint*>> Edges;
	for (FArteriesPrimitive& Primitive : Obj->Primitives)
	{
		for (int i = 0; i < Primitive.NumSegments(); i++)
		{
			FArteriesPoint* This = Primitive.GetPoint(i);
			FArteriesPoint* Next = Primitive.NextPoint(i);
			TArray<FArteriesLink*> Links = This->GetLinks(Next, NULL, -1);
			if (Links.Num() == 2 && (Links[0]->Primitive->GetVec3(AVN_TangentZ) | Links[1]->Primitive->GetVec3(AVN_TangentZ)) > Tolerance)
			{
				if (!Edges.Contains(TPair<FArteriesPoint*, FArteriesPoint*>(Next, This)))
					Edges.Add(TPair<FArteriesPoint*, FArteriesPoint*>(This, Next));
			}
		}
	}
	for (auto It = Edges.CreateIterator(); It; ++It)
		Obj->DeleteEdge(*It);
	Obj->CleanPoints();
	ARTERIES_UFUNCTION_RETURN(Obj)
}
UArteriesObject* UArteriesObject::Fuse(const FString& Groups, float SnapDist)
{
	UArteriesObject* Obj = Copy_Impl();
	FArteriesPointMapper Mapper(Obj, Groups, SnapDist);
	Mapper.Fuse();
	ARTERIES_UFUNCTION_RETURN(Obj)
}
UArteriesObject* UArteriesObject::Clean()
{
	UArteriesObject* Obj = Copy_Impl();
	for (int i = Obj->Primitives.Num() - 1; i >= 0; i--)
	{
		bool bFullOverlapped = true;
		FArteriesPrimitive* Primitive = &Obj->Primitives[i];
		TSet<FArteriesPrimitive*> Intersection;
		for (int j = 0; j < Primitive->NumSegments(); j++)
		{
			TSet<FArteriesPrimitive*> Set;
			FArteriesPoint* This = Primitive->GetPoint(j);
			FArteriesPoint* Next = Primitive->NextPoint(j);
			TArray<FArteriesLink*> Links = This->GetLinks(Next, NULL, -1);
			for (FArteriesLink* Link : Links)
				Set.Add(Link->Primitive);
			if (j == 0)
				Intersection = Set;
			else
				Intersection = Intersection.Intersect(Set);
			if (Intersection.Num() < 2)
			{
				bFullOverlapped = false;
				break;
			}
		}
		if (bFullOverlapped)
			Obj->DeletePrimitive(Primitive);
	}
	ARTERIES_UFUNCTION_RETURN(Obj)
}
UArteriesObject* UArteriesObject::Hole(const FString& OuterGroups, const FString& InnerGroups, float Tolerance)
{
	UArteriesObject* Obj = Copy_Impl();
	TSet<FArteriesPrimitive*> Outers = Obj->GetPrimitives(OuterGroups);
	TSet<FArteriesPrimitive*> Inners = Obj->GetPrimitives(InnerGroups);
	for (FArteriesPrimitive* Inner : Inners)
	{
		Inner->CalculateTangents();
		FMatrix ToLocal = Inner->GetLocalToWorldTransform().Inverse();
		TArray<FVector> InnerLocal;
		FBox InnerBounds = Inner->GetBox(&ToLocal, &InnerLocal).ExpandBy(Tolerance);
		for (FArteriesPrimitive* Outer : Outers)
		{
			TArray<FVector> OuterLocal;
			FBox OuterBounds = Outer->GetBox(&ToLocal, &OuterLocal);
			if (InnerBounds.Intersect(OuterBounds))
			{
				TArray<FVector2D> Vertices;
				TArray<uint32> Indices;
				GluSimpleTriangulator Triangulator(Indices, Vertices);
				auto AddContour = [&](TArray<FVector>& Positions)
				{
					Triangulator.BeginContour();
					for (FVector& Position : Positions)
						Triangulator.Vertex(Position, Vertices.Add((FVector2D&)Position));
					Triangulator.EndContour();
				};
				Triangulator.Begin(TESS_WINDING_ABS_GEQ_TWO, true);
				AddContour(OuterLocal);
				AddContour(InnerLocal);
				Triangulator.End();
				if (Indices.Num())
				{
					Inner->Outer = Outer;
					break;
				}
			}
		}
	//	if (!Inner->Outer)
	//		UE_LOG(LogArteries, Warning, TEXT("Can't Find Outer for this Inner"));
	}
	ARTERIES_UFUNCTION_RETURN(Obj)
}
UArteriesObject* UArteriesObject::SortRandomly(int Seed)
{
	UArteriesObject* Obj = Copy_Impl();
	FMath::RandInit(Seed);
	int LastIndex = Obj->Primitives.Num() - 1;
	for (int i = 0; i <= LastIndex; ++i)
		Obj->Primitives.Swap(i, FMath::RandRange(i, LastIndex));
	ARTERIES_UFUNCTION_RETURN(Obj)
}
UArteriesObject* UArteriesObject::SortByAttribute(FName AttrName)
{
	UArteriesObject* Obj = Copy_Impl();
	Sort(Obj->Primitives.GetData(), Obj->Primitives.Num(), [&](const FArteriesPrimitive& P0, const FArteriesPrimitive& P1)
	{
		return P0.GetFloat(AttrName) > P1.GetFloat(AttrName);
	});
	ARTERIES_UFUNCTION_RETURN(Obj)
}
UArteriesObject* UArteriesObject::GroupRange(const TArray<FArteriesGroupRange>& Groups)
{
	UArteriesObject* Obj = Copy_Impl();
	for (const FArteriesGroupRange& Range : Groups)
	{
		int Start = Range.Start;
		int End = Range.End;
		if (End <= Start)
			End = Obj->NumPrimitives();
		FArteriesPrimitiveGroup& Group = Obj->PrimitiveGroups.Add(Range.Name);
		for (int i = Start; i < End; i += Range.Of)
		{
			for (int j = 0; j < Range.Select; j++)
			{
				int Index = i + (j + Range.Offset) % Range.Of;
				if (Index < End)
					Group.Add(&Obj->Primitives[Index]);
			}
		}
	}
	ARTERIES_UFUNCTION_RETURN(Obj)
}
UArteriesObject* UArteriesObject::Measure(FName Name)
{
	UArteriesObject* Obj = Copy_Impl();
	for (FArteriesPrimitive& Primitive : Obj->Primitives)
	{
		if (Name == "Area")
			Primitive.SetFloat("Area", Primitive.CalculateArea());
	}
	ARTERIES_UFUNCTION_RETURN(Obj)
}
UArteriesObject* UArteriesObject::Bridge(FName StartGroupName, FName EndGroupName)
{
	TMap<int, const FArteriesPrimitive*> StartPrims;
	TMap<int, const FArteriesPrimitive*> EndPrims;
	for (const FArteriesPrimitive& Primitive : Primitives)
	{
		if (Primitive.HasInt(StartGroupName))
		{
			StartPrims.Add(Primitive.GetInt(StartGroupName), &Primitive);
			continue;
		}
		if (Primitive.HasInt(EndGroupName))
		{
			EndPrims.Add(Primitive.GetInt(EndGroupName), &Primitive);
			continue;
		}
	}
	UArteriesObject* Obj = New_Impl();
	TMultiMap<FArteriesPoint*, FArteriesPoint*> CrossEdges;
	FArteriesPointMapper Mapper(Obj);
	for (auto It = StartPrims.CreateIterator(); It; ++It)
	{
		const FArteriesPrimitive* StartPrim = It->Value;
		if (EndPrims.Contains(It->Key))
		{
			const FArteriesPrimitive* EndPrim = EndPrims[It->Key];
			if (StartPrim->Points.Num() == EndPrim->Points.Num())
			{
				for (int i = 0; i < StartPrim->Points.Num() - 1; i++)
				{
					FArteriesPoint* SP0 = StartPrim->Points[i];
					FArteriesPoint* SP1 = StartPrim->Points[i + 1];
					FArteriesPoint* EP0 = EndPrim->Points[i];
					FArteriesPoint* EP1 = EndPrim->Points[i + 1];
					FArteriesPrimitive* Prim = Obj->AddPrimitive();
					Prim->Add(Mapper.GetPoint(SP0));
					Prim->Add(Mapper.GetPoint(SP1));
					Prim->Add(Mapper.GetPoint(EP1));
					Prim->Add(Mapper.GetPoint(EP0));
					Prim->MakeClose();
					if (i == 0 && CrossEdges.Remove(SP0, EP0) == 0)
						CrossEdges.Add(EP0, SP0);
					if (i == StartPrim->Points.Num() - 2 && CrossEdges.Remove(EP1, SP1) == 0)
						CrossEdges.Add(SP1, EP1);
				}
			}
		}
	}
	while (CrossEdges.Num())
	{
		TArray<FArteriesPoint*> Pts;
		FArteriesPoint* FirstPoint = CrossEdges.CreateIterator()->Key;
		FArteriesPoint* Point = FirstPoint;
		do
		{
			Pts.Add(Point);
			FArteriesPoint** Next = CrossEdges.Find(Point);
			if (!Next)
			{
				Pts.Empty();
				break;
			}
			CrossEdges.Remove(Point, *Next);
			Point = *Next;
		} while (Point != FirstPoint);
		if (Pts.Num() >= 3)
		{
			FArteriesPrimitive* Prim = Obj->AddPrimitive();
			for (int i = Pts.Num() - 1; i >= 0; i--)
				Prim->Add(Mapper.GetPoint(Pts[i]));
			Prim->MakeClose();
		}
	}
	ARTERIES_UFUNCTION_RETURN(Obj)
}
UArteriesObject* UArteriesObject::Road(float DefaultWidth)
{
	UArteriesObject* Obj = New_Impl();
	FArteriesPointMapper Mapper(Obj);
	int GroupId = 0;
	for (const FArteriesPrimitive& Primitive : Primitives)
	{
		FArteriesPrimitive* Left;
		FArteriesPrimitive* Center;
		FArteriesPrimitive* Right;
		auto NewGroup = [&]()
		{
			Left = Obj->AddPrimitive();
			Center = Obj->AddPrimitive();
			Right = Obj->AddPrimitive();
			Left->SetInt("left", GroupId);
			Center->SetInt("center", GroupId);
			Right->SetInt("right", GroupId);
			GroupId++;
		};
		auto CreateCorner = [&](FArteriesPoint* Point, FArteriesLink* Link, bool bEnd)
		{
			float LinkWidth = Link->GetHalfWidth(DefaultWidth);
			FArteriesLink* PrevLink = Point->GetPrevLink(Link);
			FArteriesLink* NextLink = Point->GetNextLink(Link);
			FVector Direction =-Point->GetLinkDirection(Link);
			FVector PrevTangent;
			FVector NextTangent;
			float PrevScale;
			float NextScale;
			if (PrevLink != Link && NextLink != Link)
			{
				FVector PrevDirection = Point->GetLinkDirection(PrevLink);
				FVector NextDirection = Point->GetLinkDirection(NextLink);
				PrevTangent = (PrevDirection + Direction).GetSafeNormal();
				NextTangent = (NextDirection + Direction).GetSafeNormal();
				PrevScale = 1.f / (PrevTangent | Direction);
				NextScale = 1.f / (NextTangent | Direction);
			}
			else
			{
				PrevScale = 1;
				NextScale = 1;
				PrevTangent = Direction;
				NextTangent = Direction;
			}
			FVector PrevNormal = GetTangentYFromX(PrevTangent);
			FVector NextNormal = GetTangentYFromX(NextTangent);
			if (bEnd)
			{
				Right->Add(Mapper.GetPoint(Point->Position + LinkWidth * PrevNormal * PrevScale));
				Left->Add(Mapper.GetPoint(Point->Position - LinkWidth * NextNormal * NextScale));
			}
			else
			{
				Left->Add(Mapper.GetPoint(Point->Position + LinkWidth * PrevNormal * PrevScale));
				Right->Add(Mapper.GetPoint(Point->Position - LinkWidth * NextNormal * NextScale));
			}
			Center->Add(Mapper.GetPoint(Point->Position));
		};
		auto CreateRoundCorner = [&](FArteriesPoint* Point, FArteriesLink* Link, bool bEnd)
		{
			FArteriesLink* PrevLink = Point->GetPrevLink(Link);
			FArteriesLink* NextLink = Point->GetNextLink(Link);
			FVector Tangent = -Point->GetLinkDirection(Link);
			FVector PrevTangent = Point->GetLinkDirection(PrevLink);
			FVector NextTangent = Point->GetLinkDirection(NextLink);

			FVector Normal = GetTangentYFromX(Tangent);
			FVector PNormal = GetTangentYFromX(PrevTangent);
			FVector NNormal = GetTangentYFromX(NextTangent);

			float CutDistance = Point->GetCutDistance(Link, DefaultWidth);
			float PrevCutDistance = Point->GetCutDistance(PrevLink, DefaultWidth);
			float NextCutDistance = Point->GetCutDistance(NextLink, DefaultWidth);
			
			FVector Direction = Tangent * CutDistance;
			FVector PrevDirection = PrevTangent * PrevCutDistance;
			FVector NextDirection = NextTangent * NextCutDistance;

			FVector CutPosition = Point->Position - Direction;
			FVector PrevCutPosition = Point->Position + PrevDirection;
			FVector NextCutPosition = Point->Position + NextDirection;

			float LinkWidth = Link->GetHalfWidth(DefaultWidth);
			float PrevWidth = PrevLink->GetHalfWidth(DefaultWidth);
			float NextWidth = NextLink->GetHalfWidth(DefaultWidth);

			FVector PrevStart = CutPosition - LinkWidth * Normal;
			FVector NextStart = CutPosition + LinkWidth * Normal;
			FVector PrevEnd = PrevCutPosition - PrevWidth * PNormal;
			FVector NextEnd = NextCutPosition + NextWidth * NNormal;
			int NumSegments = 4;
			for (int i = 0; i <= NumSegments; i++)
			{
				float Alpha = float(i) / NumSegments;
				if (!bEnd) Alpha = 1.f - Alpha;
				float Alpha2 = Alpha / 2;			
				FVector PrevPosition = FMath::CubicInterp(PrevStart, Direction, PrevEnd, PrevDirection, Alpha2);
				FVector NextPosition = FMath::CubicInterp(NextStart, Direction, NextEnd, NextDirection, Alpha2);
				if (bEnd)
				{
					Left->Add(Mapper.GetPoint(PrevPosition));
					Right->Add(Mapper.GetPoint(NextPosition));
				}
				else
				{
					Right->Add(Mapper.GetPoint(PrevPosition));
					Left->Add(Mapper.GetPoint(NextPosition));
				}
			}
			Center->Add(Mapper.GetPoint(CutPosition));
		};
		NewGroup();
		for (int i = 0; i < Primitive.Points.Num() - 1; i++)
		{
			FArteriesPoint* This = Primitive.Points[i];
			FArteriesPoint* Next = Primitive.Points[i + 1];
			FArteriesLink* Link = This->GetLinks(Next, &Primitive, false)[0];
			FArteriesLink* InvLink = Next->GetLinks(This, &Primitive, true)[0];
			if (This->IsRoundCorner())
				CreateRoundCorner(This, Link, false);
			else
				CreateCorner(This, Link, false);
			if (Next->IsRoundCorner())
				CreateRoundCorner(Next, InvLink, true);
			else
				CreateCorner(Next, InvLink, true);
			if (Next->NumLinks() > 2)
				NewGroup();
		}
	}
	ARTERIES_UFUNCTION_RETURN(Obj)
}
UArteriesObject* UArteriesObject::Ground()
{
	UArteriesObject* Obj = New_Impl();
	FArteriesPointMapper Mapper(Obj);
	TMultiMap<FArteriesPoint*, FArteriesPoint*> Edges;
	for (const FArteriesPrimitive& Primitive : Primitives)
	{
		if (Primitive.HasInt("left"))
		{
			for (int i = Primitive.Points.Num() - 1; i > 0; i--)
				Edges.Add(Primitive.Points[i], Primitive.Points[i - 1]);
			continue;
		}
		if (Primitive.HasInt("right"))
		{
			for (int i = 0; i < Primitive.Points.Num() - 1; i++)
				Edges.Add(Primitive.Points[i], Primitive.Points[i + 1]);
			continue;
		}
	}
	while (Edges.Num())
	{
		TArray<FArteriesPoint*> Pts;
		FArteriesPoint* FirstPoint = Edges.CreateIterator()->Key;
		FArteriesPoint* Point = FirstPoint;
		do
		{
			Pts.Add(Point);
			FArteriesPoint** Next = Edges.Find(Point);
			if (!Next)
			{
				Pts.Empty();
				break;
			}
			Edges.Remove(Point, *Next);
			Point = *Next;
		} while (Point != FirstPoint);
		if (Pts.Num() >= 3)
		{
			FArteriesPrimitive* Prim = Obj->AddPrimitive();
			for (int i = 0; i < Pts.Num(); i++)
				Prim->Add(Mapper.GetPoint(Pts[i]));
			Prim->MakeClose();
		}
	}
	ARTERIES_UFUNCTION_RETURN(Obj)
}
UArteriesObject* UArteriesObject::MakeGrids(const FString& Groups, float GridSize, int MinGrids, bool OBBOnly)
{
	UArteriesObject* Obj = New_Impl();
	FArteriesPointMapper Mapper(Obj);
	TSet<FArteriesPrimitive*> Sets = GetPrimitives(Groups);
	for (const FArteriesPrimitive* Primitive : Sets)
	{
		if (Primitive->IsClosed())
		{
			FMatrix Matrix;
			FVector Size;
			Primitive->GetOrientedBox(Matrix, Size);
			FVector Extent = Size / 2;
			if (OBBOnly)
			{
				FArteriesPrimitive* Prim = Obj->AddPrimitive();
				Prim->Add(Mapper.GetPoint(Matrix.TransformPosition(FVector(-Extent.X, -Extent.Y, 0))));
				Prim->Add(Mapper.GetPoint(Matrix.TransformPosition(FVector(+Extent.X, -Extent.Y, 0))));
				Prim->Add(Mapper.GetPoint(Matrix.TransformPosition(FVector(+Extent.X, +Extent.Y, 0))));
				Prim->Add(Mapper.GetPoint(Matrix.TransformPosition(FVector(-Extent.X, +Extent.Y, 0))));
				Prim->MakeClose();
			}
			else
			{
				FMatrix W2L = Matrix.Inverse();
				TArray<FVector> Pts;
				for (FArteriesPoint* Point : Primitive->Points)
					Pts.Add(W2L.TransformPosition(Point->Position));
				int NumGridsX, NumGridsY;
				if (GridSize > 0)
				{
					NumGridsX = FMath::RoundToInt(Size.X / GridSize);
					NumGridsY = FMath::RoundToInt(Size.Y / GridSize);
				}
				else
				{
					if (Size.X > Size.Y)
					{
						NumGridsY = MinGrids;
						NumGridsX = FMath::RoundToInt(NumGridsY * (Size.X / Size.Y));
					}
					else
					{
						NumGridsX = MinGrids;
						NumGridsY = FMath::RoundToInt(NumGridsX * (Size.Y / Size.X));
					}
				}
				float StepX = Size.X / NumGridsX;
				float StepY = Size.Y / NumGridsY;
				for (int i = 0; i < NumGridsY; i++)
				{
					for (int j = 0; j < NumGridsX; j++)
					{
						FVector2D Start(-Extent.X + j * StepX, -Extent.Y + i * StepY);
						FVector2D Center = Start + FVector2D(0.5f * StepX, 0.5f * StepY);
						if (IsPointInside(Pts, Center))
						{
							FArteriesPrimitive* Prim = Obj->AddPrimitive();
							Prim->Add(Mapper.GetPoint(Matrix.TransformPosition(FVector(Start, 0))));
							Prim->Add(Mapper.GetPoint(Matrix.TransformPosition(FVector(Start + FVector2D(StepX, 0), 0))));
							Prim->Add(Mapper.GetPoint(Matrix.TransformPosition(FVector(Start + FVector2D(StepX, StepY), 0))));
							Prim->Add(Mapper.GetPoint(Matrix.TransformPosition(FVector(Start + FVector2D(0, StepY), 0))));
							Prim->MakeClose();
						}
					}
				}
			}
		}
	}
	ARTERIES_UFUNCTION_RETURN(Obj)
}
#include "SSkeleton/SSkeletonBuilder.h"
#include "Misc/FileHelper.h"
UArteriesObject* UArteriesObject::PolyExpand(const FString& Groups, float Offset, FName CurveGroup, FName SurfaceGroup, bool OutputTangents, bool DeleteSource)
{
	UArteriesObject* Obj = Copy_Impl();
	FArteriesPointMapper Mapper(Obj);
	TSet<FArteriesPrimitive*> Sets = Obj->GetPrimitives(Groups);
	for (FArteriesPrimitive* Primitive : Sets)
	{
		Primitive->CalculateTangents();
#if ARTERIES_PRIMITIVE_VALIDATION
		if (!Primitive->IsValidPolygon() || Primitive->IsSelfIntersect())
		{
			UE_LOG(LogArteries, Error, TEXT("Validation failed in PolyExpand"));
			continue;
		}
#endif
		FMatrix LocalToWorld = Primitive->GetLocalToWorldTransform();
		FMatrix WorldToLocal = LocalToWorld.Inverse();
		std::vector<Point2D> Polygon;
		for (int i = 0; i < Primitive->NumPoints(); i++)
		{
			const FVector& Pos = WorldToLocal.TransformPosition(Primitive->Points[i]->Position);
			Polygon.push_back(Point2D(Pos.X, Pos.Y));
		}
		SSkeletonBuilder ssb;
		ssb.enter_contour(Polygon);
		SSkeleton* iss = ssb.construct_skeleton();
#if 1
	//	try
		{
			TMap<FArteriesPrimitive*, int> CandidateGableTops;
			TSet<int> UsedEdges;
			for (auto it = iss->halfedges_begin(); it != iss->halfedges_end(); ++it)
			{
				if ((*it)->is_border() || UsedEdges.Contains((*it)->id()))
					continue;
				HalfEdge* nextstart = *it;
				TArray<FArteriesPoint*> CurvePrim;
				do
				{
					FArteriesPrimitive* SurfacePrim = SurfaceGroup != NAME_None ? Obj->AddPrimitive() : NULL;
					HalfEdge* start = nextstart;
					HalfEdge* edge = nextstart;
					Vertex* src = nextstart->opposite()->vertex();
					do
					{
						UsedEdges.Add(edge->id());
						Vertex* dst = edge->vertex();
						float stime = src->time();
						float dtime = dst->time();
						const Point2D& sp = src->point();
						const Point2D& dp = dst->point();
						auto AddLerp = [&]()
						{
							float alpha = (Offset - stime) / (dtime - stime);
							FVector Pos = FMath::Lerp(FVector(sp.x, sp.y, 0), FVector(dp.x, dp.y, 0), alpha);
							FArteriesPoint* NP = Mapper.GetPoint(LocalToWorld.TransformPosition(Pos));
							NP->SetFloat(AVN_EdgeDist, FMath::Lerp(stime, dtime, alpha));
							if (CurveGroup != NAME_None) CurvePrim.Add(NP);
							if (SurfacePrim) SurfacePrim->Add(NP);
						};
						auto AddPoint = [&](const Point2D& p, float time)
						{
							if (SurfacePrim)
							{
								FVector Pos(p.x, p.y, 0);
								FArteriesPoint* NP = Mapper.GetPoint(LocalToWorld.TransformPosition(Pos));
								NP->SetFloat(AVN_EdgeDist, time);
								SurfacePrim->Add(NP);
							}
						};
						if (Offset >= 0)
						{
							if (dtime > Offset)
							{
								if (stime < Offset)
									AddLerp();
							}
							else
							{
								if (stime > Offset)
									AddLerp();
								AddPoint(dp, dtime);
							}
						}
						else
						{
							if (src->is_contour() && !dst->is_contour())
							{
								AddLerp();
								AddPoint(sp, stime);
							}
							if (!src->is_contour() && dst->is_contour())
							{
								AddPoint(dp, dtime);
								AddLerp();
							}
						}
						if (!src->is_contour() && dst->is_contour())
							nextstart = edge->opposite();
						src = dst;
						edge = edge->next();
					} while (edge != start);
					if (SurfacePrim)
					{
						if (!SurfacePrim->Points.Num())
							Obj->DeletePrimitive(SurfacePrim);
						else
						{
							SurfacePrim->MakeClose();
							Obj->SetPrimitiveGroup(SurfacePrim, SurfaceGroup, 1);
							for (int i = 0; i < SurfacePrim->NumSegments(); i++)
							{
								FArteriesPoint* This = SurfacePrim->GetPoint(i);
								FArteriesPoint* Next = SurfacePrim->NextPoint(i);
								if (FMath::IsNearlyZero(This->GetFloat(AVN_EdgeDist)) && FMath::IsNearlyZero(Next->GetFloat(AVN_EdgeDist)))
								{
									FVector Direction = Next->Position - This->Position;
									float Size = Direction.Size();
									Direction /= Size;
									if (OutputTangents)
										SurfacePrim->SetVec3(AVN_TangentX, Direction);
									if (SurfacePrim->NumPoints() == 3)
										CandidateGableTops.Add(SurfacePrim, (i + 2) % 3);
									break;
								}
							}
						}
					}
				} while (!UsedEdges.Contains(nextstart->id()));
				if (CurveGroup != NAME_None && CurvePrim.Num())
				{
					Algo::Reverse(CurvePrim);
					Obj->SetPrimitiveGroup(Obj->AddPrimitive(CurvePrim, true), CurveGroup, 1);
				}
			}
			for (auto It = CandidateGableTops.CreateIterator(); It; ++It)
			{
				FArteriesPoint* Top = It->Key->Points[It->Value];
				if (Top->Links.Num() == 6)
					It->Key->SetInt("GableTop", It->Value);
			}
		}
		/*
		catch (...)
		{
			{
				FString Str;
				for (VertexIterator i = iss->vertices_begin(); i != iss->vertices_end(); ++i)
				{
					const Point2D& pt = (*i)->point();
					Str += FString::Printf(TEXT("%lf, %lf, %lf\r\n"), pt.x, pt.y, (*i)->time());
				}
				FFileHelper::SaveStringToFile(Str, TEXT("ErrorData.txt"));
			}
			{
				FString Str;
				for (int i = 0; i < Polygon.size(); i++)
				{
					Point2D& Pt = Polygon[i];
					Str += FString::Printf(TEXT("Polygon.push_back(Point(%.17g, %.17g));\r\n"),Pt.x,Pt.y);
				}
				FFileHelper::SaveStringToFile(Str, TEXT("ErrorCode.txt"));
			}
		}*/
#else
		for (Halfedge_const_iterator i = iss->halfedges_begin(); i != iss->halfedges_end(); ++i)
		{
			const Point& sp = i->opposite()->vertex()->point();
			const Point& dp = i->vertex()->point();
			FArteriesPrimitive* Prim = Obj->AddPrimitive();
			Prim->Add(Mapper.GetPoint(FVector(sp.x(), sp.y(), 0)));
			Prim->Add(Mapper.GetPoint(FVector(dp.x(), dp.y(), 0)));
		}
#endif
	}
	if (DeleteSource)
	{
		for (FArteriesPrimitive* Primitive : Sets)
			Obj->DeletePrimitive(Primitive);
	}
	Obj->CleanPoints();
	ARTERIES_UFUNCTION_RETURN(Obj)
}
UArteriesObject* UArteriesObject::Clip(const FString& Groups, FVector PlaneNormal, float PlaneDist, FName PositiveGroup, FName NegativeGroup)
{
	UArteriesObject* Obj = Copy_Impl();
	Obj->Clip_Impl(Groups, FPlane(PlaneNormal, PlaneDist), PositiveGroup, NegativeGroup);
	ARTERIES_UFUNCTION_RETURN(Obj)
}
UArteriesObject* UArteriesObject::Resample(bool ByEdge, float SegmentLength, int NumSegments, EArteriesTangentOutputType OutputType)
{
	UArteriesObject* Obj = New_Impl();
	FArteriesPointMapper Mapper(Obj);
	if (NumSegments > 0 || SegmentLength > 1.f)
	{
		auto SetTangentX = [&](FArteriesPoint* Point, FVector TangentX)
		{
			if (OutputType == EArteriesTangentOutputType::Flatten)
			{
				TangentX.Z = 0;
				TangentX = TangentX.GetSafeNormal();
			}
			Point->SetVec3(AVN_TangentX, TangentX);
		};
		for (const FArteriesPrimitive& Src : Primitives)
		{
			FArteriesPrimitive* Dst = Obj->AddPrimitive();
			Dst->CopyValues(Src);
			if (Src.NumPoints() > 1)
			{
				if (ByEdge)
				{
					for (int i = 0; i < Src.NumSegments(); i++)
					{
						FArteriesPoint* StartPoint = Src.GetPoint(i);
						FArteriesPoint* EndPoint = Src.NextPoint(i);
						//ByEdge模式保留原有形状，点数只多不少，每段最少两个点
						int NumSegs = NumSegments > 0 ? NumSegments : FMath::Max(1, FMath::RoundToInt((EndPoint->Position - StartPoint->Position).Size() / SegmentLength));
						for (int j = (i != 0); j <= NumSegs; j++)
						{
							float Alpha = NumSegs ? float(j) / NumSegs : 0.5f;
							FArteriesPoint* NewPoint = Mapper.GetPointLerp(StartPoint, EndPoint, Alpha);
							if (OutputType != EArteriesTangentOutputType::None) SetTangentX(NewPoint, (EndPoint->Position - StartPoint->Position).GetSafeNormal());
							Dst->Add(NewPoint);
						}
					}
				}
				else
				{
					TArray<float> Dists = Src.GetDists();
					int NumSegs = NumSegments > 0 ? NumSegments : FMath::RoundToInt(Dists.Last() / SegmentLength);
					float Step;
					float Dist;
					if (NumSegs > 0)
					{
						Step = Dists.Last() / NumSegs;
						Dist = 0;
						if (Src.bClosed)
							NumSegs--;
					}
					else
					{
						Step = 0;
						Dist = Dists.Last() / 2;
					}
					int Index = 0;
					for (int i = 0; i <= NumSegs; i++)
					{
						while (Dist > Dists[Index+1] && Index < Dists.Num() - 2)
						{
							Index++;
						}
						float Prev = Dists[Index];
						float Next = Dists[Index+1];
						float Alpha = (Dist - Prev) / (Next - Prev);
						FArteriesPoint* StartPoint = Src.GetPoint(Index);
						FArteriesPoint* EndPoint = Src.NextPoint(Index);
						FArteriesPoint* NewPoint = Mapper.GetPointLerp(StartPoint, EndPoint, Alpha);
						if (OutputType != EArteriesTangentOutputType::None) SetTangentX(NewPoint, (EndPoint->Position - StartPoint->Position).GetSafeNormal());
						Dst->Add(NewPoint);
						Dist += Step;
					}
				}
			}
			else
			{
				FArteriesPoint* NewPoint = Mapper.GetPoint(Src.Points[0]);
				if (OutputType != EArteriesTangentOutputType::None) SetTangentX(NewPoint, Src.GetVec3(AVN_TangentX));
				Dst->Add(NewPoint);
			}
			if (Dst->NumPoints() == 1)
				Dst->MakeClose();
		}
	}
	ARTERIES_UFUNCTION_RETURN(Obj)
}
UArteriesObject* UArteriesObject::Reverse(const FString& Groups)
{
	UArteriesObject* Obj = Copy_Impl();
	TSet<FArteriesPrimitive*> Sets = Obj->GetPrimitives(Groups);
	for (FArteriesPrimitive* Primitive : Sets)
		Primitive->Reverse();
	ARTERIES_UFUNCTION_RETURN(Obj)
}
UArteriesObject* UArteriesObject::Transform(const EArteriesGroupType GroupType, const FString& Groups, FRotator Rotation, FVector Translation, FVector Scale)
{
	UArteriesObject* Obj = Copy_Impl();
	Obj->Transform_Impl(FTransform(Rotation, Translation, Scale), GroupType, Groups);
	ARTERIES_UFUNCTION_RETURN(Obj)
}
UArteriesObject* UArteriesObject::PolyBevel(const EArteriesGroupType GroupType, const FString& Groups, float Distance, int Divisions, bool bBevelSingleCurve)
{
	UArteriesObject* Obj = Copy_Impl();
	if (GroupType == EArteriesGroupType::Point)
	{
		for (FArteriesPrimitive& Primitive : Obj->Primitives)
			Primitive.CalculateTangents();
		float Step = 1.f / Divisions;
		TSet<FArteriesPoint*> PointSet = Obj->GetPoints(Groups);
		FArteriesPointMapper Mapper(Obj);
		struct FBevelNode
		{
			TArray<FCubicCurve> Curves;
			TArray<TMap<FArteriesPrimitive*, FIntPoint>> SortedPrimitives;
		};
		TMap<FArteriesPoint*, FBevelNode> BevelNodes;
		for (FArteriesPoint* Point : PointSet)
		{
#if ARTERIES_PRIMITIVE_VALIDATION
			if (Point->IsSelfIntersect())
			{
				UE_LOG(LogArteries, Error, TEXT("Validation failed in PolyBevel"));
				continue;
			}
#endif
			FBevelNode& BevelNode = BevelNodes.Add(Point);
			auto CreateFirstRoundCurves = [&](FArteriesLink* Prev, FArteriesLink* Next)
			{
				FCubicCurve& Curve = BevelNode.Curves[BevelNode.Curves.AddUninitialized()];
				FVector PrevDirection, NextDirection;
				float PrevDist = FMath::Min(Distance, Point->GetLinkLength(Prev, &PrevDirection) / 2);
				float NextDist = FMath::Min(Distance, Point->GetLinkLength(Next, &NextDirection) / 2);
				Curve.P0 = Point->Position + PrevDirection * PrevDist;
				Curve.P3 = Point->Position + NextDirection * NextDist;
				Curve.P1 = (Curve.P0 + Point->Position) / 2;
				Curve.P2 = (Curve.P3 + Point->Position) / 2;
			};
			TArray<FArteriesLink*> Links = Point->GetSortedLinks();
			if (bBevelSingleCurve)
			{
				TMap<FArteriesPrimitive*, FIntPoint>& Sets = BevelNode.SortedPrimitives[BevelNode.SortedPrimitives.AddDefaulted()];
				auto CreatePrimitiveSets = [&](FArteriesLink* InPrev, FArteriesLink* InNext)
				{
					for (int i = 0; i < Links.Num(); i += 2)
					{
						FArteriesLink* Prev = Links[i];
						FArteriesLink* Next = Links[i + 1];
						FIntPoint Index;
						Index.X = Prev->Target == InPrev->Target ? 0 : (Prev->Target == InNext->Target ? Divisions : Divisions / 2);
						Index.Y = Next->Target == InNext->Target ? Divisions : (Next->Target == InPrev->Target ? 0 : Divisions / 2);
						Sets.Add(Prev->Primitive, Index);
					}
				};
				for (int i = 0; i < Links.Num(); i += 2)
				{
					if (PointSet.Contains(Links[i]->Target) && PointSet.Contains(Links[i + 1]->Target))
					{
						CreateFirstRoundCurves(Links[i], Links[i + 1]);
						CreatePrimitiveSets(Links[i], Links[i + 1]);
						break;
					}
				}
				if (!BevelNode.Curves.Num())
				{
					CreateFirstRoundCurves(Links.Last(), Links[0]);
					CreatePrimitiveSets(Links.Last(), Links[0]);
				}
			}
			else
			{
				for (int i = 0; i < Links.Num(); i += 2)
				{
					CreateFirstRoundCurves(Links[i], Links[i + 1]);
					TMap<FArteriesPrimitive*, FIntPoint>& Sets = BevelNode.SortedPrimitives[BevelNode.SortedPrimitives.AddDefaulted()];
					Sets.Add(Links[i]->Primitive, FIntPoint(0, Divisions));
				}
			}
		}
		for (auto It = BevelNodes.CreateIterator(); It; ++It)
		{
			typedef TArray<FVector> FPointsInCurve;
			typedef TArray<FCubicCurve> FCurvesInRound;
			typedef TArray<FPointsInCurve> FPointsInRound;
			TIndirectArray<FPointsInRound> PointsInRounds;
			TIndirectArray<FCurvesInRound> CurvesInRounds;
			FArteriesPoint* Point = It->Key;
			FBevelNode& BevelNode = It->Value;

			FVector Normal = FVector::ZeroVector;
			for (TMap<FArteriesPrimitive*, FIntPoint>& Sets : BevelNode.SortedPrimitives)
			{
				for (auto it = Sets.CreateIterator(); it; ++it)
					Normal += it->Key->GetVec3(AVN_TangentZ);
			}
			Normal.Normalize();
			FMatrix ProjectMatrix = GetPlaneTransform(Point->Position, Normal).InverseFast();

			FCurvesInRound& FirstRoundCurves = CurvesInRounds[CurvesInRounds.Add(new FCurvesInRound(BevelNode.Curves))];
			int NumRounds = BevelNode.Curves.Num() < 3 ? 1 : Divisions / 2 + 1;
			//二：生成所有内圈曲线
			for (int i = 1; i < NumRounds; i++)
			{
				FCurvesInRound& NewRoundCurves = CurvesInRounds[CurvesInRounds.Add(new FCurvesInRound)];
				for (int j = 0; j < FirstRoundCurves.Num(); j++)
				{
					FCubicCurve& Curve = FirstRoundCurves[j];
					FCubicCurve& PrevCurve = FirstRoundCurves[(j - 1 + FirstRoundCurves.Num()) % FirstRoundCurves.Num()];
					FCubicCurve& NextCurve = FirstRoundCurves[(j + 1) % FirstRoundCurves.Num()];
					float Alpha = i * Step;
					float InvAlpha = 1.f - Alpha;
					FCubicCurve& NewCurve = NewRoundCurves[NewRoundCurves.AddDefaulted()];
					NewCurve.P0 = PrevCurve.GetPosition(InvAlpha);
					NewCurve.P3 = NextCurve.GetPosition(Alpha);
					NewCurve.P1 = (NewCurve.P0 + Point->Position) / 2;
					NewCurve.P2 = (NewCurve.P3 + Point->Position) / 2;
				}
			}
			//三：生成所有圈的点
			for (int i = 0; i < NumRounds; i++)
			{
				FPointsInRound& ThisRoundPoints = PointsInRounds[PointsInRounds.Add(new FPointsInRound)];
				FCurvesInRound& ThisRoundCurves = CurvesInRounds[i];
				for (int j = 0; j < ThisRoundCurves.Num(); j++)
				{
					FPointsInCurve& CurvePoints = ThisRoundPoints[ThisRoundPoints.AddDefaulted()];
					FCubicCurve& Curve = ThisRoundCurves[j];
					if (i == 0)
					{
						for (int k = 0; k <= Divisions; k++)
						{
							float NewAlpha = float(k) / Divisions;
							CurvePoints.Add(Curve.GetPosition(NewAlpha));
						}
					}
					else
					{
						int PrevIndex = (j - 1 + ThisRoundCurves.Num()) % ThisRoundCurves.Num();
						int NextIndex = (j + 1) % ThisRoundCurves.Num();
						for (int k = i; k <= Divisions-i; k++)
						{
							if (float(k) / Divisions <= 0.5f)
								CurvePoints.Add(Curve.Intersection(CurvesInRounds[k][PrevIndex], ProjectMatrix));
							else
								CurvePoints.Add(Curve.Intersection(CurvesInRounds[Divisions - k][NextIndex], ProjectMatrix));
						}
					}
				}
			}
			//四：切割Primitives
			FPointsInRound& FirstRoundPoints = PointsInRounds[0];
			for (int i = 0; i < FirstRoundPoints.Num(); i++)
			{
				FPointsInCurve& Curve = FirstRoundPoints[i];
				TMap<FArteriesPrimitive*, FIntPoint>& Sets = BevelNode.SortedPrimitives[i];
				for (auto it = Sets.CreateIterator(); it; ++it)
				{
					FArteriesPrimitive* Primitive = it->Key;
					FIntPoint& Interval = it->Value;
					int Index = Primitive->Find(Point);
					Primitive->Delete(Index);
					if (Interval.Y > Interval.X)
					{
						for (int j = Interval.X; j <= Interval.Y; j++)
							if (Primitive->Insert(Mapper.GetPoint(Curve[j]), Index))
								Index++;
					}
					else
					{
						for (int j = Interval.X; j >= Interval.Y; j--)
							if (Primitive->Insert(Mapper.GetPoint(Curve[j]), Index))
								Index++;
					}
				}
			}
#if 1		//五：生成内部Primitives
			for (int i = 0; i < NumRounds - 1; i++)
			{
				FPointsInRound& ThisRound = PointsInRounds[i];
				FPointsInRound& NextRound = PointsInRounds[i + 1];
				for (int j = 0; j < ThisRound.Num(); j++)
				{
					FPointsInCurve& PrevCurve = ThisRound[(j - 1 + ThisRound.Num()) % ThisRound.Num()];
					FPointsInCurve& Curve = ThisRound[j];
					FPointsInCurve& InnerCurve = NextRound[j];
					for (int k = 0; k < Curve.Num() - 2; k++)
					{
						FArteriesPrimitive* Prim = Obj->AddPrimitive();
						Prim->Add(Mapper.GetPoint(Curve[k]));
						if (k == 0)
							Prim->Add(Mapper.GetPoint(PrevCurve[Curve.Num() - 2]));
						else
							Prim->Add(Mapper.GetPoint(InnerCurve[k-1]));
						Prim->Add(Mapper.GetPoint(InnerCurve[k]));
						Prim->Add(Mapper.GetPoint(Curve[k + 1]));
						Prim->MakeClose();
					}
				}
				if (i == NumRounds - 2 && Divisions % 2)
				{
					FArteriesPrimitive* Prim = Obj->AddPrimitive();
					for (int j = NextRound.Num()-1; j >= 0; j--)
						Prim->Add(Mapper.GetPoint(NextRound[j][0]));
					Prim->MakeClose();
				}
			}
#elif 1		//五：所有交点画线
			for (int i = 0; i < NumRounds; i++)
			{
				FPointsInRound& ThisRound = PointsInRounds[i];
				for (int j = 0; j < ThisRound.Num(); j++)
				{
					FPointsInCurve& Curve = ThisRound[j];
					FArteriesPrimitive* Prim = Obj->AddPrimitive();
					for (int k = 0; k < Curve.Num(); k++)
						Prim->Add(Mapper.GetPoint(Curve[k]));
				}
			}
#else		//五：直接Curve画线
			for (int i = 0; i < NumRounds; i++)
			{
				FCurvesInRound& ThisRoundCurves = CurvesInRounds[i];
				for (int j = 0; j < ThisRoundCurves.Num(); j++)
				{
					FCubicCurve& Curve = ThisRoundCurves[j];
					FArteriesPrimitive* Prim = Obj->AddPrimitive();
					for (int k = 0; k <= Divisions; k++)
					{
						float NewAlpha = float(k) / Divisions;
						Prim->Add(Mapper.GetPoint(Curve.GetPosition(NewAlpha)));
					}
				}
			}
#endif
		}
	}
	Obj->CleanPoints();
	ARTERIES_UFUNCTION_RETURN(Obj)
}
UArteriesObject* UArteriesObject::PolyExtrude(const FString& Groups, float Distance, float Inset, FName FrontGroup, FName SideGroup, bool DeleteSource)
{
	UArteriesObject* Obj = Copy_Impl();
	TSet<FArteriesPrimitive*> BPrims = Obj->GetPrimitives(Groups);
	struct FExtrudePoint
	{
		FExtrudePoint() :MinNormal(0, 0, 0), MaxNormal(0, 0, 0) {}
		void Add(const FVector& Normal)
		{
			if (Normal.X > 0) MaxNormal.X = FMath::Max(MaxNormal.X, Normal.X);
			else MinNormal.X = FMath::Min(MinNormal.X, Normal.X);
			if (Normal.Y > 0) MaxNormal.Y = FMath::Max(MaxNormal.Y, Normal.Y);
			else MinNormal.Y = FMath::Min(MinNormal.Y, Normal.Y);
			if (Normal.Z > 0) MaxNormal.Z = FMath::Max(MaxNormal.Z, Normal.Z);
			else MinNormal.Z = FMath::Min(MinNormal.Z, Normal.Z);
			Normals.Add(Normal);
		}
		void Add(const FVector& Normal, const FVector& Offset)
		{
			Add(Normal);
			Offsets.Add(Offset);
		}
		void Finalize(UArteriesObject* Object, FArteriesPoint* Point, float InDist)
		{
			FVector Normal = (MinNormal + MaxNormal).GetSafeNormal();
			//	UE_LOG(LogArteries, Log, TEXT("======================="));
			float MinDot = 1;
			for (FVector& N : Normals)
			{
				float Dot = Normal | N;
				MinDot = FMath::Min(MinDot, Dot);
				//	UE_LOG(LogArteries, Log, TEXT("%f"), Dot);
			}
			FVector Offset(0, 0, 0);
			for (FVector& O : Offsets)
				Offset += O;
			Target = Object->AddPoint(Point->Position + Normal * (InDist / MinDot) + Offset);
		}
		FVector MinNormal;
		FVector MaxNormal;
		TArray<FVector> Normals;
		TArray<FVector> Offsets;
		FArteriesPoint* Target;
	};
	TMap<FArteriesPoint*, FExtrudePoint> EPoints;
	TMultiMap<FArteriesPoint*, FArteriesPoint*> EEdges;
	for (FArteriesPrimitive* BPrim : BPrims)
	{
		BPrim->CalculateTangents();
		for (int i = 0; i < BPrim->NumSegments(); i++)
		{
			FArteriesPoint* This = BPrim->GetPoint(i);
			FVector TangentZ = BPrim->GetVec3(AVN_TangentZ);
			if (FMath::IsNearlyZero(Inset))
				EPoints.FindOrAdd(This).Add(TangentZ);
			else
			{
				FArteriesPoint* Prev = BPrim->PrevPoint(i);
				FArteriesPoint* Next = BPrim->NextPoint(i);
				FVector PrevDirection = (This->Position - Prev->Position).GetSafeNormal();
				FVector NextDirection = (Next->Position - This->Position).GetSafeNormal();
				FVector Direction = (PrevDirection + NextDirection).GetSafeNormal();
				FVector Normal = (TangentZ ^ Direction).GetSafeNormal();
				float Dist = Inset / (Direction | NextDirection);
				EPoints.FindOrAdd(This).Add(TangentZ, Normal * Dist);
			}
			EEdges.Add(BPrim->GetPoint(i), BPrim->NextPoint(i));
		}
	}
	for (auto It = EPoints.CreateIterator(); It; ++It)
		It->Value.Finalize(Obj, It->Key, Distance);
	if (FrontGroup != NAME_None)
	{
		for (FArteriesPrimitive* BPrim : BPrims)
		{
			FArteriesPrimitive* Prim = Obj->AddPrimitive();
			for (FArteriesPoint* Point : BPrim->Points)
				Prim->Add(EPoints[Point].Target);
			Prim->MakeClose();
			if (BPrim->HasVec3(AVN_TangentX))
				Prim->SetVec3(AVN_TangentX, BPrim->GetVec3(AVN_TangentX));
			Obj->SetPrimitiveGroup(Prim, FrontGroup, 1);
		}		
	}
	if (SideGroup != NAME_None)
	{
		for (FArteriesPrimitive* BPrim : BPrims)
		{
			for (int i = 0; i < BPrim->NumSegments(); i++)
			{
				FArteriesPoint* Start = BPrim->GetPoint(i);
				FArteriesPoint* End = BPrim->NextPoint(i);
				if (!EEdges.FindPair(End, Start))
				{
					FArteriesPrimitive* Prim = Obj->AddPrimitive();
					Prim->Add(Start);
					Prim->Add(End);
					Prim->Add(EPoints[End].Target);
					Prim->Add(EPoints[Start].Target);
					Prim->MakeClose();
					Prim->SetVec3(AVN_TangentX, (End->Position - Start->Position).GetSafeNormal());
					Obj->SetPrimitiveGroup(Prim, SideGroup, 1);
				}
			}
		}
	}
	if (DeleteSource)
	{
		for (FArteriesPrimitive* BPrim : BPrims)
			Obj->DeletePrimitive(BPrim);
	}
	Obj->CleanPoints();
	ARTERIES_UFUNCTION_RETURN(Obj)
}
UArteriesObject* UArteriesObject::Sweep(UArteriesObject* Backbones)
{
	UArteriesObject* Obj = Sweep_Impl(Backbones);
	ARTERIES_UFUNCTION_RETURN(Obj)
}
UArteriesObject* UArteriesObject::SetMaterial(const FString& Groups, UMaterialInterface* Material, FVector2D UVScale, FVector2D UVOffset, float UVRotation, bool NullOnly)
{
	UArteriesObject* Obj = Copy_Impl();
	TSet<FArteriesPrimitive*> Prims = Obj->GetPrimitives(Groups);
	for (FArteriesPrimitive* Primitive : Prims)
	{
		if (!NullOnly || !Primitive->Material)
			Primitive->Material = Material;
	}
	/*
	FMatrix UVTransform = FScaleMatrix(FVector(UVScale,1)) * FRotationTranslationMatrix(FRotator(0, UVRotation, 0), FVector(UVOffset,0));
	FVector X, Y, Z;
	UVTransform.GetUnitAxes(X,Y,Z);
	for (FArteriesVertex& Vertex : Obj->Vertices)
	{
		FMatrix TangentTransform(Vertex.TangentX, Vertex.TangentY, Vertex.TangentZ(), FVector::ZeroVector);
		Vertex.TangentX = TangentTransform.TransformVector(X);
		Vertex.TangentY = TangentTransform.TransformVector(Y);
		Vertex.UV = FVector2D(UVTransform.TransformPosition(FVector(Vertex.UV, 0)));
	}*/
	ARTERIES_UFUNCTION_RETURN(Obj)
}
FArteriesPoint* UArteriesObject::AddPoint()
{
	FArteriesPoint* NewPoint = new FArteriesPoint;
	Points.Add(NewPoint);
	return NewPoint;
}
FArteriesPoint* UArteriesObject::AddPoint(const FVector& InPosition)
{
	FArteriesPoint* NewPoint = new FArteriesPoint(InPosition);
	Points.Add(NewPoint);
	return NewPoint;
}
FArteriesPoint* UArteriesObject::AddPoint(const FArteriesPoint* P, FTransform* Transform)
{
	FArteriesPoint* NewPoint = new FArteriesPoint;
	NewPoint->Position = P->Position;
	//其他属性Transform？？？
	if (Transform)
		NewPoint->Position = Transform->TransformPosition(NewPoint->Position);
	Points.Add(NewPoint);
	return NewPoint;
}
FArteriesPoint* UArteriesObject::AddPointLerp(const FArteriesPoint* P0, const FArteriesPoint* P1, float Alpha)
{
	FArteriesPoint* NewPoint = new FArteriesPoint;
	NewPoint->Position = FMath::Lerp(P0->Position, P1->Position, Alpha);
	//其他属性插值？？？
	Points.Add(NewPoint);
	return NewPoint;
}
void UArteriesObject::DeletePoint(int Index)
{
	TArray<FArteriesLink*> Links = Points[Index].GetLinks(NULL, NULL, 0);
	for (FArteriesLink* Link : Links)
	{
		int Idx = Link->Primitive->Find(&Points[Index]);
		Link->Primitive->Delete(Idx);
	}
	Points.RemoveAt(Index);
}
void UArteriesObject::DeletePoint(FArteriesPoint* InPoint)
{
	DeletePoint(GetPointIndex(InPoint));
}
FArteriesPrimitive* UArteriesObject::AddPrimitive()
{
	FArteriesPrimitive* NewPrimitive = new FArteriesPrimitive;
	Primitives.Add(NewPrimitive);
	return NewPrimitive;
}
FArteriesPrimitive* UArteriesObject::AddPrimitive(const TArray<FArteriesPoint*>& InPoints, bool bClosed)
{
	FArteriesPrimitive* NewPrimitive = new FArteriesPrimitive;
	for (FArteriesPoint* Point : InPoints)
		NewPrimitive->Add(Point);
	if (bClosed)
		NewPrimitive->MakeClose();
	Primitives.Add(NewPrimitive);
	return NewPrimitive;
}
void UArteriesObject::DeletePrimitive(int Index)
{
	Primitives[Index].DeleteAllLinks();
	Primitives.RemoveAt(Index);
}
void UArteriesObject::DeletePrimitive(FArteriesPrimitive* InPrimitive)
{
	DeletePrimitive(GetPrimitiveIndex(InPrimitive));
}
void UArteriesObject::DeleteEdge(TPair<FArteriesPoint*, FArteriesPoint*> Edge)
{
	TArray<FArteriesLink*> Links = Edge.Key->GetLinks(Edge.Value, NULL, -1);
	if (Links.Num() == 2)
	{
		FArteriesPrimitive* Prim = NULL;
		FArteriesPrimitive* Other = NULL;
		if (Links[0]->Inverse)
		{
			if (!Links[1]->Inverse)
			{
				Prim = Links[1]->Primitive;
				Other = Links[0]->Primitive;
			}
			else check(0);
		}
		else
		{
			if (Links[1]->Inverse)
			{
				Prim = Links[0]->Primitive;
				Other = Links[1]->Primitive;
			}
			else check(0);
		}
		if (Prim != Other)
		{
			int InsertIndex = Prim->Points.Find(Edge.Value);
			int CutIndex = Other->Points.Find(Edge.Key);
			TArray<FArteriesPoint*> CutPoints = Other->GetCutPoints(CutIndex);
			Prim->Insert(CutPoints, InsertIndex);
			DeletePrimitive(Other);
		}
		else
		{
			TArray<int> Indices;
			for (int i = 0; i < Prim->Points.Num(); i++)
			{
				FArteriesPoint* Point = Prim->Points[i];
				if (Point == Edge.Key || Point == Edge.Value)
					Indices.Add(i);
			}
			FInt32Interval MaxInterval;
			int MaxSize = 0;
			for (int i = 0; i < Indices.Num(); i++)
			{
				int Start = Indices[i];
				int End = Indices[(i + 1) % Indices.Num()];
				int Size = (End - Start + Prim->NumPoints()) % Prim->NumPoints();
				if (MaxSize < Size)
				{
					MaxSize = Size;
					MaxInterval = FInt32Interval(Start, End);
				}
			}
			Prim->Delete((MaxInterval.Max + 1) % Prim->NumPoints(), Prim->NumPoints() - MaxSize);
		}
	}
}
void UArteriesObject::CleanPoints()
{
#if 0
	TSet<FArteriesPoint*> PointSet;
	for (FArteriesPrimitive& Primitive : Primitives)
	{
		for (FArteriesPoint* Point : Primitive.Points)
			PointSet.Add(Point);
	}
	for (int i = Points.Num() - 1; i >= 0; i--)
	{
		if (!PointSet.Contains(&Points[i]))
			Points.RemoveAt(i);
	}
#else
	for (int i = Points.Num() - 1; i >= 0; i--)
	{
		if (!Points[i].Links.Num())
			Points.RemoveAt(i);
	}
#endif
}
void UArteriesObject::CleanPrimitives()
{
	for (int i = Primitives.Num() - 1; i >= 0; i--)
	{
		FArteriesPrimitive& Prim = Primitives[i];
		if (!Prim.HasAnyValidLinks())
			Primitives.RemoveAt(i);
	}
}
FArteriesPrimitive* UArteriesObject::AddLine(FVector Origin, FVector Direction, float Length, int NumPoints)
{
	FArteriesPrimitive* Primitive = AddPrimitive();
	FVector End = Origin + Direction * Length;
	FVector TangentY = GetTangentYFromX(Direction);
	int NumSegments = FMath::Max(1, NumPoints - 1);
	for (int i = 0; i <= NumSegments; i++)
	{
		float Alpha = float(i) / NumSegments;
		FVector Position = FMath::Lerp(Origin, End, Alpha);
		Primitive->Add(AddPoint(Position));
	}
	return Primitive;
}
TArray<FArteriesPrimitive*> UArteriesObject::AddGrid(FVector Origin, FVector2D Size, int NumPointsX, int NumPointsY)
{
	TArray<FArteriesPrimitive*> OutPrims;
	if (NumPointsX >= 2 && NumPointsY >= 2)
	{
		int NumSegmentsX = NumPointsX - 1;
		int NumSegmentsY = NumPointsY - 1;
		float SizeX = Size.X / NumSegmentsX;
		float SizeY = Size.Y / NumSegmentsY;
		FVector2D Start = FVector2D(Origin) - Size / 2;
		for (int i = 0; i < NumPointsX; i++)
		{
			float X = Start.X + i * SizeX;
			for (int j = 0; j < NumPointsY; j++)
			{
				float Y = Start.Y + j * SizeY;
				AddPoint(FVector(X, Y, Origin.Z));
			}
		}
		for (int i = 0; i < NumSegmentsX; i++)
		{
			int Row0 = i * NumPointsY;
			int Row1 = i * NumPointsY + NumPointsY;
			for (int j = 0; j < NumSegmentsY; j++)
			{
				FArteriesPrimitive* Primitive = AddPrimitive();
				int I0 = Row0 + j;
				int I1 = Row1 + j;
				int I2 = Row1 + j + 1;
				int I3 = Row0 + j + 1;
				Primitive->Add(&Points[I0]);
				Primitive->Add(&Points[I1]);
				Primitive->Add(&Points[I2]);
				Primitive->Add(&Points[I3]);
				Primitive->MakeClose();
				OutPrims.Add(Primitive);
			}
		}
	}
	return MoveTemp(OutPrims);
}
FArteriesPrimitive* UArteriesObject::AddCircle(FVector Origin, FVector2D Radius, int NumPoints)
{
	FArteriesPrimitive* Primitive = AddPrimitive();
	for (int i = 0; i < NumPoints; i++)
	{
		float Alpha = float(i) / NumPoints;
		float Radian = FMath::Lerp(0.f, PI * 2, Alpha);
		FVector Position = Origin + FVector(FMath::Cos(Radian)*Radius.X, FMath::Sin(Radian)*Radius.Y, 0);
		Primitive->Add(AddPoint(Position));
	}
	Primitive->MakeClose();
	return Primitive;
}
UArteriesObject* UArteriesObject::Copy_Impl()
{
#if ARTERIES_PROFILE
	FArteriesCycleCounter Counter(FName("Copy"));
#endif
	//Source object may has Transactional flag if it's a manually editing object.
	//Copy this flag causes crash in main thread when editor process transactions.
	return (UArteriesObject*)StaticDuplicateObject(this, GetTransientPackage(), NAME_None, ~RF_Transactional);
}
void UArteriesObject::Clip_Impl(const FString& Groups, const FPlane& Plane, const FName& PositiveGroup, const FName& NegativeGroup)
{
	FArteriesPointMapper Mapper(this);
	TSet<FArteriesPrimitive*> Sets = GetPrimitives(Groups);
	for (FArteriesPrimitive* Prim : Sets)
	{
		TArray<FArteriesPoint*> PPrim;
		TArray<FArteriesPoint*> NPrim;
		Prim->Clip(Mapper, Plane, PPrim, NPrim);
		if (PPrim.Num() && NPrim.Num())
		{
			if (PositiveGroup != NAME_None)
				SetPrimitiveGroup(AddPrimitive(PPrim, true), PositiveGroup, 1);
			if (NegativeGroup != NAME_None)
				SetPrimitiveGroup(AddPrimitive(NPrim, true), NegativeGroup, 1);
			DeletePrimitive(Prim);
		}
		else if (PPrim.Num() && PositiveGroup == NAME_None || NPrim.Num() && NegativeGroup == NAME_None)
			DeletePrimitive(Prim);
	}
}
void UArteriesObject::Bridge_Impl(FArteriesPrimitive* From, FArteriesPrimitive* To)
{
	if (From->Points.Num() == To->Points.Num())
	{
		/*
		TArray<FVector> Directions;
		TArray<float> Dist = From->GetDists(&Directions);
		TArray<FIntPoint> Indices = From->GetVerticesIndices();
		TArray<FMatrix> Transforms = From->GetTransforms();
		bool bClosed = From->IsClosed();
		int BaseVertex = Vertices.Num();
		for (int i = 0; i < From->Points.Num(); i++)
		{
			FArteriesPoint* FPoint = From->Points[i];
			FArteriesPoint* TPoint = To->Points[i];
			auto AddVertices = [&](const FVector& TangentX, float U, FMatrix* UVTransform = NULL)
			{
				FMatrix TangentTransform = GetTransform(FVector::ZeroVector, TangentX);
				FVector TangentY = TPoint->Position - FPoint->Position;
				float Size = TangentY.Size();
				TangentY /= Size;
				TangentY = TangentTransform.TransformVector(TangentY);
				float U0 = U + (UVTransform ? UVTransform->TransformPosition(FPoint->Position).X : 0);
				float U1 = U + (UVTransform ? UVTransform->TransformPosition(TPoint->Position).X : 0);
				AddVertex(TangentX, TangentY, FVector2D(U0, 0));
				AddVertex(TangentX, TangentY, FVector2D(U1, Size));
			};
			FVector PrevDirection = (i == 0) ? (bClosed ? Directions.Last() : Directions[0]) : Directions[i - 1];
			FVector NextDirection = (i == From->Points.Num() - 1) ? (bClosed ? Directions[0] : Directions[i - 1]) : Directions[i];
			if (From->Points[i]->bSmooth)
				AddVertices((PrevDirection + NextDirection).GetSafeNormal(), Dist[i]);
			else
			{
				FMatrix& Transform = (i == Transforms.Num()) ? Transforms[0] : Transforms[i];
				if (i != 0)
				{
					FMatrix PrevUV = Transform * GetTransform(FPoint->Position, PrevDirection).InverseFast();
					AddVertices(PrevDirection, Dist[i], &PrevUV);
				}
				if (i != From->Points.Num() - 1)
				{
					FMatrix NextUV = Transform * GetTransform(FPoint->Position, NextDirection).InverseFast();
					AddVertices(NextDirection, Dist[i], &NextUV);
				}
			}
		}*/
		for (int i = 0; i < From->Points.Num()-1; i++)
		{
			FArteriesPrimitive* Prim = AddPrimitive();
			Prim->Add(From->Points[i]);
			Prim->Add(From->Points[i + 1]);
			Prim->Add(To->Points[i + 1]);
			Prim->Add(To->Points[i]);
			Prim->MakeClose();
		}
	}
}
UArteriesObject* UArteriesObject::Sweep_Impl(UArteriesObject* Backbones)
{
	UArteriesObject* Obj = New_Impl();
	for (FArteriesPrimitive& Backbone : Backbones->Primitives)
	{
		bool bBackboneClosed = Backbone.IsClosed();
		TArray<FVector> BDirections;
		TArray<float> BDist = Backbone.GetDists(&BDirections);
		TArray<FMatrix> Transforms = Backbone.GetTransforms();
		for (const FArteriesPrimitive& Profile : Primitives)
		{
			int BasePointIndex = Obj->Points.Num();
			TArray<float> PDist = Profile.GetDists();
			for (int i = 0; i < BDist.Num(); i++)
			{
				FMatrix& Transform = Transforms[i%Transforms.Num()];
				for (int j = 0; j < PDist.Num(); j++)
				{
					FArteriesPoint* Point = Profile.Points[j%Profile.NumPoints()];
					FArteriesPoint* NewP = Obj->AddPoint(Transform.TransformPosition(Point->Position));
				//	NewP->SetVec2(AVN_UV, FVector2D(BDist[i], PDist[j]));
				}
			}
			for (int i = 0; i < Backbone.NumSegments(); i++)
			{
				int PointThisLine = BasePointIndex + i * PDist.Num();
				int PointNextLine = PointThisLine + PDist.Num();
				for (int j = 0; j < Profile.NumSegments(); j++)
				{
					FArteriesPrimitive* Prim = Obj->AddPrimitive();
					Prim->Add(&Obj->Points[PointThisLine + j]);
					Prim->Add(&Obj->Points[PointNextLine + j]);
					Prim->Add(&Obj->Points[PointNextLine + j+1]);
					Prim->Add(&Obj->Points[PointThisLine + j+1]);
					Prim->Material = Profile.Material ? Profile.Material : Backbone.Material;
					Prim->MakeClose();
					Prim->SetVec3(AVN_TangentX, BDirections[i]);
				}
			}
		}
	}
	return Obj;
}
void UArteriesObject::Merge_Impl(UArteriesObject* Object, FTransform* Transform)
{
	TMap<FArteriesPoint*, FArteriesPoint*> PointMap;
	for (FArteriesPoint& Point : Object->Points)
	{
		FArteriesPoint* NewPoint = AddPoint(&Point, Transform);
		NewPoint->CopyValues(Point);
		PointMap.Add(&Point, NewPoint);
	}
	TMap<FArteriesPrimitive*, FArteriesPrimitive*> PrimitiveMap;
	for (FArteriesPrimitive& Primitive : Object->Primitives)
	{
		FArteriesPrimitive* NewP = AddPrimitive();
		for (int i = 0; i < Primitive.Points.Num(); i++)
			NewP->Add(PointMap[Primitive.Points[i]]);
		if (Primitive.IsClosed())
			NewP->MakeClose();
		NewP->Material = Primitive.Material;
		NewP->CopyValues(Primitive, Transform);
		PrimitiveMap.Add(&Primitive, NewP);
	}
	for (FArteriesPrimitive& Primitive : Object->Primitives)
	{
		FArteriesPrimitive* NewP = PrimitiveMap[&Primitive];
		if (Primitive.Outer)
			NewP->Outer = PrimitiveMap[Primitive.Outer];
	}
	for (auto It = Object->PointGroups.CreateIterator(); It; ++It)
	{
		FArteriesPointGroup& Src = It->Value;
		FArteriesPointGroup& Dst = PointGroups.FindOrAdd(It->Key);
		for (FArteriesPoint* Point : Src.Points)
		{
			if (PointMap.Contains(Point))
				Dst.Add(PointMap[Point]);
		}
	}
	for (auto It = Object->PrimitiveGroups.CreateIterator(); It; ++It)
	{
		FArteriesPrimitiveGroup& Src = It->Value;
		FArteriesPrimitiveGroup& Dst = PrimitiveGroups.FindOrAdd(It->Key);
		for (FArteriesPrimitive* Primitive : Src.Primitives)
		{
			if (PrimitiveMap.Contains(Primitive))
				Dst.Add(PrimitiveMap[Primitive]);
		}
	}
	for (auto It = Object->EdgeGroups.CreateIterator(); It; ++It)
	{
		FArteriesEdgeGroup& Src = It->Value;
		FArteriesEdgeGroup& Dst = EdgeGroups.FindOrAdd(It->Key);
		for (TPair<FArteriesPoint*, FArteriesPoint*>& Edge : Src.Edges)
		{
			if (PointMap.Contains(Edge.Key) && PointMap.Contains(Edge.Value))
				Dst.Add(PointMap[Edge.Key], PointMap[Edge.Value]);
		}
	}
	for (auto It = Object->InstancesMap.CreateIterator(); It; ++It)
		InstancesMap.FindOrAdd(It->Key).Merge(It->Value, Transform);
}
void UArteriesObject::Transform_Impl(const FTransform& Transform, const EArteriesGroupType GroupType, const FString& Groups)
{
	TSet<FArteriesPoint*> Sets;
	switch (GroupType)
	{
	case EArteriesGroupType::Point:
		Sets = GetPoints(Groups);
		break;
	case EArteriesGroupType::Primitive:
	{
		TSet<FArteriesPrimitive*> Prims = GetPrimitives(Groups);
		for (FArteriesPrimitive* Prim : Prims)
			Sets.Append(Prim->Points);
		break;
	}
	default:
		for (FArteriesPoint& Point : Points)
			Sets.Add(&Point);
	}
	for (FArteriesPoint* Point : Sets)
		Point->Position = Transform.TransformPosition(Point->Position);
	if (GroupType == EArteriesGroupType::Object)
	{
		for (auto It = InstancesMap.CreateIterator(); It; ++It)
			It->Value.Transform(Transform);
	}
}
void UArteriesObject::Build(TMap<UMaterialInterface*, FProcMeshSection>& Sections)
{
	TMap<FArteriesPrimitive*, TArray<FArteriesPrimitive*>> Holes;
	for (FArteriesPrimitive& Primitive : Primitives)
	{
		Primitive.CalculateTangents();
		if (Primitive.Outer)
			Holes.FindOrAdd(Primitive.Outer).Add(&Primitive);
	}
	TMap<UMaterialInterface*, TMap<FIntVector, TArray<int>>> VerticesMappers;
	for (FArteriesPrimitive& Primitive : Primitives)
		Primitive.Build(Sections.FindOrAdd(Primitive.Material), VerticesMappers.FindOrAdd(Primitive.Material), Holes.Find(&Primitive));
}
void UArteriesObject::Serialize(FArchive& Ar)
{
	TMap<FArteriesPoint*, int> PointIDs;
	TMap<FArteriesPrimitive*, int> PrimitiveIDs;
//	FArteriesRunnable::SetThreadData("PointIDs",&PointIDs);
//	FArteriesRunnable::SetThreadData("PrimitiveIDs",&PrimitiveIDs);
	if (Ar.IsSaving() || Ar.IsObjectReferenceCollector())
	{
		for (int i = 0; i < Points.Num(); i++)
			PointIDs.Add(&Points[i], i);
		for (int i = 0; i < Primitives.Num(); i++)
			PrimitiveIDs.Add(&Primitives[i], i);
		for (auto It = PointGroups.CreateIterator(); It; ++It)
		{
			It->Value.PreSave(this, PointIDs);
			if (!It->Value.Points.Num())
				It.RemoveCurrent();
		}
		for (auto It = PrimitiveGroups.CreateIterator(); It; ++It)
		{
			It->Value.PreSave(this, PrimitiveIDs);
			if (!It->Value.Primitives.Num())
				It.RemoveCurrent();
		}
		for (auto It = EdgeGroups.CreateIterator(); It; ++It)
		{
			It->Value.PreSave(this, PointIDs);
			if (!It->Value.Edges.Num())
				It.RemoveCurrent();
		}
	}
	UObject::Serialize(Ar);
//	Ar.UsingCustomVersion(FArteriesCustomVersion::GUID);
	Points.Serialize(Ar, this);
	Primitives.Serialize(Ar, this, PointIDs, PrimitiveIDs);
	if (Ar.IsLoading())
	{
		for (auto It = PointGroups.CreateIterator(); It; ++It) It->Value.PostLoad(this);
		for (auto It = PrimitiveGroups.CreateIterator(); It; ++It) It->Value.PostLoad(this);
		for (auto It = EdgeGroups.CreateIterator(); It; ++It) It->Value.PostLoad(this);
	}
}
void UArteriesObject::PostLoad()
{
	RebuildLinks();
	UObject::PostLoad();
}
void UArteriesObject::RemoveAllLinks()
{
	for (FArteriesPoint& Point : Points)
		Point.Links.Empty();
}
void UArteriesObject::RebuildLinks()
{
	RemoveAllLinks();
	for (FArteriesPrimitive& Primitive : Primitives)
	{
		for (int i = 0; i < Primitive.NumSegments(); i++)
		{
			FArteriesPoint* This = Primitive.GetPoint(i);
			FArteriesPoint* Next = Primitive.NextPoint(i);
			This->AddLink(Next, &Primitive, false);
			Next->AddLink(This, &Primitive, true);
		}
	}
	for (FArteriesPoint& Point : Points)
		Point.SortLinksForRoads();
}
FArteriesPointMapper::FArteriesPointMapper(UArteriesObject* InObject, const FString& Groups, float InSnapDist):Object(InObject), SnapDist(InSnapDist)
{
	TSet<FArteriesPoint*> Set = Object->GetPoints(Groups);
	for (FArteriesPoint* Point : Set)
		Points.FindOrAdd(Point->IntKey(SnapDist)).Add(Point);
}
FArteriesPoint* FArteriesPointMapper::GetPoint(const FVector& InPosition)
{
	FIntVector Key = GetIntVector(InPosition, SnapDist);
	TArray<FArteriesPoint*>& Pts = Points.FindOrAdd(Key);
	if (!Pts.Num())
	{
		FArteriesPoint* NewP = Object->AddPoint(InPosition);
		Pts.Add(NewP);
		return NewP;
	}
	return Pts[0];
}
FArteriesPoint* FArteriesPointMapper::GetPoint(const FArteriesPoint* Point)
{
	return GetPoint(Point->Position);
}
FArteriesPoint* FArteriesPointMapper::GetPointLerp(const FArteriesPoint* P0, const FArteriesPoint* P1, float Alpha)
{
	return GetPoint(FMath::Lerp(P0->Position, P1->Position, Alpha));
}
void FArteriesPointMapper::Fuse()
{
	for (auto It = Points.CreateIterator(); It; ++It)
	{
		TArray<FArteriesPoint*>& Pts = It->Value;
		if (Pts.Num() > 1)
		{
			FArteriesPoint* ToPoint = Pts[0];
			FVector Position(0, 0, 0);
			for (FArteriesPoint* Point : Pts)
				Position += Point->Position;
			ToPoint->Position = Position / Pts.Num();
			TSet<FArteriesPoint*> ReplacePoints;
			TSet<FArteriesPrimitive*> CheckPrimitives;
			for (int i = 1; i < Pts.Num(); i++)
			{
				ReplacePoints.Add(Pts[i]);
				TArray<FArteriesLink*> Links = Pts[i]->GetLinks(NULL, NULL, 0);
				for (FArteriesLink* Link : Links)
					CheckPrimitives.Add(Link->Primitive);
			}
			for (FArteriesPrimitive* Primitive : CheckPrimitives)
			{
				for (int i = 0; i < Primitive->NumPoints(); )
				{
					FArteriesPoint* FromPoint = Primitive->Points[i];
					if (ReplacePoints.Contains(FromPoint))
					{
						FArteriesPoint* Prev = Primitive->PrevPoint(i);
						FArteriesPoint* Next = Primitive->NextPoint(i);
						if (Prev == ToPoint || ReplacePoints.Contains(Prev) || Next == ToPoint || ReplacePoints.Contains(Next))
						{
							Primitive->Delete(i);
							continue;
						}
						if (Prev)
						{
							Prev->GetLinks(FromPoint, Primitive, 0)[0]->Target = ToPoint;
							FromPoint->DeleteLinks(Prev, Primitive, 1);
							ToPoint->AddLink(Prev, Primitive, 1);
						}
						if (Next)
						{
							Next->GetLinks(FromPoint, Primitive, 1)[0]->Target = ToPoint;
							FromPoint->DeleteLinks(Next, Primitive, 0);
							ToPoint->AddLink(Next, Primitive, 0);
						}
						Primitive->Points[i] = ToPoint;
					}
					i++;
				}
			}
		}
	}
	Object->CleanPoints();
}