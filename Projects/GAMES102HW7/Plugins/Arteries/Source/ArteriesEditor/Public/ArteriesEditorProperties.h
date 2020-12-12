// Author: LiJiayu (JerryLi)
// Mail: lijiayu83@gmail.com (fullike@163.com)
// Copyright 2019. All Rights Reserved.

#pragma once
#include "KismetCompiler.h"
#include "ArteriesPoint.h"
#include "ArteriesPrimitive.h"
#include "ArteriesEditorProperties.generated.h"

class IAssetEditorInstance;
class FArteriesEditorMode;
class UArteriesObject;
UCLASS()
class UArteriesEditorProperties :public UObject
{
	GENERATED_UCLASS_BODY()
public:
	void OnSelectionChanged();
	virtual bool CanEditChange(const UProperty* InProperty) const;
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent);

	UPROPERTY(EditAnywhere, Category = "Arteries")
	UArteriesObject* CurrentObject;
	UPROPERTY(EditAnywhere, Category = "Element")
	FVector Position;
	UPROPERTY(EditAnywhere, Category = "Primitive")
	UMaterialInterface* Material;
	FArteriesEditorMode* Mode;
};