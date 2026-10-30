// CombatFile/HealthComponent.cpp
// CombatFile/HealthComponent.cpp
#include "HealthComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerState.h"              // 添加这一行！
#include "CoopAdventureGameState.h"
#include "CoopAdventureCharacter.h"
#include "PlayerStateFile/CoopAdventurePlayerState.h" // 添加这一行！
#include "AGameplayFile/KillRegulator.h"
#include "AGameplayFile/VictoryArbiter.h"

UHealthComponent::UHealthComponent()
{
    MaxHealth = 100.0f;
    InitialHealth = -1.0f;
    CurrentHealth = MaxHealth;
    bIsDead = false;
    bFriendlyFire = false;
    bDestroyOnDeath = false;
    LastDamageTime = 0.0f;
    bCanRespawn = false;

    SetIsReplicatedByDefault(true);
}

void UHealthComponent::BeginPlay()
{
    Super::BeginPlay();

    if (GetOwnerRole() == ROLE_Authority)
    {
        CurrentHealth = (InitialHealth > 0) ? InitialHealth : MaxHealth;
        bIsDead = false;
        bCanRespawn = false;
        BroadcastHealthChanged(0);
    }
}

void UHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UHealthComponent, CurrentHealth);
    DOREPLIFETIME(UHealthComponent, MaxHealth);
    DOREPLIFETIME(UHealthComponent, bIsDead);
    DOREPLIFETIME(UHealthComponent, LastDamageTime);
    DOREPLIFETIME(UHealthComponent, bCanRespawn);
}

void UHealthComponent::TakeDamage(const FDamageInfo& DamageInfo)
{
    if (bIsDead && DamageInfo.BaseDamage > 0)
    {
        return;
    }

    if (GetOwnerRole() == ROLE_Authority)
    {
        ApplyDamage(DamageInfo);
    }
    else
    {
        Server_TakeDamage(DamageInfo);
    }
}

void UHealthComponent::Heal(float HealAmount, AActor* Healer)
{
    if (HealAmount <= 0 || bIsDead) return;

    if (GetOwnerRole() == ROLE_Authority)
    {
        Server_Heal_Implementation(HealAmount);
    }
    else
    {
        Server_Heal(HealAmount);
    }
}

void UHealthComponent::Revive(bool bResetToMaxHP)
{
    if (!bIsDead) return;

    if (GetOwnerRole() == ROLE_Authority)
    {
        Server_Revive_Implementation(bResetToMaxHP);
    }
    else
    {
        Server_Revive(bResetToMaxHP);
    }
}

void UHealthComponent::RequestRespawn()
{
    if (!bIsDead || !bCanRespawn)
    {
        UE_LOG(LogTemp, Warning, TEXT("RequestRespawn failed: Dead=%d, CanRespawn=%d"),
            (int32)bIsDead, (int32)bCanRespawn);
        return;
    }

    if (GetOwnerRole() == ROLE_Authority)
    {
        Server_PerformRespawn_Implementation();
    }
    else
    {
        Server_PerformRespawn();
    }
}

// Server RPC实现
void UHealthComponent::Server_TakeDamage_Implementation(const FDamageInfo& DamageInfo)
{
    ApplyDamage(DamageInfo);
}

bool UHealthComponent::Server_TakeDamage_Validate(const FDamageInfo& DamageInfo)
{
    return DamageInfo.BaseDamage >= 0;
}

void UHealthComponent::Server_Heal_Implementation(float HealAmount)
{
    if (bIsDead || HealAmount <= 0) return;

    float OldHealth = CurrentHealth;
    CurrentHealth = FMath::Clamp(CurrentHealth + HealAmount, 0.0f, MaxHealth);
    float ActualHeal = CurrentHealth - OldHealth;

    if (ActualHeal > 0)
    {
        BroadcastHealthChanged(ActualHeal);
    }
}

bool UHealthComponent::Server_Heal_Validate(float HealAmount)
{
    return HealAmount > 0;
}

void UHealthComponent::Server_Revive_Implementation(bool bResetToMaxHP)
{
    if (!bIsDead) return;

    bIsDead = false;
    bCanRespawn = false;
    CurrentHealth = bResetToMaxHP ? MaxHealth : (MaxHealth * 0.5f);

    if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
    {
        Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        if (AController* Controller = Character->GetController())
        {
            Controller->SetIgnoreMoveInput(false);
        }
    }

    BroadcastHealthChanged(0);
}

bool UHealthComponent::Server_Revive_Validate(bool bResetToMaxHP)
{
    return true;
}

void UHealthComponent::Server_PerformRespawn_Implementation()
{
    if (!bCanRespawn || !bIsDead) return;

    UE_LOG(LogTemp, Log, TEXT("Server: Performing respawn for %s"), *GetOwner()->GetName());

    // 重置复活状态
    bCanRespawn = false;

    // 关闭Ragdoll并恢复输入
    if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
    {
        if (ACoopAdventureCharacter* CoopChar = Cast<ACoopAdventureCharacter>(Character))
        {
            CoopChar->Multicast_DisableRagdoll();
            CoopChar->SetInputEnabled(true);

            // 传送到复活点（延迟执行确保Ragdoll完全关闭）
            FTimerHandle RespawnHandle;
            GetWorld()->GetTimerManager().SetTimer(RespawnHandle, [CoopChar]()
                {
                    CoopChar->ResetToSpawnPoint();
                }, 0.1f, false);
        }
    }

    // 恢复满血
    Revive(true);
}

