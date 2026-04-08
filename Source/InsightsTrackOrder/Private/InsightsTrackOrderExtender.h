#pragma once

#include "Insights/ITimingViewExtender.h"

/**
 * Timing view extender that reorders Insights tracks based on a config file.
 *
 * Reads "[InsightsTrackOrder] PriorityTracks" array and orders the lanes
 * so that configured priority tracks appear first (top-most) in the
 * Timing View, in the order they are listed.
 *
 * Tracks are matched exactly against their display name.
 * 
 * Example config file:
 *   [InsightsTrackOrder]
 *   +PriorityTracks=GameThread
 *   +PriorityTracks=RenderThread 0
 *   +PriorityTracks=RHIThread
 *   +PriorityTracks=RHISubmissionThread
 *   +PriorityTracks=GPU
 *   +PriorityTracks=GPU0-Graphics0
 *   +PriorityTracks=GPU0-Copy0
 *   +PriorityTracks=GPU0-Compute0
 *   +PriorityTracks=RHIInterruptThread
 */
class FInsightsTrackOrderExtender : public UE::Insights::Timing::ITimingViewExtender
{
public:
	FInsightsTrackOrderExtender();

	// ~Begin ITimingViewExtender
	virtual void OnBeginSession(UE::Insights::Timing::ITimingViewSession& InSession) override;
	virtual void OnEndSession(UE::Insights::Timing::ITimingViewSession& InSession) override;
	virtual void Tick(const UE::Insights::Timing::FTimingViewExtenderTickParams& Params) override;
	// ~End ITimingViewExtender

private:
	void LoadConfig();

	/** Ordered list of track name patterns from config. */
	TArray<FString> PriorityTrackNames;

	/** The session we're attached to (non-owning). */
	UE::Insights::Timing::ITimingViewSession* Session = nullptr;

	bool bForceRescan = false;
};
