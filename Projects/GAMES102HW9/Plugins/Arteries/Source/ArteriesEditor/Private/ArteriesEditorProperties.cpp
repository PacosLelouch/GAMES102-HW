// Author: LiJiayu (JerryLi)
// Mail: lijiayu83@gmail.com (fullike@163.com)
// Copyright 2019. All Rights Reserved.

#include "ArteriesEditorProperties.h"
#include "ArteriesEditorMode.h"
#include "ArteriesObject.h"
#include "ArteriesActor.h"
#include "ArteriesEditor.h"
#include "Toolkits/AssetEditorManager.h"
#include "EditorModeManager.h"
#include "Async/Async.h"
#include "K2Node_CallFunction.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet2/KismetEditorUtilities.h"

UArteriesEditorProperties::UArteriesEditorProperties(const FObjectInitializer& ObjectInitializer) :UObject(ObjectInitializer), CurrentObject(NULL)
{
}
void UArteriesEditorProperties::OnSelectionChanged()
{
	Position = Mode->SelectionBounds.GetCenter();
	if (Mode->GetSelectionType() == FArteriesPrimitive::StaticStruct())
		Material = Mode->SelectedElements[FSetElementId::FromInteger(0)]->ToPrimitive()->Material;
	else
		Material = NULL;
}
bool UArteriesEditorProperties::CanEditChange(const UProperty* InProperty) const
{
	/*
	const FName PropertyName = InProperty->GetFName();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UArteriesEditorProperties, CurrentObject))
		return Blueprint == NULL;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UArteriesEditorProperties, LocalObjects))
		return Blueprint != NULL;*/
	return UObject::CanEditChange(InProperty);
}
void UArteriesEditorProperties::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	UProperty* PropertyThatChanged = PropertyChangedEvent.MemberProperty;
	if (PropertyThatChanged)
	{
		const FName PropertyName = PropertyThatChanged->GetFName();
		if (PropertyName == GET_MEMBER_NAME_CHECKED(UArteriesEditorProperties, CurrentObject))
			Mode->SetArteriesObject(CurrentObject);
		else if (PropertyName == GET_MEMBER_NAME_CHECKED(UArteriesEditorProperties, Position))
		{
			FVector Offset = Position - Mode->SelectionBounds.GetCenter();
			Mode->ApplyOffset(Offset);
		}
		else if (PropertyName == GET_MEMBER_NAME_CHECKED(UArteriesEditorProperties, Material))
		{
			for (FArteriesElement* Element : Mode->SelectedElements)
			{
				if (FArteriesPrimitive* Primitive = Element->ToPrimitive())
					Primitive->Material = Material;
			}
		}
	}
	Super::PostEditChangeProperty(PropertyChangedEvent);
}