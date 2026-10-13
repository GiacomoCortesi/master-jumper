using UnrealBuildTool;

public class MasterJumperEditorTarget : TargetRules
{
	public MasterJumperEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("MasterJumper");
	}
}
