using UnrealBuildTool;
using System.Collections.Generic;

public class PokemonEmeraldRemasteredEditorTarget : TargetRules
{
    public PokemonEmeraldRemasteredEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("PokemonEmeraldRemastered");
        PreBuildSteps.Add("python \"$(ProjectDir)/../tools/build_unreal_core_catalogs.py\"");
    }
}
