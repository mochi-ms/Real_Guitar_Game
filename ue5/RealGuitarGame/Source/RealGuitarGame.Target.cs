using UnrealBuildTool;

public class RealGuitarGameTarget : TargetRules
{
    public RealGuitarGameTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
        ExtraModuleNames.Add("RealGuitarGame");
    }
}
