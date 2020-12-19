// Author: LiJiayu (JerryLi)
// Mail: lijiayu83@gmail.com (fullike@163.com)
// Copyright 2019. All Rights Reserved.

#include "K2Node_SpawnActorAsync.h"
#include "KismetCompilerMisc.h"
#include "KismetCompiler.h"
#include "K2Node_CallFunction.h"
#include "K2Node_AddDelegate.h"
#include "Kismet/GameplayStatics.h"
#include "ArteriesActor.h"
#include "ArteriesLibrary.h"

struct FK2Node_SpawnActorAsyncHelper
{
	static const FName SpawnTransformPinName;
	static const FName SpawnEvenIfCollidingPinName;
	static const FName NoCollisionFailPinName;
	static const FName DelegatePinName;
	static const FName AttachToCallerPinName;
};

const FName FK2Node_SpawnActorAsyncHelper::SpawnTransformPinName(TEXT("SpawnTransform"));
const FName FK2Node_SpawnActorAsyncHelper::SpawnEvenIfCollidingPinName(TEXT("SpawnEvenIfColliding"));		// deprecated pin, name kept for backwards compat
const FName FK2Node_SpawnActorAsyncHelper::NoCollisionFailPinName(TEXT("bNoCollisionFail"));		// deprecated pin, name kept for backwards compat
const FName FK2Node_SpawnActorAsyncHelper::DelegatePinName(TEXT("Completed"));
const FName FK2Node_SpawnActorAsyncHelper::AttachToCallerPinName(TEXT("AttachToCaller"));


#define LOCTEXT_NAMESPACE "K2Node_SpawnActorAsync"

UK2Node_SpawnActorAsync::UK2Node_SpawnActorAsync(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeTooltip = LOCTEXT("NodeTooltip", "Attempts to spawn a new Actor with the specified transform");
}

UClass* UK2Node_SpawnActorAsync::GetClassPinBaseClass() const
{
	return AArteriesActor::StaticClass();
}

void UK2Node_SpawnActorAsync::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	// Transform pin
	UScriptStruct* TransformStruct = TBaseStructure<FTransform>::Get();
	UEdGraphPin* TransformPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Struct, TransformStruct, FK2Node_SpawnActorAsyncHelper::SpawnTransformPinName);

	// AttachToCaller pin
	UEdGraphPin* AttachToCallerPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Boolean, FK2Node_SpawnActorAsyncHelper::AttachToCallerPinName);

	UEdGraphNode::FCreatePinParams PinParams;
	PinParams.bIsReference = true;
	PinParams.bIsConst = true;
	if (UEdGraphPin* DelegatePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Delegate, FK2Node_SpawnActorAsyncHelper::DelegatePinName, PinParams))
	{
		FMemberReference::FillSimpleMemberReference<UFunction>(GetDelegateSignature(), DelegatePin->PinType.PinSubCategoryMemberReference);
		DelegatePin->PinFriendlyName = NSLOCTEXT("K2Node", "OnBuildCompleted", "Completed");
	}

	if (ENodeAdvancedPins::NoPins == AdvancedPinDisplay)
	{
		AdvancedPinDisplay = ENodeAdvancedPins::Hidden;
	}
}

void UK2Node_SpawnActorAsync::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
	Super::ReallocatePinsDuringReconstruction(OldPins);
}

bool UK2Node_SpawnActorAsync::IsSpawnVarPin(UEdGraphPin* Pin) const
{
	UEdGraphPin* ParentPin = Pin->ParentPin;
	while (ParentPin)
	{
		if (ParentPin->PinName == FK2Node_SpawnActorAsyncHelper::SpawnTransformPinName)
		{
			return false;
		}
		ParentPin = ParentPin->ParentPin;
	}

	return(Super::IsSpawnVarPin(Pin) &&
		Pin->PinName != FK2Node_SpawnActorAsyncHelper::SpawnTransformPinName &&
		Pin->PinName != FK2Node_SpawnActorAsyncHelper::AttachToCallerPinName &&
		Pin->PinName != FK2Node_SpawnActorAsyncHelper::DelegatePinName);
}

