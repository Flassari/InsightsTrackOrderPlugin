using UnrealBuildTool;

public class InsightsTrackOrder : ModuleRules
{
	public InsightsTrackOrder(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Slate",
				"SlateCore",
				"TraceInsights",
				"TraceServices",
			}
		);
	}
}
