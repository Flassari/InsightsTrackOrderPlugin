// Copyright Epic Games, Inc. All Rights Reserved.

#include "Modules/ModuleManager.h"
#include "Features/IModularFeatures.h"

#include "InsightsTrackOrderExtender.h"

class FInsightsTrackOrderModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		Extender = MakeUnique<FInsightsTrackOrderExtender>();
		IModularFeatures::Get().RegisterModularFeature(
			UE::Insights::Timing::TimingViewExtenderFeatureName, Extender.Get());
	}

	virtual void ShutdownModule() override
	{
		if (Extender.IsValid())
		{
			IModularFeatures::Get().UnregisterModularFeature(
				UE::Insights::Timing::TimingViewExtenderFeatureName, Extender.Get());
			Extender.Reset();
		}
	}

private:
	TUniquePtr<FInsightsTrackOrderExtender> Extender;
};

IMPLEMENT_MODULE(FInsightsTrackOrderModule, InsightsTrackOrder)