void UK2Node_SpawnActorAsync::GetPinHoverText(const UEdGraphPin& Pin, FString& HoverTextOut) const
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	if (UEdGraphPin* TransformPin = GetSpawnTransformPin())
	{
		K2Schema->ConstructBasicPinTooltip(*TransformPin, LOCTEXT("TransformPinDescription", "The transform to spawn the Actor with"), TransformPin->PinToolTip);
	}
	return Super::GetPinHoverText(Pin, HoverTextOut);
}

FSlateIcon UK2Node_SpawnActorAsync::GetIconAndTint(FLinearColor& OutColor) const
{
	static FSlateIcon Icon("EditorStyle", "GraphEditor.SpawnActor_16x");
	return Icon;
}

UFunction* UK2Node_SpawnActorAsync::GetDelegateSignature() const
{
	UMulticastDelegateProperty* DelegateProperty = GetDelegateProperty();
	return (DelegateProperty != nullptr) ? DelegateProperty->SignatureFunction : nullptr;
}

UMulticastDelegateProperty* UK2Node_SpawnActorAsync::GetDelegateProperty() const
{
	return FindField<UMulticastDelegateProperty>(AArteriesActor::StaticClass(), "OnBuildCompleted");
}

UEdGraphPin* UK2Node_SpawnActorAsync::GetSpawnTransformPin() const
{
	UEdGraphPin* Pin = FindPinChecked(FK2Node_SpawnActorAsyncHelper::SpawnTransformPinName);
	check(Pin->Direction == EGPD_Input);
	return Pin;
}

UEdGraphPin* UK2Node_SpawnActorAsync::GetCompletedPin() const
{
	UEdGraphPin* Pin = FindPin(FK2Node_SpawnActorAsyncHelper::DelegatePinName);
	check(Pin == nullptr || Pin->Direction == EGPD_Input);
	return Pin;
}

UEdGraphPin* UK2Node_SpawnActorAsync::GetAttachToCallerPin() const
{
	UEdGraphPin* Pin = FindPin(FK2Node_SpawnActorAsyncHelper::AttachToCallerPinName);
	check(Pin == nullptr || Pin->Direction == EGPD_Input);
	return Pin;
}

FText UK2Node_SpawnActorAsync::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	FText NodeTitle = NSLOCTEXT("K2Node", "SpawnActorAsync", "Spawn Actor Async");
	return NodeTitle;
}

bool UK2Node_SpawnActorAsync::IsCompatibleWithGraph(const UEdGraph* TargetGraph) const
{
	return Super::IsCompatibleWithGraph(TargetGraph);
}

void UK2Node_SpawnActorAsync::GetNodeAttributes(TArray<TKeyValuePair<FString, FString>>& OutNodeAttributes) const
{
	UClass* ClassToSpawn = GetClassToSpawn();
	const FString ClassToSpawnStr = ClassToSpawn ? ClassToSpawn->GetName() : TEXT("InvalidClass");
	OutNodeAttributes.Add(TKeyValuePair<FString, FString>(TEXT("Type"), TEXT("SpawnActorFromClass")));
	OutNodeAttributes.Add(TKeyValuePair<FString, FString>(TEXT("Class"), GetClass()->GetName()));
	OutNodeAttributes.Add(TKeyValuePair<FString, FString>(TEXT("Name"), GetName()));
	OutNodeAttributes.Add(TKeyValuePair<FString, FString>(TEXT("ActorClass"), ClassToSpawnStr));
}

FNodeHandlingFunctor* UK2Node_SpawnActorAsync::CreateNodeHandler(FKismetCompilerContext& CompilerContext) const
{
	return new FNodeHandlingFunctor(CompilerContext);
}

void UK2Node_SpawnActorAsync::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	static const FName BeginSpawningBlueprintFuncName = GET_FUNCTION_NAME_CHECKED(UArteriesLibrary, BeginDeferredActorSpawnFromClass);
	static const FName ActorClassParamName(TEXT("ActorClass"));
	static const FName WorldContextParamName(TEXT("WorldContextObject"));

	static const FName FinishSpawningFuncName = GET_FUNCTION_NAME_CHECKED(UArteriesLibrary, FinishSpawningActor);
	static const FName ActorParamName(TEXT("Actor"));
	static const FName TransformParamName(TEXT("SpawnTransform"));
	static const FName AttachToCallerParamName(TEXT("AttachToCaller"));

	static const FName ObjectParamName(TEXT("Object"));
	static const FName ValueParamName(TEXT("Value"));
	static const FName PropertyNameParamName(TEXT("PropertyName"));

	UK2Node_SpawnActorAsync* SpawnNode = this;
	UEdGraphPin* SpawnNodeExec = SpawnNode->GetExecPin();
	UEdGraphPin* SpawnNodeTransform = SpawnNode->GetSpawnTransformPin();
	UEdGraphPin* SpawnNodeAttachToCaller = SpawnNode->GetAttachToCallerPin();
