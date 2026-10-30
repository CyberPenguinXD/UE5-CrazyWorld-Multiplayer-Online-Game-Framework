// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CoopAdventure : ModuleRules
{
    public CoopAdventure(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "EnhancedInput",
            "AIModule",
            "StateTreeModule",
            "GameplayStateTreeModule",
            "UMG",
            "Slate",
            "SlateCore",        // 关键：添加这个，UMG需要
            "OnlineSubsystem",
            "OnlineSubsystemSteam",
            "Projects",
            "GameplayTags",
            "Niagara",
            "Sockets",        // 新增
            "Networking"      // 新增
        });

        PrivateDependencyModuleNames.AddRange(new string[] {
            // 如果需要编辑器功能（如编辑器工具），打包时这些会被自动排除
            // "UnrealEd", 
            // "EditorStyle"
        });

        PublicIncludePaths.AddRange(new string[] {
            "CoopAdventure",
            "CoopAdventure/GameAbilityFile",      // 建议添加：你的技能系统路径
            "CoopAdventure/InventoryFile",        // 建议添加：背包系统路径
            "CoopAdventure/PlayerStateFile",      // 建议添加：PlayerState路径
            "CoopAdventure/AGameplayFile",        // 建议添加：身份系统路径
        });
    }
}