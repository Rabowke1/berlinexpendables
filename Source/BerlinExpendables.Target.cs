using UnrealBuildTool;
using System.Collections.Generic;

public class BerlinExpendablesTarget : TargetRules
{
	public BerlinExpendablesTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("BerlinExpendables");
	}
}