//	UEdGraphPin* SpawnNodeCollisionHandlingOverride = GetCollisionHandlingOverridePin();
	UEdGraphPin* SpawnWorldContextPin = SpawnNode->GetWorldContextPin();
	UEdGraphPin* SpawnClassPin = SpawnNode->GetClassPin();
//	UEdGraphPin* SpawnNodeOwnerPin = SpawnNode->GetOwnerPin();
	UEdGraphPin* SpawnNodeThen = SpawnNode->GetThenPin();
	UEdGraphPin* SpawnNodeResult = SpawnNode->GetResultPin();

	// Cache the class to spawn. Note, this is the compile time class that the pin was set to or the variable type it was connected to. Runtime it could be a child.
	UClass* ClassToSpawn = GetClassToSpawn();

	UClass* SpawnClass = (SpawnClassPin != NULL) ? Cast<UClass>(SpawnClassPin->DefaultObject) : NULL;
	if (!SpawnClassPin || ((0 == SpawnClassPin->LinkedTo.Num()) && (NULL == SpawnClass)))
	{
		CompilerContext.MessageLog.Error(*LOCTEXT("SpawnActorNodeMissingClass_Error", "Spawn node @@ must have a @@ specified.").ToString(), SpawnNode, SpawnClassPin);
		// we break exec links so this is the only error we get, don't want the SpawnActor node being considered and giving 'unexpected node' type warnings
		SpawnNode->BreakAllNodeLinks();
		return;
	}

	//////////////////////////////////////////////////////////////////////////
	// create 'begin spawn' call node
	UK2Node_CallFunction* CallBeginSpawnNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(SpawnNode, SourceGraph);
	CallBeginSpawnNode->FunctionReference.SetExternalMember(BeginSpawningBlueprintFuncName, UArteriesLibrary::StaticClass());
	CallBeginSpawnNode->AllocateDefaultPins();

	UEdGraphPin* CallBeginExec = CallBeginSpawnNode->GetExecPin();
	UEdGraphPin* CallBeginWorldContextPin = CallBeginSpawnNode->FindPinChecked(WorldContextParamName);
	UEdGraphPin* CallBeginActorClassPin = CallBeginSpawnNode->FindPinChecked(ActorClassParamName);
	UEdGraphPin* CallBeginTransform = CallBeginSpawnNode->FindPinChecked(TransformParamName);
//	UEdGraphPin* CallBeginCollisionHandlingOverride = CallBeginSpawnNode->FindPinChecked(CollisionHandlingOverrideParamName);
//	UEdGraphPin* CallBeginOwnerPin = CallBeginSpawnNode->FindPinChecked(FK2Node_SpawnActorAsyncHelper::OwnerPinName);
	UEdGraphPin* CallBeginResult = CallBeginSpawnNode->GetReturnValuePin();

	// Move 'exec' connection from spawn node to 'begin spawn'
	CompilerContext.MovePinLinksToIntermediate(*SpawnNodeExec, *CallBeginExec);

	if (SpawnClassPin->LinkedTo.Num() > 0)
	{
		// Copy the 'blueprint' connection from the spawn node to 'begin spawn'
		CompilerContext.MovePinLinksToIntermediate(*SpawnClassPin, *CallBeginActorClassPin);
	}
	else
	{
		// Copy blueprint literal onto begin spawn call 
		CallBeginActorClassPin->DefaultObject = SpawnClass;
	}

	// Copy the world context connection from the spawn node to 'begin spawn' if necessary
	if (SpawnWorldContextPin)
	{
		CompilerContext.MovePinLinksToIntermediate(*SpawnWorldContextPin, *CallBeginWorldContextPin);
	}
	/*
	if (SpawnNodeOwnerPin != nullptr)
	{
		CompilerContext.MovePinLinksToIntermediate(*SpawnNodeOwnerPin, *CallBeginOwnerPin);
	}*/

	// Copy the 'transform' connection from the spawn node to 'begin spawn'
	CompilerContext.MovePinLinksToIntermediate(*SpawnNodeTransform, *CallBeginTransform);

	// Copy the 'bNoCollisionFail' connection from the spawn node to 'begin spawn'
