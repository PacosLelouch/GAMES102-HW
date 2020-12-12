// Author: LiJiayu (JerryLi)
// Mail: lijiayu83@gmail.com (fullike@163.com)
// Copyright 2019. All Rights Reserved.

#include "ArteriesObjectFactory.h"
#include "ArteriesObject.h"
#include "ArteriesActor.h"
#include "ArteriesEditorMode.h"
#include "OSMImporter.h"
#include "EditorModeManager.h"
#include "UnrealEdGlobals.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonSerializer.h"
#define LOCTEXT_NAMESPACE "Arteries"

/////////////////////////////////////////////////////
// UPaperFlipbookFactory

UArteriesObjectFactory::UArteriesObjectFactory(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bText = true;
	bCreateNew = true;
	bEditAfterNew = true;
	bEditorImport = true;
	SupportedClass = UArteriesObject::StaticClass();
	Formats.Add(TEXT("osm;Arteries Object Input"));
	Formats.Add(TEXT("json;Arteries Object Input"));
	Formats.Add(TEXT("txt;Arteries Object Input"));
}
bool UArteriesObjectFactory::FactoryCanImport(const FString& Filename)
{
	FString Ext = FPaths::GetExtension(Filename);
	return Ext == TEXT("osm") || Ext == TEXT("json") || Ext == TEXT("txt");
}
UObject* UArteriesObjectFactory::ImportObject(UClass* InClass, UObject* InOuter, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, bool& OutCanceled)
{
	UObject* Result = nullptr;
	CurrentFilename = Filename;
	if (!Filename.IsEmpty())
		Result = FactoryCreateFile(InClass, InOuter, InName, Flags, *Filename, Parms, GWarn, OutCanceled);
	if (Result != nullptr)
	{
		Result->MarkPackageDirty();
		ULevel::LevelDirtiedEvent.Broadcast();
		Result->PostEditChange();
	}
	CurrentFilename = TEXT("");
	return Result;
}
UObject* UArteriesObjectFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UArteriesObject* Object = NewObject<UArteriesObject>(InParent, InClass, InName, Flags | RF_Transactional);
	return Object;
}
UObject* UArteriesObjectFactory::FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled)
{
	UArteriesObject* Object = NewObject<UArteriesObject>(InParent, InClass, InName, Flags | RF_Transactional);
	FString Ext = FPaths::GetExtension(Filename);
	if (Ext == TEXT("osm"))
	{
		FOSMImporter Importer(Object);
		Importer.LoadXML(Filename);
	}
	else if (Ext == TEXT("json"))
	{
		FString Content;
		FFileHelper::LoadFileToString(Content, *Filename);
		TSharedPtr<FJsonValue> JsonValue;
		FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Content), JsonValue);
		const TArray<TSharedPtr<FJsonValue>>& Primitives = JsonValue->AsArray();
		FArteriesPointMapper Mapper(Object);
		for (const TSharedPtr<FJsonValue>& Primitive : Primitives)
		{
			FArteriesPrimitive* Prim = Object->AddPrimitive();
			const TArray<TSharedPtr<FJsonValue>>& Points = Primitive->AsArray();
			for (const TSharedPtr<FJsonValue>& Point : Points)
			{
				const TArray<TSharedPtr<FJsonValue>>& XYZ = Point->AsArray();
				FVector Position(XYZ[0]->AsNumber(), XYZ[1]->AsNumber(), XYZ[2]->AsNumber());
				Prim->Add(Mapper.GetPoint(Position));
			}
			Prim->MakeClose();
		}
	}
	else
	{
		FArteriesPointMapper Mapper(Object);
		FArteriesPrimitive* Prim = Object->AddPrimitive();
		TArray<FString> Lines;
		if (FFileHelper::LoadFileToStringArray(Lines, *Filename))
		{
			for (FString& Line : Lines)
			{
				FVector Position;
				if (FParse::Value(*Line, TEXT("X="), Position.X) && FParse::Value(*Line, TEXT("Y="), Position.Y) && FParse::Value(*Line, TEXT("Z="), Position.Z))
					Prim->Add(Mapper.GetPoint(Position));
			}
		}
		Prim->MakeClose();
	}
	return Object;
}

FString UArteriesObjectFactory::GetDefaultNewAssetName() const
{
	return FString(TEXT("New Object"));
}

FText FArteriesObjectTypeActions::GetName() const
{
	return LOCTEXT("FArteriesObjectTypeActionsName", "Object");
}

UClass* FArteriesObjectTypeActions::GetSupportedClass() const
{
	return UArteriesObject::StaticClass();
}

#undef LOCTEXT_NAMESPACE