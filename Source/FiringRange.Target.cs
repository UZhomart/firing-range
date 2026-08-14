// Copyright zutemiss & dshadykh. Educational project.

using UnrealBuildTool;
using System.Collections.Generic;

/**
 * Build target for the packaged game.
 *
 * No platform specific settings are declared here on purpose: the project has
 * to cook and package identically on Windows, Linux and macOS.
 */
public class FiringRangeTarget : TargetRules
{
	public FiringRangeTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;

		// Modern defaults: implicit "Core/Public" style includes are off, which
		// forces fully qualified include paths. That is exactly what keeps the
		// code compiling on case sensitive file systems such as ext4 or APFS.
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

		ExtraModuleNames.Add("FiringRange");
	}
}
