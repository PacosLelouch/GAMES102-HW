// Author: LiJiayu (JerryLi)
// Mail: lijiayu83@gmail.com (fullike@163.com)
// Copyright 2019. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "Arteries.generated.h"

#define ARTERIES_PROFILE				1
#define ARTERIES_PRIMITIVE_VALIDATION	1

class ARTERIES_API FArteriesCycleCounter
{
public:
	struct FNode
	{
		FNode(double Delta) : CallTimes(1), TotalCycles(Delta) {}
		int CallTimes;
		double TotalCycles;
	};
	FArteriesCycleCounter(const FName& InName);
	~FArteriesCycleCounter();
	static void Empty() { Counter.Empty(); }
	static void Flush(const FName& CallerName);
private:
	static TMap<FName, FNode> Counter;
	FName Name;
	double AccumulatedTime;
};

UCLASS(config = Engine, defaultconfig)
class ARTERIES_API UArteriesSettings : public UObject
{
	GENERATED_UCLASS_BODY()
	UPROPERTY(EditAnywhere, config, Category = Settings)
	bool bBuildWhenPropertiesChanged;
};

/**
 * The public interface to this module.  In most cases, this interface is only public to sibling modules 
 * within this plugin.
 */
class FArteriesModule : public IModuleInterface
{

public:

	/**
	 * Singleton-like access to this module's interface.  This is just for convenience!
	 * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static inline FArteriesModule& Get()
	{
		return FModuleManager::LoadModuleChecked< FArteriesModule >( "Arteries" );
	}

	/**
	 * Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	 *
	 * @return True if the module is loaded and ready to use
	 */
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded( "Arteries" );
	}

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

DECLARE_LOG_CATEGORY_EXTERN(LogArteries, Verbose, All);