// CoopAdventureCharacter_Appearance.cpp - 跨关卡外观同步
// 负责处理角色模型(SkeletalMesh)和动画蓝图(AnimBlueprint)在关卡切换后的恢复
#include "CoopAdventureCharacter.h"
#include "GameFramework/PlayerState.h"
#include "Engine/GameInstance.h"
#include "CoopAdventure.h"

/**
 * 保存当前外观到GameInstance
 * 在Teleport前调用，确保关卡切换后能恢复模型
 * GameInstance生命周期跨越所有关卡，是唯一可靠的跨关卡存储位置
 */
void ACoopAdventureCharacter::SaveAppearanceToGameInstance()
{
    if (!SubordinateMesh) return;

    USkeletalMesh* CurrentMesh = SubordinateMesh->GetSkeletalMeshAsset();
    if (!CurrentMesh) return;

    APlayerState* PS = GetPlayerState();
    if (!PS) return;

    FString PlayerKey = PS->GetPlayerName();
    if (PlayerKey.IsEmpty()) PlayerKey = GetName();

    if (UCoopAdventureGameInstance* GI = Cast<UCoopAdventureGameInstance>(GetGameInstance()))
    {
        GI->PlayerSavedMeshes.Add(PlayerKey, CurrentMesh);
        GI->PlayerSavedAnims.Add(PlayerKey, SubordinateMesh->GetAnimClass());

        // 保存当前昵称（如果有变化）
        GI->PlayerSavedNicknames.Add(PlayerKey, PS->GetPlayerName());

        UE_LOG(LogTemp, Log, TEXT("外观已保存到GameInstance - 玩家: %s, 模型: %s, 昵称: %s"),
            *PlayerKey, *CurrentMesh->GetName(), *PS->GetPlayerName());
    }
}

/**
 * 从GameInstance加载外观并应用
 * 在新关卡BeginPlay后延迟调用（确保PlayerState已同步）
 *
 * 注意权限区分：
 * - Server: 从GameInstance读取 -> 设置Replicated变量 -> 自动同步到客户端
 * - Client: 等待Server的Replicated变量同步后，通过OnRep回调应用
 */
void ACoopAdventureCharacter::LoadAppearanceFromGameInstance()
{
    // 防止重复加载（每个角色只恢复一次）
    if (bAppearanceRestored || !SubordinateMesh) return;

    // 服务器端逻辑：从GameInstance读取并设置复制变量
    if (HasAuthority())
    {
        APlayerState* PS = GetPlayerState();
        if (!PS) return;

        FString PlayerKey = PS->GetPlayerName();
        if (PlayerKey.IsEmpty()) PlayerKey = GetName();

        if (UCoopAdventureGameInstance* GI = Cast<UCoopAdventureGameInstance>(GetGameInstance()))
        {
            // 恢复昵称
            if (GI->PlayerSavedNicknames.Contains(PlayerKey))
            {
                FString SavedNickname = GI->PlayerSavedNicknames[PlayerKey];
                if (!SavedNickname.IsEmpty() && SavedNickname != PS->GetPlayerName())
                {
                    PS->SetPlayerName(SavedNickname);
                    UE_LOG(LogTemp, Log, TEXT("Server恢复昵称 - 玩家: %s -> %s"), *PlayerKey, *SavedNickname);
                }
            }

            // 恢复模型和动画（原有代码）
            if (GI->PlayerSavedMeshes.Contains(PlayerKey))
            {
                USkeletalMesh* SavedMesh = GI->PlayerSavedMeshes[PlayerKey];
                TSubclassOf<UAnimInstance> SavedAnim = GI->PlayerSavedAnims.Contains(PlayerKey) ? GI->PlayerSavedAnims[PlayerKey] : nullptr;

                if (SavedMesh)
                {
                    ReplicatedMesh = SavedMesh;
                    ReplicatedAnim = SavedAnim;
                    OnRep_AppearanceData();
                    bAppearanceRestored = true;
                    UE_LOG(LogTemp, Log, TEXT("Server恢复外观 - 玩家: %s"), *PlayerKey);
                }
            }
        }
    }
    // 客户端逻辑：等待Replicated变量同步后应用（原有代码不变）
    else
    {
        if (ReplicatedMesh)
        {
            OnRep_AppearanceData();
            bAppearanceRestored = true;
            UE_LOG(LogTemp, Log, TEXT("Client通过RepNotify恢复外观"));
        }
    }
}

/**
 * RepNotify回调 - 当ReplicatedMesh/ReplicatedAnim变化时自动触发
 * 在客户端和Server都会执行（Server在LoadAppearanceFromGameInstance中手动调用）
 *
 * 实际执行模型替换和动画类设置
 */
void ACoopAdventureCharacter::OnRep_AppearanceData()
{
    if (SubordinateMesh && ReplicatedMesh)
    {
        // 应用骨骼网格体（模型）
        SubordinateMesh->SetSkeletalMeshAsset(ReplicatedMesh);

        // 应用动画蓝图类（如果有的话）
        if (ReplicatedAnim)
        {
            SubordinateMesh->SetAnimInstanceClass(ReplicatedAnim);
        }

        bAppearanceRestored = true;

        UE_LOG(LogTemp, Log, TEXT("外观已应用 - 模型: %s"), *ReplicatedMesh->GetName());
    }
}

