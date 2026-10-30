// CoopAdventureGameState.cpp
#include "CoopAdventureGameState.h"

ACoopAdventureGameState::ACoopAdventureGameState()
{
    // 只在非CDO（非默认对象）时创建组件，避免编辑器报错
    if (!HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
    {
        // 创建恶魔管理组件，命名为 "DemonManager"
        DemonManager = CreateDefaultSubobject<UDemonManagerComponent>(TEXT("DemonManager"));

        PhaseManager = CreateDefaultSubobject<UGamePhaseManager>(TEXT("PhaseManager")); // 添加这行

        // 新增：创建击杀监管器
        KillRegulator = CreateDefaultSubobject<UKillRegulator>(TEXT("KillRegulator"));

        VictoryArbiter = CreateDefaultSubobject<UVictoryArbiter>(TEXT("VictoryArbiter"));

        DailyTaskPenalty = CreateDefaultSubobject<UDailyTaskPenaltyComponent>(TEXT("DailyTaskPenalty"));

        DailyTaskRefresh = CreateDefaultSubobject<UDailyTaskRefreshComponent>(TEXT("DailyTaskRefresh"));
    }
}

void ACoopAdventureGameState::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Log, TEXT("=== [GameState] BeginPlay 被调用 ==="));

    // 服务器端初始化恶魔管理器
    if (HasAuthority() && DemonManager)
    {
        UE_LOG(LogTemp, Log, TEXT("=== [GameState] 服务器初始化，准备分配恶魔 ==="));
        // 延迟初始化，等待所有PlayerState进入
        // DemonManager->InitializeGame(); // 已经在DemonManager::BeginPlay中处理
    }
}