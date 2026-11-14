using UnrealBuildTool;

public class RealGuitarGameTarget : TargetRules
{
    public RealGuitarGameTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_5;
        ExtraModuleNames.Add("RealGuitarGame");
    }
}
