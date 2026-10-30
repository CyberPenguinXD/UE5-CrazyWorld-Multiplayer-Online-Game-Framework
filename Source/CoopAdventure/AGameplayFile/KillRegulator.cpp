// AGameplayFile/KillRegulator.cpp
#include "KillRegulator.h"
#include "GamePhaseManager.h"
#include "DemonManagerComponent.h"
#include "GameFramework/Character.h"  // 添加这一行！
#include "../CoopAdventureGameState.h"
#include "../PlayerStateFile/CoopAdventurePlayerState.h"
#include "../GameAbilityFile/HealthComponent.h"

UKillRegulator::UKillRegulator()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UKillRegulator::BeginPlay()
{
    Super::BeginPlay();
    UE_LOG(LogTemp, Log, TEXT("[KillRegulator] 初始化完成"));
}

void UKillRegulator::ProcessKill(AActor* Victim, AActor* Killer)
{
    // 基础检查：无效击杀、自杀、环境伤害不处理
    if (!Killer || !Victim || Killer == Victim) return;


    // 圣职击杀无限制，直接返回
    if (IsPlayerHoly(Killer))
    {
        UE_LOG(LogTemp, Log, TEXT("[KillRegulator] 圣职击杀，无惩罚"));
        return;
    }

    // 获取必要组件
    ACoopAdventureGameState* GameState = Cast<ACoopAdventureGameState>(GetWorld()->GetGameState());
    if (!GameState) return;

    UGamePhaseManager* PhaseMgr = GameState->FindComponentByClass<UGamePhaseManager>();
    UDemonManagerComponent* DemonMgr = GameState->FindComponentByClass<UDemonManagerComponent>();
    if (!PhaseMgr || !DemonMgr) return;

  
    bool bIsNight = PhaseMgr->IsNight();
    // ===== 夜晚监管逻辑 =====
    if (PhaseMgr->IsNight())
    {
        bool bKillerIsDemon = IsPlayerDemon(Killer);
        if (bKillerIsDemon)
        {
            if (PhaseMgr->CanDemonKillAtNight())
            {
                PhaseMgr->IncrementNightDemonKillCount();
                // 允许击杀，不惩罚
                return;
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("[KillRegulator] 恶魔夜晚击杀超过1人，处以死刑！"));
                PunishPlayer(Killer);
                return;
            }
        }
        else
        {
            if (PhaseMgr->CanHumanKillAtNight())
            {
                PhaseMgr->IncrementNightHumanKillCount();
                return;
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("[KillRegulator] 人类夜晚击杀超过1人，处以死刑！"));
                PunishPlayer(Killer);
                return;
            }
        }
    }



    // ===== 白天监管逻辑 =====

    // 获取凶手身份
    bool bKillerIsDemon = IsPlayerDemon(Killer);

    if (!bKillerIsDemon)
    {
        // 人类白天杀人 → 立即处死（偿命）
        UE_LOG(LogTemp, Warning, TEXT("[KillRegulator] 人类 %s 在白天击杀 %s，处以死刑！"),
            *Killer->GetName(), *Victim->GetName());
        PunishPlayer(Killer);
    }
    else
    {
        // 恶魔白天杀人 → 检查次数
        int32 KillCount = PhaseMgr->GetDailyKillCount();

        if (KillCount >= 1)
        {
            // 第2次或更多击杀 → 处死恶魔
            UE_LOG(LogTemp, Warning, TEXT("[KillRegulator] 恶魔 %s 白天击杀超过1人（当前%d人），处以死刑！"),
                *Killer->GetName(), KillCount + 1);
            PunishPlayer(Killer);
        }
        else
        {
            // 第1次击杀 → 记录次数，免死
            PhaseMgr->IncrementDailyKillCount();
            UE_LOG(LogTemp, Log, TEXT("[KillRegulator] 恶魔 %s 白天首次击杀（1/1），记录次数"), *Killer->GetName());
        }
    }
}

void UKillRegulator::PunishPlayer(AActor* Player)
{
    if (!Player) return;

    // 调用 HealthComponent 自杀（即死伤害）
    UHealthComponent* HealthComp = Player->FindComponentByClass<UHealthComponent>();
    if (HealthComp)
    {
        FDamageInfo SuicideDamage;
        SuicideDamage.BaseDamage = 99999.f;  // 确保致死
        SuicideDamage.Instigator = Player;    // 自己杀自己
        SuicideDamage.DamageType = FGameplayTag::RequestGameplayTag(FName("Damage.Type.DivinePunishment"), false);

        HealthComp->TakeDamage(SuicideDamage);

        UE_LOG(LogTemp, Log, TEXT("[KillRegulator] %s 已被处决"), *Player->GetName());
    }
}

bool UKillRegulator::IsPlayerDemon(AActor* Player) const
{
    ACharacter* Char = Cast<ACharacter>(Player);
    if (!Char) return false;

    ACoopAdventurePlayerState* PS = Cast<ACoopAdventurePlayerState>(Char->GetPlayerState());
    if (!PS) return false;

    ACoopAdventureGameState* GameState = Cast<ACoopAdventureGameState>(GetWorld()->GetGameState());
    if (!GameState) return false;

    UDemonManagerComponent* DemonMgr = GameState->FindComponentByClass<UDemonManagerComponent>();
    if (!DemonMgr) return false;

    return DemonMgr->IsPlayerDemon(PS);
}

bool UKillRegulator::IsPlayerHoly(AActor* Player) const
{
    ACharacter* Char = Cast<ACharacter>(Player);
    if (!Char) return false;

    ACoopAdventurePlayerState* PS = Cast<ACoopAdventurePlayerState>(Char->GetPlayerState());
    if (!PS) return false;

    UIdentityComponent* IdentityComp = PS->GetIdentityComponent();
    if (!IdentityComp) return false;

    return IdentityComp->GetCurrentIdentity() == EPlayerIdentityType::Holy;
}