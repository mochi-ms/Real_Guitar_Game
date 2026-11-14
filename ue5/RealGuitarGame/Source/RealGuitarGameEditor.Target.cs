using UnrealBuildTool;

public class RealGuitarGameEditorTarget : TargetRules
{
    public RealGuitarGameEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_5;
        ExtraModuleNames.Add("RealGuitarGame");
    }
}
