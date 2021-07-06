// Author: LiJiayu (JerryLi)
// Mail: lijiayu83@gmail.com (fullike@163.com)
// Copyright 2019. All Rights Reserved.

#pragma once
#include "FastXml.h"
#include "ArteriesObject.h"
class FOSMImporter: public IFastXmlCallback
{
public:
	FOSMImporter(UArteriesObject* InObject) :Object(InObject) {}
	virtual ~FOSMImporter() {}
	void SubmitAttributes(const TCHAR* ElementName);
	void LoadXML(const FString& Filename);
	//IFastXmlCallback
	bool ProcessXmlDeclaration(const TCHAR* ElementData, int32 XmlFileLineNumber) { return true; }
	bool ProcessElement(const TCHAR* ElementName, const TCHAR* ElementData, int32 XmlFileLineNumber);
	bool ProcessAttribute(const TCHAR* AttributeName, const TCHAR* AttributeValue);
	bool ProcessClose(const TCHAR* ElementName);
	bool ProcessComment(const TCHAR* CommentName) { return true; }
	UArteriesObject* Object;
	FArteriesElement* CurrentElement;
	FIntPoint LonlatCenter;
	TMap<int64, FArteriesPoint*> Points;
	TMap<int64, FArteriesPrimitive*> Primitives;
	TMap<FName, FString> Attributes;
};