// Copyright Epic Games, Inc. All Rights Reserved.

#include "CoopAdventure.h"
#include "Modules/ModuleManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/AssetManager.h"
#include "NiagaraSystem.h"
#include "Sound/SoundCue.h"
#include "Engine/Texture2D.h"
#include "Sound/SoundAttenuation.h"

IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, CoopAdventure, "CoopAdventure");

DEFINE_LOG_CATEGORY(LogCoopAdventure);

// 在 GameInstance 初始化时自动调用强制加载（可选）
void UCoopAdventureGameInstance::Init()
{
    Super::Init();

    // 异步加载所有关键资源（推荐）
    ForceLoadAllAssets();
}

// 实现强制加载函数
void UCoopAdventureGameInstance::ForceLoadAllAssets()
{
    TArray<FSoftObjectPath> AssetsToLoad;

    // ========== 1. 图标资源 ==========
    AssetsToLoad.Add(FSoftObjectPath(TEXT("/Game/MainFile/Icon/ICON_SFireball.ICON_SFireball")));
    AssetsToLoad.Add(FSoftObjectPath(TEXT("/Game/MainFile/Icon/ICON_SIceball.ICON_SIceball")));
    AssetsToLoad.Add(FSoftObjectPath(TEXT("/Game/MainFile/Icon/ICON_SThunder.ICON_SThunder")));
    AssetsToLoad.Add(FSoftObjectPath(TEXT("/Game/MainFile/Icon/ICON_SBlade.ICON_SBlade")));
    AssetsToLoad.Add(FSoftObjectPath(TEXT("/Game/MainFile/Icon/ICON_SkillFrame.ICON_SkillFrame")));

    // ========== 2. 枪械特效 Niagara ==========
    AssetsToLoad.Add(FSoftObjectPath(TEXT("/Game/sA_Megapack_v1/sA_ShootingVfxPack/FX/NiagaraSystems/NS_BulletTrail_2.NS_BulletTrail_2")));
    AssetsToLoad.Add(FSoftObjectPath(TEXT("/Game/sA_Megapack_v1/sA_ShootingVfxPack/FX/NiagaraSystems/NS_Hit2.NS_Hit2"))); // 请确认实际路径
    AssetsToLoad.Add(FSoftObjectPath(TEXT("/Game/sA_Megapack_v1/sA_ShootingVfxPack/FX/NiagaraSystems/NS_AR_Muzzleflash_1_ONCE.NS_AR_Muzzleflash_1_ONCE"))); // 请确认实际路径

    // ========== 3. 材质（如果缺失） ==========
    AssetsToLoad.Add(FSoftObjectPath(TEXT("/Game/sA_Megapack_v1/sA_Projectilevfx/Vfx/Materials/M_ABNiagara.M_ABNiagara"))); // 请确认实际路径

    // ========== 4. 音效资源 ==========
    // 根据你的截图 Sounds 文件夹内容，添加所有 Sound Cue 和衰减设置
    // 注意：路径需要根据你的项目结构调整，假设它们位于 /Game/MainFile/Sounds/ 下
    AssetsToLoad.Add(FSoftObjectPath(TEXT("/Game/MainFile/Sounds/Att_Shells.Att_Shells")));
    AssetsToLoad.Add(FSoftObjectPath(TEXT("/Game/MainFile/Sounds/Att_Shoot.Att_Shoot")));
    AssetsToLoad.Add(FSoftObjectPath(TEXT("/Game/MainFile/Sounds/Cue_Crowbar_Attack.Cue_Crowbar_Attack")));
    AssetsToLoad.Add(FSoftObjectPath(TEXT("/Game/MainFile/Sounds/Cue_Crowbar_HitDefault.Cue_Crowbar_HitDefault")));
    AssetsToLoad.Add(FSoftObjectPath(TEXT("/Game/MainFile/Sounds/Cue_Crowbar_TD_01.Cue_Crowbar_TD_01")));
    AssetsToLoad.Add(FSoftObjectPath(TEXT("/Game/MainFile/Sounds/Cue_Exp_RK.Cue_Exp_RK")));
    AssetsToLoad.Add(FSoftObjectPath(TEXT("/Game/MainFile/Sounds/Cue_Reload_Revolver.Cue_Reload_Revolver")));
    AssetsToLoad.Add(FSoftObjectPath(TEXT("/Game/MainFile/Sounds/Cue_Revolver_Shoot.Cue_Revolver_Shoot")));
    AssetsToLoad.Add(FSoftObjectPath(TEXT("/Game/MainFile/Sounds/Cue_Shell_End.Cue_Shell_End")));
    AssetsToLoad.Add(FSoftObjectPath(TEXT("/Game/MainFile/Sounds/Cue_Shell_Hit.Cue_Shell_Hit")));

    // 如果有其他音效，继续添加...

    // ========== 5. 执行异步加载 ==========
    if (AssetsToLoad.Num() > 0)
    {
        FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
        Streamable.RequestAsyncLoad(AssetsToLoad, FStreamableDelegate::CreateLambda([]()
            {
                UE_LOG(LogCoopAdventure, Log, TEXT("Force loaded all critical assets asynchronously."));
            }));
    }
}