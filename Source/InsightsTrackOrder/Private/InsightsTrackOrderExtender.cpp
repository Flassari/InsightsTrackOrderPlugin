#include "InsightsTrackOrderExtender.h"

#include "Insights/ITimingViewSession.h"
#include "Insights/IUnrealInsightsModule.h"
#include "Insights/ViewModels/BaseTimingTrack.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY_STATIC(LogInsightsTrackOrder, Log, All);

FInsightsTrackOrderExtender::FInsightsTrackOrderExtender()
{
	LoadConfig();
}

void FInsightsTrackOrderExtender::LoadConfig()
{
	// Load from the plugin's Config/InsightsTrackOrder.ini file.
	const FString IniConfigPath = FPaths::Combine(
		FPaths::EnginePluginsDir(),
		TEXT("InsightsTrackOrder/Config/InsightsTrackOrder.ini")
	);
	if (FPaths::FileExists(IniConfigPath))
	{
		UE_LOG(LogInsightsTrackOrder, Log, TEXT("Loading plugin config from: %s"), *IniConfigPath);

		FConfigFile PluginConfig;
		PluginConfig.Combine(IniConfigPath);
		PluginConfig.GetArray(TEXT("InsightsTrackOrder"), TEXT("PriorityTracks"), PriorityTrackNames);
	}
	else
	{
		UE_LOG(LogInsightsTrackOrder, Error, TEXT("Plugin config file not found: %s"), *IniConfigPath);
	}
}

void FInsightsTrackOrderExtender::OnBeginSession(UE::Insights::Timing::ITimingViewSession& InSession)
{
	if (InSession.GetName() != FInsightsManagerTabs::TimingProfilerTabId)
	{
		return;
	}
	Session = &InSession;
	bForceRescan = true;
}

void FInsightsTrackOrderExtender::OnEndSession(UE::Insights::Timing::ITimingViewSession& InSession)
{
	if (InSession.GetName() != FInsightsManagerTabs::TimingProfilerTabId)
	{
		return;
	}
	Session = nullptr;
}

void FInsightsTrackOrderExtender::Tick(const UE::Insights::Timing::FTimingViewExtenderTickParams& Params)
{
	if (PriorityTrackNames.Num() == 0 || !Session)
	{
		return;
	}

	// Check periodically (every second at 60fps) so we catch newly-added tracks (threads that appear after session start).
	if (!bForceRescan && (GFrameCounter % 60) != 0)
	{
		return;
	}
	bForceRescan = false;

	// Collect all scrollable tracks, and figure out which match our patterns.
	struct FTrackEntry
	{
		TSharedPtr<FBaseTimingTrack> Track;
		int32 PrioritizedIndex; // -1 = not prioritized
		int32 OriginalOrder;
	};

	TArray<FTrackEntry> AllScrollableTracks;
	Session->EnumerateTracks([&AllScrollableTracks](TSharedPtr<FBaseTimingTrack> Track)
	{
		if (Track.IsValid() && Track->GetLocation() == ETimingTrackLocation::Scrollable)
		{
			AllScrollableTracks.Add({ Track, -1, Track->GetOrder() });
		}
	});

	// Match tracks to priority track names exactly.
	// If a track matches multiple priority names, the earliest (lowest index) wins.
	for (FTrackEntry& ScrollableTrack : AllScrollableTracks)
	{
		const FString& TrackName = ScrollableTrack.Track->GetName();
		for (int32 i = 0; i < PriorityTrackNames.Num(); i++)
		{
			if (TrackName.Equals(PriorityTrackNames[i]))
			{
				if (ScrollableTrack.PrioritizedIndex < 0 || i < ScrollableTrack.PrioritizedIndex)
				{
					ScrollableTrack.PrioritizedIndex = i;
				}
			}
		}
	}

	// Sort tracks by priority priority first (config order), then by their original order.
	AllScrollableTracks.Sort([](const FTrackEntry& A, const FTrackEntry& B)
	{
		const bool bAIsPrioritized = (A.PrioritizedIndex >= 0);
		const bool bBIsPrioritized = (B.PrioritizedIndex >= 0);
		if (bAIsPrioritized != bBIsPrioritized)
		{
			return bAIsPrioritized; // Prioritized before non-prioritized
		}
		if (bAIsPrioritized && bBIsPrioritized)
		{
			if (A.PrioritizedIndex != B.PrioritizedIndex)
			{
				return A.PrioritizedIndex < B.PrioritizedIndex;
			}
		}
		return A.OriginalOrder < B.OriginalOrder;
	});

	// Assign a new contiguous order in the GPU range (since GPU is normally the first scrollable track).
	// All tracks (GPU, CPU, everything) will be packed into this space.
	constexpr int32 BaseOrder = FTimingTrackOrder::Gpu;
	constexpr int32 Spacing = 100; // same spacing the engine uses
	bool bAnyChanged = false;
	for (int32 i = 0; i < AllScrollableTracks.Num(); ++i)
	{
		const int32 NewOrder = BaseOrder + i * Spacing;
		FTrackEntry& ScrollableTrack = AllScrollableTracks[i];
		if (ScrollableTrack.OriginalOrder >= BaseOrder && ScrollableTrack.Track->GetOrder() != NewOrder)
		{
			ScrollableTrack.Track->SetOrder(NewOrder);
			bAnyChanged = true;
		}
	}

	if (bAnyChanged)
	{
		Session->InvalidateScrollableTracksOrder();
	}
}
