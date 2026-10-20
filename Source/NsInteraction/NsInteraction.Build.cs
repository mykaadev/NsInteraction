// Copyright (C) 2025 nulled.softworks. All rights reserved.

using UnrealBuildTool;

public class NsInteraction : ModuleRules
{
    public NsInteraction(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        // Engine modules only. NsInteraction depends on no other plugin
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
        });
    }
}