//	CompilerContext.MovePinLinksToIntermediate(*SpawnNodeCollisionHandlingOverride, *CallBeginCollisionHandlingOverride);

	//////////////////////////////////////////////////////////////////////////
	// create 'finish spawn' call node
	UK2Node_CallFunction* CallFinishSpawnNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(SpawnNode, SourceGraph);
	CallFinishSpawnNode->FunctionReference.SetExternalMember(FinishSpawningFuncName, UArteriesLibrary::StaticClass());
	CallFinishSpawnNode->AllocateDefaultPins();

	UEdGraphPin* CallFinishExec = CallFinishSpawnNode->GetExecPin();
	UEdGraphPin* CallFinishThen = CallFinishSpawnNode->GetThenPin();
	UEdGraphPin* CallFinishActor = CallFinishSpawnNode->FindPinChecked(ActorParamName);
//	UEdGraphPin* CallFinishTransform = CallFinishSpawnNode->FindPinChecked(TransformParamName);
	UEdGraphPin* CallFinishAttachToCaller = CallFinishSpawnNode->FindPinChecked(AttachToCallerParamName);
	UEdGraphPin* CallFinishResult = CallFinishSpawnNode->GetReturnValuePin();

	// Move 'then' connection from spawn node to 'finish spawn'
	CompilerContext.MovePinLinksToIntermediate(*SpawnNodeThen, *CallFinishThen);

	// Copy transform connection
//	CompilerContext.CopyPinLinksToIntermediate(*CallBeginTransform, *CallFinishTransform);
	CompilerContext.MovePinLinksToIntermediate(*SpawnNodeAttachToCaller, *CallFinishAttachToCaller);

	// Connect output actor from 'begin' to 'finish'
	CallBeginResult->MakeLinkTo(CallFinishActor);

	// Move result connection from spawn node to 'finish spawn'
	CallFinishResult->PinType = SpawnNodeResult->PinType; // Copy type so it uses the right actor subclass
	CompilerContext.MovePinLinksToIntermediate(*SpawnNodeResult, *CallFinishResult);

	//////////////////////////////////////////////////////////////////////////
	// create 'set var' nodes

	// Get 'result' pin from 'begin spawn', this is the actual actor we want to set properties on
	UEdGraphPin* LastThen = FKismetCompilerUtilities::GenerateAssignmentNodes(CompilerContext, SourceGraph, CallBeginSpawnNode, SpawnNode, CallBeginResult, ClassToSpawn);
	UEdGraphPin* InputDelegatePin = GetCompletedPin();
	if (InputDelegatePin->LinkedTo.Num() > 0)
	{
		UK2Node_AddDelegate* AddDelegateNode = CompilerContext.SpawnIntermediateNode<UK2Node_AddDelegate>(SpawnNode, SourceGraph);
		AddDelegateNode->SetFromProperty(GetDelegateProperty(), false, nullptr);
		AddDelegateNode->AllocateDefaultPins();

		UEdGraphPin* ExecPin = AddDelegateNode->GetExecPin();
		UEdGraphPin* SelfPin = AddDelegateNode->FindPinChecked(UEdGraphSchema_K2::PN_Self);
		UEdGraphPin* DelegatePin = AddDelegateNode->GetDelegatePin();

		CallBeginResult->MakeLinkTo(SelfPin);
		CompilerContext.MovePinLinksToIntermediate(*InputDelegatePin, *DelegatePin);

		LastThen->MakeLinkTo(ExecPin);
		LastThen = AddDelegateNode->FindPinChecked(UEdGraphSchema_K2::PN_Then);
	}
	// Make exec connection between 'then' on last node and 'finish'
	LastThen->MakeLinkTo(CallFinishExec);

	// Break any links to the expanded node
	SpawnNode->BreakAllNodeLinks();
}

#undef LOCTEXT_NAMESPACE

