// AGameplayFile/DemonManagerComponent.cpp
#include "DemonManagerComponent.h"
#include "PlayerStateFile/CoopAdventurePlayerState.h"
#include "IdentityComponent.h"
#include "TaskComponent.h"  // 添加任务组件头文件
#include "Net/UnrealNetwork.h"
#include "VictoryArbiter.h"
#include "Kismet/GameplayStatics.h"
#include "../CoopAdventureGameState.h"  // 或根据您的实际路径
#include "GameFramework/Character.h"        // 包含 ACharacter 定义
#include "AGameplayFile/GamePhaseManager.h" // 如果之前没包含
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"

UDemonManagerComponent::UDemonManagerComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);

    CurrentDemonPlayer = nullptr;
    PreviousDemonPlayer = nullptr;
    bGameInitialized = false;
}

void UDemonManagerComponent::BeginPlay()
{
    Super::BeginPlay();

    // 不再自动初始化，等待区域触发
    // 服务器只打印日志确认组件已就绪
    if (GetOwner()->HasAuthority())
    {
        UE_LOG(LogTemp, Log, TEXT("[DemonManager] 已就绪，等待区域触发..."));
    }
}

void UDemonManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UDemonManagerComponent, CurrentDemonPlayer);
    DOREPLIFETIME(UDemonManagerComponent, PreviousDemonPlayer);
    DOREPLIFETIME(UDemonManagerComponent, bGameInitialized);
}

// ========== 核心：区域触发接口 ==========

bool UDemonManagerComponent::TriggerGameStart(ACoopAdventurePlayerState* TriggerPlayer)
{
    // 组件中使用 GetOwnerRole() 而非 HasAuthority()
    if (GetOwnerRole() != ROLE_Authority)
    {
        UE_LOG(LogTemp, Warning, TEXT("[DemonManager] 只有服务器可以触发游戏开始"));
        return false;
    }

    // 防止重复初始化
    if (bGameInitialized)
    {
        UE_LOG(LogTemp, Log, TEXT("[DemonManager] 游戏已经初始化过，忽略重复触发"));
        return false;
    }

    // 日志记录（支持 TriggerPlayer 为 nullptr）
    if (TriggerPlayer)
    {
        UE_LOG(LogTemp, Log, TEXT("[DemonManager] 游戏开始！触发玩家: %s"),
            *TriggerPlayer->GetPlayerName());
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("[DemonManager] 游戏开始！由系统/区域触发"));
    }

    // 1. 分配首个恶魔（false = 不排除当前恶魔，因为是首次）
    bool bSuccess = AssignNewDemon(false);

    if (bSuccess)
    {
        bGameInitialized = true;

        // 2. 给所有存活玩家分配任务（整合原 IdentityAssignZone 的功能）
        InitializeTasksForAllPlayers();

        // 3. 广播游戏开始事件（所有客户端都会收到）
        OnGameInitialized.Broadcast();

        UE_LOG(LogTemp, Log, TEXT("[DemonManager] 游戏初始化完成！当前恶魔: %s"),
            CurrentDemonPlayer ? *CurrentDemonPlayer->GetPlayerName() : TEXT("Unknown"));

        return true;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[DemonManager] 初始化失败，没有可用玩家"));
        OnNoDemonAvailable.Broadcast();
        return false;
    }
}

void UDemonManagerComponent::InitializeTasksForAllPlayers()
{
    if (!GetOwner()->HasAuthority()) return;

    TArray<ACoopAdventurePlayerState*> AllPlayers = GetAllAlivePlayerStates();

    UE_LOG(LogTemp, Log, TEXT("[DemonManager] 正在为 %d 名玩家分配任务..."), AllPlayers.Num());

    for (ACoopAdventurePlayerState* PS : AllPlayers)
    {
        if (PS && PS->GetTaskComponent())
        {
            PS->GetTaskComponent()->InitializeTasks();
            UE_LOG(LogTemp, Log, TEXT("[DemonManager] 已为 %s 分配任务"), *PS->GetPlayerName());
        }
    }
}

// ========== 恶魔轮转逻辑 ==========

