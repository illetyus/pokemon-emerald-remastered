using UnrealBuildTool;
using System.IO;

public class PokemonEmeraldRemastered : ModuleRules
{
    public PokemonEmeraldRemastered(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        // Portable C embeds and per-file helper namespaces need separate translation units.
        bUseUnity = false;

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
        PrivateIncludePaths.Add(Path.Combine(RepoRoot, "unreal", "Intermediate", "CoreCatalogs"));

        PublicDefinitions.Add("REMASTER_UNREAL_RUNTIME=1");
    }
}
