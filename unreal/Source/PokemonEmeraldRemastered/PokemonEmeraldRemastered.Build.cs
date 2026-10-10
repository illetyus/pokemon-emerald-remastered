using UnrealBuildTool;
using System.IO;

public class PokemonEmeraldRemastered : ModuleRules
{
    public PokemonEmeraldRemastered(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "Json",
            "UMG",
            "EnhancedInput",
            "Niagara",
            "DeveloperSettings"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "ApplicationCore",
            "Slate",
            "SlateCore"
        });

        string RepoRoot = Path.GetFullPath(Path.Combine(ModuleDirectory, "..", "..", ".."));
        PublicIncludePaths.Add(Path.Combine(RepoRoot, "core", "include"));

        PublicDefinitions.Add("REMASTER_UNREAL_RUNTIME=1");
    }
}
