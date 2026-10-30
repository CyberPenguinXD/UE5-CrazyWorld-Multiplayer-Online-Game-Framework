// AGameplayFile/GamePhaseManager.cpp
#include "GamePhaseManager.h"
#include "Net/UnrealNetwork.h"

UGamePhaseManager::UGamePhaseManager()
{
    PrimaryComponentTick.bCanEverTick = false; // 不需要Tick，被动接收时间更新
    SetIsReplicatedByDefault(true);

    CurrentPhase = EDayPhase::Waiting;
    CurrentDayCount = 1;
    DailyKillCount = 0;
    bSettlementTriggeredToday = false;
    LastRecordedHour = -1;

    NightHumanKillCount = 0;
    NightDemonKillCount = 0;
}

void UGamePhaseManager::BeginPlay()
{
    Super::BeginPlay();

    // 只在服务器初始化默认值
    if (GetOwnerRole() == ROLE_Authority)
    {
        CurrentPhase = EDayPhase::Waiting;
        CurrentDayCount = 1;
        DailyKillCount = 0;
        UE_LOG(LogTemp, Log, TEXT("[GamePhaseManager] 初始化完成，等待游戏开始..."));
    }
}

void UGamePhaseManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UGamePhaseManager, CurrentPhase);
    DOREPLIFETIME(UGamePhaseManager, CurrentDayCount);
    DOREPLIFETIME(UGamePhaseManager, DailyKillCount);
    DOREPLIFETIME(UGamePhaseManager, bSettlementTriggeredToday);
    
    DOREPLIFETIME(UGamePhaseManager, NightHumanKillCount);
    DOREPLIFETIME(UGamePhaseManager, NightDemonKillCount);
}

void UGamePhaseManager::UpdatePhaseFromTime(int32 CurrentHour, int32 CurrentMinute)
{
    // 仅在服务器执行逻辑
    if (GetOwnerRole() != ROLE_Authority) return;

    // 检测时间跳跃（防止作弊或调试）
    if (LastRecordedHour != -1 && FMath::Abs(CurrentHour - LastRecordedHour) > 2)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GamePhaseManager] 检测到时间大幅跳跃: %d -> %d"), LastRecordedHour, CurrentHour);
    }
    LastRecordedHour = CurrentHour;

    EDayPhase NewPhase = CurrentPhase;

    // 核心时间逻辑（06:00-18:00为白天，其余为夜晚）
    if (CurrentHour >= 6 && CurrentHour < 18)
    {
        NewPhase = EDayPhase::Day;

        // 进入白天的瞬间（06:00）
        if (CurrentPhase != EDayPhase::Day && CurrentHour == 6)
        {
            // 新的一天开始
            if (CurrentPhase == EDayPhase::Night || CurrentPhase == EDayPhase::Waiting)
            {
                CurrentDayCount++;
                OnRep_DayCount(); // 手动触发，服务器也需要广播

                // 重置每日数据
                ResetDailyKillCount();
                bSettlementTriggeredToday = false;

                OnDailyStart.Broadcast();
                UE_LOG(LogTemp, Log, TEXT("[GamePhaseManager] 第 %d 天开始 (06:00)"), CurrentDayCount);
            }
        }
    }
    else
    {
        NewPhase = EDayPhase::Night;

        // 进入夜晚的瞬间（18:00）
        if (CurrentPhase == EDayPhase::Day && CurrentHour == 18)
        {
            UE_LOG(LogTemp, Log, TEXT("[GamePhaseManager] 第 %d 天夜晚开始 (18:00)"), CurrentDayCount);
            CheckAndTriggerSettlement();
        }
    }

    // 应用阶段变化
    if (NewPhase != CurrentPhase)
    {
        SetGamePhase(NewPhase);
    }
}

void UGamePhaseManager::SetGamePhase(EDayPhase NewPhase)
{
    if (CurrentPhase == NewPhase) return;

    EDayPhase OldPhase = CurrentPhase;
    CurrentPhase = NewPhase;

    UE_LOG(LogTemp, Log, TEXT("[GamePhaseManager] 阶段切换: %s -> %s"),
        *UEnum::GetValueAsString(OldPhase),
        *UEnum::GetValueAsString(NewPhase));

    // 广播事件（服务器和客户端都会通过RepNotify或此处执行）
    OnPhaseChanged.Broadcast(NewPhase, OldPhase);

    // 服务器特定逻辑
    if (GetOwnerRole() == ROLE_Authority)
    {
        // 进入白天：重置击杀计数（保险起见，双重保障）
        if (NewPhase == EDayPhase::Day)
        {
            ResetDailyKillCount();
        }
    }

    if (GetOwnerRole() == ROLE_Authority)
    {
        if (NewPhase == EDayPhase::Day)
        {
            ResetDailyKillCount();
        }
        else if (NewPhase == EDayPhase::Night)
        {
            ResetNightKillCounts();
        }
    }
}

void UGamePhaseManager::ForceSetPhase(EDayPhase NewPhase)
{
    if (GetOwnerRole() != ROLE_Authority) return;
    SetGamePhase(NewPhase);
}

void UGamePhaseManager::ResetDailyKillCount()
{
    DailyKillCount = 0;
    bSettlementTriggeredToday = false;
    UE_LOG(LogTemp, Log, TEXT("[GamePhaseManager] 每日击杀计数已重置"));
}

void UGamePhaseManager::IncrementDailyKillCount()
{
    if (GetOwnerRole() != ROLE_Authority) return;

    DailyKillCount++;
    UE_LOG(LogTemp, Log, TEXT("[GamePhaseManager] 恶魔击杀计数: %d"), DailyKillCount);

    // 如果白天击杀超过1人，可以在这里触发反杀逻辑，或者让DemonManager查询后处理
    if (IsDay() && DailyKillCount > 1)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GamePhaseManager] 警告：恶魔白天击杀超过1人！"));
    }
}

void UGamePhaseManager::CheckAndTriggerSettlement()
{
    if (!bSettlementTriggeredToday)
    {
        bSettlementTriggeredToday = true;
        OnDailySettlement.Broadcast();
        UE_LOG(LogTemp, Log, TEXT("[GamePhaseManager] 触发每日任务结算"));
    }
}

// ===== RepNotify实现 =====
void UGamePhaseManager::OnRep_CurrentPhase(EDayPhase OldPhase)
{
    // 客户端收到阶段变化
    UE_LOG(LogTemp, Log, TEXT("[GamePhaseManager] 客户端收到阶段同步: %s -> %s"),
        *UEnum::GetValueAsString(OldPhase),
        *UEnum::GetValueAsString(CurrentPhase));

    OnPhaseChanged.Broadcast(CurrentPhase, OldPhase);
}

void UGamePhaseManager::OnRep_DayCount()
{
    OnDayCountChanged.Broadcast(CurrentDayCount);
}

void UGamePhaseManager::ResetNightKillCounts()
{
    NightHumanKillCount = 0;
    NightDemonKillCount = 0;
    UE_LOG(LogTemp, Log, TEXT("Reset night kill counts"));
}

void UGamePhaseManager::IncrementNightHumanKillCount()
{
    if (GetOwnerRole() != ROLE_Authority) return;
    NightHumanKillCount++;
    UE_LOG(LogTemp, Log, TEXT("Night human kill count: %d"), NightHumanKillCount);
}

void UGamePhaseManager::IncrementNightDemonKillCount()
{
    if (GetOwnerRole() != ROLE_Authority) return;
    NightDemonKillCount++;
    UE_LOG(LogTemp, Log, TEXT("Night demon kill count: %d"), NightDemonKillCount);
}