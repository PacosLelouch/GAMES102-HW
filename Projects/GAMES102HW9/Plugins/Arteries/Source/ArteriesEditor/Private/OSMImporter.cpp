// Author: LiJiayu (JerryLi)
// Mail: lijiayu83@gmail.com (fullike@163.com)
// Copyright 2019. All Rights Reserved.

#include "OSMImporter.h"
#define EARTH_RADIUS		(6378137)
#define GEOM_UNITSINDEGREE	(10000000.0)
#ifndef M_PI
#define M_PI				(3.14159265358979323846)
#endif
#define DEG2RAD(a)   ((a) / (180 / M_PI))
#define RAD2DEG(a)   ((a) * (180 / M_PI))

double y2lat_m(double y)
{
	return RAD2DEG(2 * atan(exp(y / EARTH_RADIUS)) - M_PI / 2);
}
double x2lon_m(double x)
{
	return RAD2DEG(x / EARTH_RADIUS);
}

double lat2y_m(double lat)
{
	return log(tan(DEG2RAD(lat) / 2 + M_PI / 4)) * EARTH_RADIUS;
}
double lon2x_m(double lon)
{
	return DEG2RAD(lon) * EARTH_RADIUS;
}

FVector2D Project(const FIntPoint& point, const FIntPoint& ref, float ScaleFactor = 1)
{
	//参考经纬度算出绝对XY
	double X = lon2x_m(ref.X / GEOM_UNITSINDEGREE);
	double Y = lat2y_m(ref.Y / GEOM_UNITSINDEGREE);
	//
	double x = lon2x_m(point.X / GEOM_UNITSINDEGREE);
	double y = lat2y_m(point.Y / GEOM_UNITSINDEGREE);

	return FVector2D(x - X, y - Y)*ScaleFactor;
}

FIntPoint UnProject(const FVector2D& point, const FIntPoint& ref, float ScaleFactor = 1)
{
	//参考经纬度算出绝对XY
	double X = lon2x_m(ref.X / GEOM_UNITSINDEGREE);
	double Y = lat2y_m(ref.Y / GEOM_UNITSINDEGREE);

	double lon = x2lon_m((point.X + X) / ScaleFactor);
	double lat = y2lat_m((point.Y + Y) / ScaleFactor);

	return FIntPoint(lon*GEOM_UNITSINDEGREE, lat*GEOM_UNITSINDEGREE);
}

void FOSMImporter::SubmitAttributes(const TCHAR* ElementName)
{
	if (FArteriesPoint* Point = CurrentElement->ToPoint())
	{
		if (Attributes.Contains("id"))
			Points.Add(FCString::Atoi64(*Attributes["id"]), Point);
		if (Attributes.Contains("lon") && Attributes.Contains("lat"))
		{
			double lon = FCString::Atod(*Attributes[TEXT("lon")]);
			double lat = FCString::Atod(*Attributes[TEXT("lat")]);
			FVector2D Position = Project(FIntPoint(lon*GEOM_UNITSINDEGREE, lat*GEOM_UNITSINDEGREE), LonlatCenter);
			Point->Position = FVector(Position,0);
		}
	}
	if (FArteriesPrimitive* Primitive = CurrentElement->ToPrimitive())
	{
		if (Attributes.Contains("id"))
			Primitives.Add(FCString::Atoi64(*Attributes["id"]), Primitive);
		if (!FCString::Strcmp(ElementName, TEXT("nd")))
		{
			int64 ref = FCString::Atoi64(*Attributes[TEXT("ref")]);
			Primitive->Add(Points[ref]);
		}
		else if (!FCString::Strcmp(ElementName, TEXT("tag")))
		{
			Primitive->SetStr(*Attributes["k"], Attributes["v"]);
		}
	}
}
void FOSMImporter::LoadXML(const FString& Filename)
{
	FText OutError;
	int OutLineNum;
	FFastXml::ParseXmlFile(this, *Filename, nullptr, nullptr, true, true, OutError, OutLineNum);
}
bool FOSMImporter::ProcessElement(const TCHAR* ElementName, const TCHAR* ElementData, int32 XmlFileLineNumber)
{
	if (!FCString::Strcmp(ElementName, TEXT("node")))
		CurrentElement = Object->AddPoint();
	else if (!FCString::Strcmp(ElementName, TEXT("way")))
		CurrentElement = Object->AddPrimitive();
	else if (CurrentElement)
		SubmitAttributes(NULL);
	Attributes.Empty();
	return true;
}
bool FOSMImporter::ProcessAttribute(const TCHAR* AttributeName, const TCHAR* AttributeValue)
{
	Attributes.Add(AttributeName, AttributeValue);
	return true;
}
bool FOSMImporter::ProcessClose(const TCHAR* ElementName)
{
	if (!FCString::Strcmp(ElementName, TEXT("bounds")))
	{
		FIntPoint MinLonlat = FIntPoint(FCString::Atod(*Attributes[TEXT("minlon")])*GEOM_UNITSINDEGREE, FCString::Atod(*Attributes[TEXT("minlat")])*GEOM_UNITSINDEGREE);
		FIntPoint MaxLonlat = FIntPoint(FCString::Atod(*Attributes[TEXT("maxlon")])*GEOM_UNITSINDEGREE, FCString::Atod(*Attributes[TEXT("maxlat")])*GEOM_UNITSINDEGREE);
		LonlatCenter = MinLonlat / 2 + MaxLonlat / 2;
	}
	else if (!FCString::Strcmp(ElementName, TEXT("node")) || !FCString::Strcmp(ElementName, TEXT("way")))
		CurrentElement = NULL;
	else if (CurrentElement)
		SubmitAttributes(ElementName);	
	return true;
}