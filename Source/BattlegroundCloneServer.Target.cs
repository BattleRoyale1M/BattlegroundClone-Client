using UnrealBuildTool;
using System.Collections.Generic;

public class BattlegroundCloneServerTarget : TargetRules
{
	public BattlegroundCloneServerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Server;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("BattlegroundClone");
	}
}