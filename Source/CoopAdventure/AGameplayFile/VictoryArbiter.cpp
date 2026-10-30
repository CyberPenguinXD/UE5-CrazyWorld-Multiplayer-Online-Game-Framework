#include "VictoryArbiter.h"
#include "Net/UnrealNetwork.h"
#include "DemonManagerComponent.h"
#include "PlayerStateFile/CoopAdventurePlayerState.h"
#include "../CoopAdventureGameState.h"
#include "../CoopAdventureCharacter.h"
#include "GameFramework/GameStateBase.h"

UVictoryArbiter::UVictoryArbiter()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
    CurrentVictoryState = EVictoryType::None;
}

void UVictoryArbiter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UVictoryArbiter, CurrentVictoryState);
}

void UVictoryArbiter::CheckDemonDeath(ACoopAdventurePlayerState* DeadDemon)
{
    if (GetOwnerRole() != ROLE_Authority || IsGameEnded()) return;

    int32 AliveDemons = GetAliveDemonCount();
    int32 AliveHumans = GetAliveHumanCount(); // 关键：同时检查人类数量

    UE_LOG(LogTemp, Log, TEXT("[VictoryArbiter] Demon died - AliveDemons: %d, AliveHumans: %d"),
        AliveDemons, AliveHumans);

    if (AliveDemons == 0)
    {
        if (AliveHumans > 0)
        {
            // 恶魔死光，但还有人类活着 → 人类胜利
            TriggerVictory(EVictoryType::HumanVictory);
        }
        else
        {
            // 恶魔死光，人类也死光了（同归于尽）→ 恶魔胜利（全死则恶魔赢）
            UE_LOG(LogTemp, Warning, TEXT("[VictoryArbiter] All players dead - Demon wins!"));
            TriggerVictory(EVictoryType::DemonVictory);
        }
    }
}

void UVictoryArbiter::CheckHumanDeath(ACoopAdventurePlayerState* DeadHuman)
{
    if (GetOwnerRole() != ROLE_Authority || IsGameEnded()) return;

    int32 AliveHumans = GetAliveHumanCount();

    UE_LOG(LogTemp, Log, TEXT("[VictoryArbiter] Human died - AliveHumans: %d"), AliveHumans);

    if (AliveHumans == 0)
    {
        // 人类死光（不管恶魔是否还活着）→ 恶魔胜利
        UE_LOG(LogTemp, Warning, TEXT("[VictoryArbiter] All humans dead - Demon wins!"));
        TriggerVictory(EVictoryType::DemonVictory);
    }
}

void UVictoryArbiter::TriggerVictory(EVictoryType VictoryType)
{
    if (CurrentVictoryState != EVictoryType::None) return;

    CurrentVictoryState = VictoryType;
    OnRep_VictoryState();

    UE_LOG(LogTemp, Log, TEXT("[VictoryArbiter] GAME OVER - %s"),
        *UEnum::GetValueAsString(VictoryType));

    OnVictoryDetected.Broadcast(VictoryType);

}

int32 UVictoryArbiter::GetAliveHumanCount() const
{
    int32 Count = 0;
    AGameStateBase* GS = GetWorld()->GetGameState();
    if (!GS) return 0;

    UDemonManagerComponent* DemonMgr = nullptr;
    if (ACoopAdventureGameState* CoopGS = Cast<ACoopAdventureGameState>(GS))
    {
        DemonMgr = CoopGS->GetDemonManager();
    }

    for (APlayerState* PS : GS->PlayerArray)
    {
        ACoopAdventurePlayerState* CoopPS = Cast<ACoopAdventurePlayerState>(PS);
        if (!CoopPS) continue;
        if (DemonMgr && DemonMgr->IsPlayerDemon(CoopPS)) continue;

        ACoopAdventureCharacter* Char = Cast<ACoopAdventureCharacter>(CoopPS->GetPawn());
        if (Char && Char->GetHealthComponent() && !Char->GetHealthComponent()->IsDead())
        {
            Count++;
        }
    }
    return Count;
}

int32 UVictoryArbiter::GetAliveDemonCount() const
{
    int32 Count = 0;
    AGameStateBase* GS = GetWorld()->GetGameState();
    if (!GS) return 0;

    UDemonManagerComponent* DemonMgr = nullptr;
    if (ACoopAdventureGameState* CoopGS = Cast<ACoopAdventureGameState>(GS))
    {
        DemonMgr = CoopGS->GetDemonManager();
    }
    if (!DemonMgr) return 0;

    for (APlayerState* PS : GS->PlayerArray)
    {
        ACoopAdventurePlayerState* CoopPS = Cast<ACoopAdventurePlayerState>(PS);
        if (!CoopPS) continue;

        if (DemonMgr->IsPlayerDemon(CoopPS))
        {
            ACoopAdventureCharacter* Char = Cast<ACoopAdventureCharacter>(CoopPS->GetPawn());
            if (Char && Char->GetHealthComponent() && !Char->GetHealthComponent()->IsDead())
            {
                Count++;
            }
        }
    }
    return Count;
}

void UVictoryArbiter::OnRep_VictoryState()
{
    UE_LOG(LogTemp, Log, TEXT("[VictoryArbiter] Client victory state: %s"),
        *UEnum::GetValueAsString(CurrentVictoryState));
    // 关键修复：客户端收到复制后也广播事件
    if (GetOwnerRole() != ROLE_Authority && CurrentVictoryState != EVictoryType::None)
    {
        OnVictoryDetected.Broadcast(CurrentVictoryState);
    }
}

void UVictoryArbiter::CheckVictoryConditions()
{
    if (GetOwnerRole() != ROLE_Authority || IsGameEnded()) return;

    int32 AliveHumans = GetAliveHumanCount();
    int32 AliveDemons = GetAliveDemonCount();

    UE_LOG(LogTemp, Log, TEXT("[Victory] Check - Humans: %d, Demons: %d"), AliveHumans, AliveDemons);

    // 优先判断：人类死光 = 恶魔胜利（无论恶魔是否还活着）
    if (AliveHumans == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Victory] All humans dead -> Demon Victory"));
        TriggerVictory(EVictoryType::DemonVictory);
        return;
    }

    // 恶魔死光且人类还活着 = 人类胜利
    if (AliveDemons == 0 && AliveHumans > 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Victory] All demons dead -> Human Victory"));
        TriggerVictory(EVictoryType::HumanVictory);
        return;
    }
}

