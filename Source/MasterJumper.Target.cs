using UnrealBuildTool;

public class MasterJumperTarget : TargetRules
{
	public MasterJumperTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("MasterJumper");
	}
}
