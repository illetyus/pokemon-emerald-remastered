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
    }
}
