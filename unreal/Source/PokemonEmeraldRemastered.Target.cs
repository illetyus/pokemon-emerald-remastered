using UnrealBuildTool;
using System.Collections.Generic;

public class PokemonEmeraldRemasteredTarget : TargetRules
{
    public PokemonEmeraldRemasteredTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("PokemonEmeraldRemastered");
        PreBuildSteps.Add("python \"$(ProjectDir)/../tools/build_unreal_core_catalogs.py\"");
    }
}
