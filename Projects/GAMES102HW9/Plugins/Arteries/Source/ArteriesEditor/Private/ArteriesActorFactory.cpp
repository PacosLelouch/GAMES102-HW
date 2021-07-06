// Author: LiJiayu (JerryLi)
// Mail: lijiayu83@gmail.com (fullike@163.com)
// Copyright 2019. All Rights Reserved.

#include "ArteriesActorFactory.h"
#include "ArteriesActor.h"
UArteriesActorFactory::UArteriesActorFactory(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	DisplayName = NSLOCTEXT("Arteries", "ArteriesFactoryDisplayName", "Add Arteries Graph Actor");
	NewActorClass = AArteriesActor::StaticClass();
}

void UArteriesActorFactory::PostSpawnActor(UObject* Asset, AActor* NewActor)
{
	Super::PostSpawnActor(Asset, NewActor);
	if (NewActor->HasAnyFlags(RF_Transactional))
	{
		AArteriesActor* TypedActor = CastChecked<AArteriesActor>(NewActor);
		TypedActor->BuildObject(Cast<UArteriesObject>(Asset));
	}
}

void UArteriesActorFactory::PostCreateBlueprint(UObject* Asset, AActor* CDO)
{
	if (UArteriesObject* Graph = Cast<UArteriesObject>(Asset))
	{
		if (AArteriesActor* TypedActor = Cast<AArteriesActor>(CDO))
		{
		//	UPaperFlipbookComponent* RenderComponent = TypedActor->GetRenderComponent();
		//	check(RenderComponent);

		//	RenderComponent->SetFlipbook(Flipbook);
		}
	}
}

bool UArteriesActorFactory::CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg)
{
	if (AssetData.IsValid() && AssetData.GetClass()->IsChildOf(UArteriesObject::StaticClass()))
	{
		return true;
	}
	else
	{
		OutErrorMsg = NSLOCTEXT("Lib3d", "CanCreateActorFrom_NoGraph", "No graph was specified.");
		return false;
	}
}