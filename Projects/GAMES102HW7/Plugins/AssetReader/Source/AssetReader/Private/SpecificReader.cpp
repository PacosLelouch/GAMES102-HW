// Copyright 2020 PacosLelouch, Inc. All Rights Reserved.
// https://github.com/PacosLelouch/

#include "SpecificReader.h"
#include "Engine/StaticMesh.h"
#include "Engine/ObjectLibrary.h"
#include "Engine/Texture2D.h"
#include "IImageWrapperModule.h"
#include "IImageWrapper.h"
#include "Modules/ModuleManager.h"
#include "Kismet/KismetRenderingLibrary.h"
#if WITH_EDITOR
#include "ObjectTools.h"
#endif

void USpecificReader::Load(TArray<FAssetData>& OutAssetArray, TSubclassOf<UObject> Class, FString Path)
{
	//UObjectLibrary* ObjectLibrary = UObjectLibrary::CreateLibrary(Class, false, GIsEditor);
	UObjectLibrary* ObjectLibrary = UObjectLibrary::CreateLibrary(Class, false, !GWorld->HasBegunPlay());

	ObjectLibrary->LoadAssetDataFromPath(Path);
	ObjectLibrary->GetAssetDataList(OutAssetArray);

	//for (int32 i = 0; i < OutAssetArray.Num(); ++i)
	//{
	//	UE_LOG(LogTemp, Error, TEXT("%s"), *OutAssetArray[i].ObjectPath.ToString());
	//}
}

UTexture2D* USpecificReader::GetThumbnailFromAssetData(const FAssetData& AssetData)
{
#if WITH_EDITOR
	UObject* Object = AssetData.GetAsset();
	int32 ImageRes = 128;
	FObjectThumbnail ObjThumnail;
	//= ThumbnailTools::GetThumbnailForObject(InObject);
	ThumbnailTools::RenderThumbnail(Object, ImageRes, ImageRes, ThumbnailTools::EThumbnailTextureFlushMode::AlwaysFlush, NULL, &ObjThumnail);
	TArray<uint8> ThumnailDatat = ObjThumnail.GetUncompressedImageData();

	TArray<FColor> ImageRawColor;

	for (int i = 0; i < ThumnailDatat.Num(); i += 4)
	{
		ImageRawColor.Add(FColor(ThumnailDatat[i + 2], ThumnailDatat[i + 1], ThumnailDatat[i], ThumnailDatat[i + 3]));
	}

	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::JPEG);
	ImageWrapper->SetRaw(ImageRawColor.GetData(), ImageRawColor.GetAllocatedSize(), ImageRes, ImageRes, ERGBFormat::RGBA, 8);
	const TArray<uint8> ImageData = ImageWrapper->GetCompressed(100);
	UTexture2D* ReTexture2d = UKismetRenderingLibrary::ImportBufferAsTexture2D(Object->GetWorld(), ImageData);
	return ReTexture2d;
#endif
	return nullptr;
}