void UDemonManagerComponent::OnPlayerDeath(ACoopAdventurePlayerState* DeadPlayer, AActor* Killer)
{
    if (GetOwnerRole() != ROLE_Authority || !DeadPlayer) return;

    // 记录死亡（使用弱引用检查是否已存在）
    bool bAlreadyDead = false;
    for (const auto& WeakPS : DeadPlayers)
    {
        if (WeakPS.IsValid() && WeakPS.Get() == DeadPlayer)
        {
            bAlreadyDead = true;
            break;
        }
    }
    if (!bAlreadyDead)
    {
        DeadPlayers.Add(TWeakObjectPtr<APlayerState>(DeadPlayer));
    }

    // 获取 GameState 及时间管理器
    ACoopAdventureGameState* GS = Cast<ACoopAdventureGameState>(GetWorld()->GetGameState());
    UGamePhaseManager* PhaseMgr = GS ? GS->FindComponentByClass<UGamePhaseManager>() : nullptr;
    bool bIsDay = PhaseMgr ? PhaseMgr->IsDay() : true; // 默认白天

    // 判断击杀者是否是圣职
    bool bKillerIsHoly = false;
    if (Killer)
    {
        // 从 Killer AActor 获取 Character
        if (ACharacter* Char = Cast<ACharacter>(Killer))
        {
            // 从 Character 获取 PlayerState
            if (ACoopAdventurePlayerState* KillerPS = Char->GetPlayerState<ACoopAdventurePlayerState>())
            {
                // 从 PlayerState 获取身份组件
                if (UIdentityComponent* IdentityComp = KillerPS->GetIdentityComponent())
                {
                    bKillerIsHoly = (IdentityComp->GetCurrentIdentity() == EPlayerIdentityType::Holy);
                }
            }
        }
    }

    // 如果死亡的是当前恶魔
    if (DeadPlayer == CurrentDemonPlayer)
    {
        UE_LOG(LogTemp, Warning, TEXT("[DemonManager] 恶魔 %s 死亡，击杀者: %s，圣职: %d"),
            *DeadPlayer->GetPlayerName(),
            Killer ? *Killer->GetName() : TEXT("None"),
            bKillerIsHoly);

        // 圣职击杀：不轮换新恶魔
        if (!bKillerIsHoly)
        {
            if (bIsDay)
            {
                // 白天且非圣职击杀 → 延迟1秒选新恶魔
                UE_LOG(LogTemp, Log, TEXT("[DemonManager] 白天恶魔死亡，准备选择新恶魔..."));
                FTimerDelegate TimerDelegate;
                TimerDelegate.BindUObject(this, &UDemonManagerComponent::OnSelectNewDemon);
                GetWorld()->GetTimerManager().SetTimer(NewDemonTimerHandle, TimerDelegate, 1.0f, false);
            }
            else
            {
                // 夜晚恶魔死亡 → 不轮换
                UE_LOG(LogTemp, Log, TEXT("[DemonManager] 夜晚恶魔死亡，不进行轮换"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Log, TEXT("[DemonManager] 圣职击杀恶魔，不进行轮换，直接人类胜利"));
        }
    }

    // 通知胜利仲裁器检查胜利条件
    if (GS)
    {
        if (UVictoryArbiter* Arbiter = GS->FindComponentByClass<UVictoryArbiter>())
        {
            Arbiter->CheckDemonDeath(DeadPlayer);
        }
    }
}

bool UDemonManagerComponent::AssignNewDemon(bool bExcludeCurrentDemon)
{
    TArray<ACoopAdventurePlayerState*> AlivePlayers = GetAllAlivePlayerStates();

    if (AlivePlayers.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[DemonManager] 没有存活玩家可选"));
        return false;
    }

    // 排除当前恶魔（如果需要）
    if (bExcludeCurrentDemon && CurrentDemonPlayer)
    {
        AlivePlayers.Remove(CurrentDemonPlayer);
    }

    // 排除已死亡玩家
    for (auto It = AlivePlayers.CreateIterator(); It; ++It)
    {
        if (DeadPlayers.Contains(*It))
        {
            It.RemoveCurrent();
        }
    }

    if (AlivePlayers.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[DemonManager] 排除后没有可用玩家"));
        return false;
    }

    // 随机选择新恶魔
    int32 RandomIndex = FMath::RandRange(0, AlivePlayers.Num() - 1);
    ACoopAdventurePlayerState* NewDemon = AlivePlayers[RandomIndex];

    SetPlayerAsDemon(NewDemon);
    return true;
}

void UDemonManagerComponent::SetPlayerAsDemon(ACoopAdventurePlayerState* NewDemon)
{
    if (!NewDemon) return;

    PreviousDemonPlayer = CurrentDemonPlayer;
    CurrentDemonPlayer = NewDemon;

    // 设置身份
    if (UIdentityComponent* IdentityComp = NewDemon->GetIdentityComponent())
    {
        IdentityComp->AssignIdentity(EPlayerIdentityType::Demon);
    }

    // 其他玩家设为普通身份
    TArray<ACoopAdventurePlayerState*> AllPlayers = GetAllAlivePlayerStates();
    for (ACoopAdventurePlayerState* PS : AllPlayers)
    {
        if (PS && PS != NewDemon)
        {
            if (UIdentityComponent* IdentityComp = PS->GetIdentityComponent())
            {
                IdentityComp->AssignIdentity(EPlayerIdentityType::Human);
            }
        }
    }

    // 广播变更
    OnDemonChanged.Broadcast(NewDemon, PreviousDemonPlayer);

    UE_LOG(LogTemp, Log, TEXT("[DemonManager] 新恶魔已选出: %s (之前: %s)"),
        *NewDemon->GetPlayerName(),
        PreviousDemonPlayer ? *PreviousDemonPlayer->GetPlayerName() : TEXT("None"));
}

// ========== 查询接口实现 ==========

bool UDemonManagerComponent::IsPlayerDemon(ACoopAdventurePlayerState* Player) const
{
    return Player && Player == CurrentDemonPlayer;
}

TArray<ACoopAdventurePlayerState*> UDemonManagerComponent::GetNormalPlayers() const
{
    TArray<ACoopAdventurePlayerState*> Result;
    TArray<ACoopAdventurePlayerState*> AllAlive = GetAllAlivePlayerStates();

    for (ACoopAdventurePlayerState* PS : AllAlive)
    {
        if (PS && PS != CurrentDemonPlayer)
        {
            Result.Add(PS);
        }
    }
    return Result;
}

TArray<ACoopAdventurePlayerState*> UDemonManagerComponent::GetAlivePlayers() const
{
    return GetAllAlivePlayerStates();
}

TArray<ACoopAdventurePlayerState*> UDemonManagerComponent::GetAllAlivePlayerStates() const
{
    TArray<ACoopAdventurePlayerState*> Result;

    if (AGameStateBase* GS = GetWorld()->GetGameState())
    {
        for (APlayerState* PS : GS->PlayerArray)
        {
            if (ACoopAdventurePlayerState* CoopPS = Cast<ACoopAdventurePlayerState>(PS))
            {
                if (!DeadPlayers.Contains(CoopPS))
                {
                    Result.Add(CoopPS);
                }
            }
        }
    }

    return Result;
}

// ========== RepNotify ==========

void UDemonManagerComponent::OnRep_CurrentDemon()
{
    UE_LOG(LogTemp, Log, TEXT("[DemonManager] 客户端同步新恶魔"));
    OnDemonChanged.Broadcast(CurrentDemonPlayer, PreviousDemonPlayer);
}

void UDemonManagerComponent::OnRep_GameInitialized()
{
    // 客户端收到游戏开始通知
    if (bGameInitialized)
    {
        UE_LOG(LogTemp, Log, TEXT("[DemonManager] 客户端收到：游戏正式开始"));
        OnGameInitialized.Broadcast();
    }
}


// 添加 EndPlay 实现
void UDemonManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // 清理定时器，防止回调时组件已销毁
    GetWorld()->GetTimerManager().ClearTimer(NewDemonTimerHandle);

    Super::EndPlay(EndPlayReason);
}



// 新添加：定时器回调函数
void UDemonManagerComponent::OnSelectNewDemon()
{
    if (!AssignNewDemon(true))
    {
        OnNoDemonAvailable.Broadcast();
    }
}

