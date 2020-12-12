// Author: LiJiayu (JerryLi)
// Mail: lijiayu83@gmail.com (fullike@163.com)
// Copyright 2019. All Rights Reserved.

#include "Arteries.h"
#include "ArteriesActor.h"
#if WITH_EDITOR
	#include "ISettingsModule.h"
#endif
#include "Modules/ModuleManager.h"

UArteriesSettings::UArteriesSettings(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer), bBuildWhenPropertiesChanged(false)
{
}

IMPLEMENT_MODULE( FArteriesModule, Arteries )
#define LOCTEXT_NAMESPACE "Arteries"
void FArteriesModule::StartupModule()
{
	FArteriesBuilder::TlsSlot = FPlatformTLS::AllocTlsSlot();
#if WITH_EDITOR
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->RegisterSettings("Project", "Plugins", "Arteries",
			LOCTEXT("RuntimeSettingsName", "Arteries"),
			LOCTEXT("RuntimeSettingsDescription", "Configure the Arteries plugin"),
			GetMutableDefault<UArteriesSettings>());
	}
#endif
}

void FArteriesModule::ShutdownModule()
{
#if WITH_EDITOR
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->UnregisterSettings("Project", "Plugins", "Arteries");
	}
#endif
	FPlatformTLS::FreeTlsSlot(FArteriesBuilder::TlsSlot);
	FArteriesBuilder::TlsSlot = 0;
}
#undef LOCTEXT_NAMESPACE
DEFINE_LOG_CATEGORY(LogArteries);
TMap<FName, FArteriesCycleCounter::FNode> FArteriesCycleCounter::Counter;
FArteriesCycleCounter::FArteriesCycleCounter(const FName& InName) :Name(InName)
{
	AccumulatedTime = FPlatformTime::Seconds();
}
FArteriesCycleCounter::~FArteriesCycleCounter()
{
	static FCriticalSection Critical;
	FScopeLock Lock(&Critical);
	double Delta = FPlatformTime::Seconds() - AccumulatedTime;
	if (FNode* Node = Counter.Find(Name))
	{
		Node->CallTimes++;
		Node->TotalCycles += Delta;
	}
	else
		Counter.Add(Name, FNode(Delta));
}
void FArteriesCycleCounter::Flush(const FName& CallerName)
{
	UE_LOG(LogArteries, Log, TEXT("========%s========"), *CallerName.ToString());
	for (auto It = Counter.CreateIterator(); It; ++It)
		UE_LOG(LogArteries, Log, TEXT("%s: %d %02lf"), *It->Key.ToString(), It->Value.CallTimes, It->Value.TotalCycles);
	Empty();
}