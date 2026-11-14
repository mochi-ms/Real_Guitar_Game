using UnrealBuildTool;

public class RealGuitarGameEditorTarget : TargetRules
{
    public RealGuitarGameEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
        ExtraModuleNames.Add("RealGuitarGame");
    }
}