bool UHealthComponent::Server_PerformRespawn_Validate()
{
    return true;
}

// 内部逻辑
void UHealthComponent::ApplyDamage(const FDamageInfo& DamageInfo)
{
    if (bIsDead) return;

    float ActualDamage = DamageInfo.BaseDamage;
    float OldHealth = CurrentHealth;
    CurrentHealth = FMath::Clamp(CurrentHealth - ActualDamage, 0.0f, MaxHealth);
    float Delta = OldHealth - CurrentHealth;

    LastDamageTime = GetWorld()->GetTimeSeconds();

    OnTakeDamage.Broadcast(DamageInfo, ActualDamage);
    BroadcastHealthChanged(-Delta);

    if (CurrentHealth <= 0 && !bIsDead)
    {
        HandleDeath(DamageInfo.Instigator);
    }
}

void UHealthComponent::HandleDeath(AActor* Killer)
{
    if (bIsDead) return;
    bIsDead = true;

    UE_LOG(LogTemp, Log, TEXT("HealthComponent: %s has died."), *GetOwner()->GetName());

    // 停止移动
    if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
    {
        Character->GetCharacterMovement()->DisableMovement();
        if (AController* Controller = Character->GetController())
        {
            Controller->SetIgnoreMoveInput(true);
        }

        // 启用Ragdoll和禁用输入
        if (ACoopAdventureCharacter* CoopChar = Cast<ACoopAdventureCharacter>(Character))
        {
            CoopChar->Multicast_EnableRagdoll();
            //CoopChar->SetInputEnabled(false);
        }
    }

    // 广播死亡事件
    OnDeath.Broadcast(GetOwner(), Killer);

    // 立即允许复活（Server）
    if (GetOwnerRole() == ROLE_Authority)
    {
        bCanRespawn = true;
        OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, 0);
        UE_LOG(LogTemp, Log, TEXT("立即复活已启用"));
    }

    if (bDestroyOnDeath)
    {
        GetOwner()->SetLifeSpan(3.0f);
    }


    if (GetOwnerRole() == ROLE_Authority)
    {
        if (ACoopAdventureGameState* GS = Cast<ACoopAdventureGameState>(GetWorld()->GetGameState()))
        {
            ACharacter* Char = Cast<ACharacter>(GetOwner());
            if (!Char) return;

            ACoopAdventurePlayerState* PS = Cast<ACoopAdventurePlayerState>(Char->GetPlayerState());
            if (!PS) return;

            // 获取时间管理器
            UGamePhaseManager* PhaseMgr = GS->FindComponentByClass<UGamePhaseManager>();
            UDemonManagerComponent* DemonMgr = GS->GetDemonManager();
            UVictoryArbiter* Arbiter = GS->FindComponentByClass<UVictoryArbiter>();

            // KillRegulator（原有）
            if (UKillRegulator* Regulator = GS->FindComponentByClass<UKillRegulator>())
            {
                Regulator->ProcessKill(GetOwner(), Killer);
            }

            // 关键逻辑：判断死者身份和时间
            if (DemonMgr && DemonMgr->IsPlayerDemon(PS))
            {
                // 恶魔死亡
                if (PhaseMgr && PhaseMgr->IsDay())
                {
                    // 白天：通知 DemonManager 进行轮换（延迟1秒后选新恶魔）
                    DemonMgr->OnPlayerDeath(PS, Killer);  // 传入击杀者
                    UE_LOG(LogTemp, Log, TEXT("[Death] Day demon death - will assign new demon"));
                }
                else
                {
                    // 夜晚：不轮换，直接人类胜利
                    UE_LOG(LogTemp, Warning, TEXT("[Death] Night demon death - no rotation, humans win"));
                    // 但也要通知 DemonManager 记录死亡，并可能让胜利仲裁器检查
                    DemonMgr->OnPlayerDeath(PS, Killer);
                }
            }

            // 统一检查胜利条件（延迟一帧，确保轮换逻辑先完成）
            if (Arbiter)
            {
                FTimerHandle TempHandle;
                GetWorld()->GetTimerManager().SetTimerForNextTick([Arbiter]()
                    {
                        Arbiter->CheckVictoryConditions();
                    });
            }
        }
    }

}

void UHealthComponent::BroadcastHealthChanged(float Delta)
{
    OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, Delta);
}

// RepNotify
void UHealthComponent::OnRep_CurrentHealth()
{
    BroadcastHealthChanged(0);
}

void UHealthComponent::OnRep_IsDead()
{
    if (bIsDead)
    {
        UE_LOG(LogTemp, Log, TEXT("Client: Death state replicated"));
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("Client: Revive state replicated"));
    }
}

void UHealthComponent::OnRep_CanRespawn()
{
    UE_LOG(LogTemp, Verbose, TEXT("Client: bCanRespawn updated to %d"), (int32)bCanRespawn);
    if (bCanRespawn && bIsDead)
    {
        OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, 0);
    }
}