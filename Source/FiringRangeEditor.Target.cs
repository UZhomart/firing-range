// Copyright zutemiss & dshadykh. Educational project.

using UnrealBuildTool;
using System.Collections.Generic;

/** Build target used when the project is opened inside the Unreal Editor. */
public class FiringRangeEditorTarget : TargetRules
{
	public FiringRangeEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;

		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

		ExtraModuleNames.Add("FiringRange");
	}
}
