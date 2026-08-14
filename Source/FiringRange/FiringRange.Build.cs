// Copyright zutemiss & dshadykh. Educational project.

using UnrealBuildTool;

/**
 * Module rules for the single runtime module of the project.
 *
 * Every dependency below ships with the engine on all desktop platforms, so the
 * module builds without any third party or platform specific library.
 */
public class FiringRange : ModuleRules
{
	public FiringRange(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",             // containers, maths, strings
			"CoreUObject",      // UObject reflection system
			"Engine",           // actors, components, gameplay framework
			"InputCore",        // FKey definitions
			"EnhancedInput",    // input actions and mapping contexts
			"AIModule",         // AAIController for the moving targets
			"NavigationSystem", // navigation queries used by the target AI
			"GameplayTasks",    // required transitively by AIModule
			"PhysicsCore",      // surface types used by impact feedback
			"UMG",              // viewport widget management
			"Slate",            // menu widgets
			"SlateCore"         // brushes, fonts and styling primitives
		});

		// Include paths are relative to the module root so that the same string
		// resolves on every operating system.
		PublicIncludePaths.Add(ModuleDirectory);
	}
}
