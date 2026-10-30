#include "TaskComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "CoopAdventure.h"
#include "AGameplayFile/IdentityComponent.h"
#include "Algo/RandomShuffle.h"  // 用于打乱数组（可选）
#include "PlayerStateFile/CoopAdventurePlayerState.h"

UTaskComponent::UTaskComponent()
{
    SetIsReplicatedByDefault(true);
}

void UTaskComponent::BeginPlay()
{
    Super::BeginPlay();

    if (AActor* Owner = GetOwner())
    {
        Owner->SetNetUpdateFrequency(100.0f);
        Owner->SetMinNetUpdateFrequency(60.0f);
    }
}

void UTaskComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UTaskComponent, TaskList);
}

void UTaskComponent::InitializeTasks()
{
    if (GetOwnerRole() != ROLE_Authority) return;

    ACoopAdventurePlayerState* PS = Cast<ACoopAdventurePlayerState>(GetOwner());
    if (!PS) return;

    UIdentityComponent* IdentityComp = PS->GetIdentityComponent();
    bool bIsImpostor = IdentityComp && IdentityComp->IsImpostor();

    // 清空现有任务
    TaskList.Empty();

    UDataTable* TaskTable = LoadTaskDataTable();
    if (!TaskTable) return;

    TArray<FName> RowNames = TaskTable->GetRowNames();
    static const FString Context(TEXT("TaskInit"));

    // 收集主线任务ID
    TArray<FName> MajorTaskIDs;
    for (const FName& RowName : RowNames)
    {
        FTaskStaticData* Data = TaskTable->FindRow<FTaskStaticData>(RowName, Context);
        if (Data && Data->TaskType == ETaskType::Major)
        {
            MajorTaskIDs.Add(RowName);
        }
    }

    // 【关键修复】使用强唯一种子：PlayerId + 名称哈希 + 当前帧时间戳
    // 确保即使同一帧内多个玩家初始化，种子也不同
    uint32 NameHash = GetTypeHash(PS->GetPlayerName());
    uint32 TimeFactor = FPlatformTime::Cycles(); // 当前CPU周期计数，几乎每次调用都不同
    uint32 Seed = PS->GetPlayerId() * 100000 + NameHash + (TimeFactor % 10000);

    FRandomStream RandomStream;
    RandomStream.Initialize(Seed);

    UE_LOG(LogTemp, Log, TEXT("[TaskComponent] 玩家 %s 正在初始化任务... PlayerId:%d, NameHash:%u, Seed:%u"),
        *GetOwner()->GetName(), PS->GetPlayerId(), NameHash, Seed);

    // ===== 分配主线任务（5个，随机不重复）=====
    if (MajorTaskIDs.Num() > 0)
    {
        TArray<FName> TempMajorIDs = MajorTaskIDs;

        // Fisher-Yates 洗牌
        for (int32 i = TempMajorIDs.Num() - 1; i > 0; i--)
        {
            int32 RandIndex = RandomStream.RandRange(0, i);
            TempMajorIDs.Swap(i, RandIndex);
        }

        int32 NumToAssign = FMath::Min(5, TempMajorIDs.Num());
        for (int32 i = 0; i < NumToAssign; i++)
        {
            FName TaskID = TempMajorIDs[i];
            FTaskStaticData* Data = TaskTable->FindRow<FTaskStaticData>(TaskID, Context);
            if (!Data) continue;

            FTaskInstance Instance;
            Instance.TaskID = TaskID;
            Instance.TargetProgress = Data->CompletionValue;
            Instance.CurrentProgress = 0;
            Instance.bCompleted = false;
            Instance.bIsDaily = false;
            Instance.bIsFake = bIsImpostor;
            TaskList.Add(Instance);
        }
    }

    // ===== 分配每日任务（固定3个）=====
    TArray<FName> FixedDailyTasks = { FName("Eat"), FName("Wash"), FName("Sleep") };
    for (FName TaskID : FixedDailyTasks)
    {
        FTaskStaticData* Data = TaskTable->FindRow<FTaskStaticData>(TaskID, Context);
        if (!Data) continue;

        FTaskInstance Instance;
        Instance.TaskID = TaskID;
        Instance.TargetProgress = Data->CompletionValue;
        Instance.CurrentProgress = 0;
        Instance.bCompleted = false;
        Instance.bIsDaily = true;
        Instance.bIsFake = false;
        TaskList.Add(Instance);
    }

    // 输出最终任务列表
    FString TaskOrderStr;
    for (int32 i = 0; i < FMath::Min(3, TaskList.Num()); i++)
    {
        TaskOrderStr += TaskList[i].TaskID.ToString() + TEXT(", ");
    }

    UE_LOG(LogTemp, Log, TEXT("[TaskComponent] 玩家 %s (ID:%d) 任务初始化完成，身份:%s，种子:%u，前3个任务:%s"),
        *GetOwner()->GetName(),
        PS->GetPlayerId(),
        bIsImpostor ? TEXT("内鬼") : TEXT("好人"),
        Seed,
        *TaskOrderStr);

    OnRep_TaskList();
}

// TaskComponent.cpp
void UTaskComponent::TryCompleteTask(FName TaskID)
{
    if (GetOwnerRole() != ROLE_Authority) return;

    int32 Index = FindTaskIndex(TaskID);
    if (Index == INDEX_NONE) return;

    FTaskInstance& Task = TaskList[Index];
    if (Task.bCompleted) return;

    // 进度增加
    Task.CurrentProgress++;
    UE_LOG(LogTemp, Log, TEXT("[TaskComponent] 玩家 %s 完成任务 %s 进度 %d/%d (假任务: %d)"),
        *GetOwner()->GetName(), *TaskID.ToString(), Task.CurrentProgress, Task.TargetProgress, Task.bIsFake ? 1 : 0);

    // 检查是否达到目标
    bool bNowCompleted = (Task.CurrentProgress >= Task.TargetProgress);
    if (bNowCompleted)
    {
        Task.bCompleted = true;
        UE_LOG(LogTemp, Log, TEXT("[TaskComponent] 玩家 %s 任务 %s 已完成！"), *GetOwner()->GetName(), *TaskID.ToString());

        // 主线任务且非假任务，记录完成
        if (!Task.bIsDaily && !Task.bIsFake)
        {
            CompletedMajorTaskCount++;
            OnMajorTaskCompleted.Broadcast(TaskID);

            // 检查是否完成所有5个主线任务
            if (CompletedMajorTaskCount >= 5)
            {
                // 获取身份组件并变身圣职
                ACoopAdventurePlayerState* PS = Cast<ACoopAdventurePlayerState>(GetOwner());
                if (PS)
                {
                    UIdentityComponent* IdentityComp = PS->GetIdentityComponent();
                    if (IdentityComp && IdentityComp->GetCurrentIdentity() == EPlayerIdentityType::Human)
                    {
                        IdentityComp->AssignIdentity(EPlayerIdentityType::Holy);
                        UE_LOG(LogTemp, Log, TEXT("玩家 %s 完成所有主线任务，变身圣职！"), *PS->GetPlayerName());
                    }
                }
            }
        }
    }

    // 广播任务更新（UI刷新）
    OnRep_TaskList();
}


void UTaskComponent::Server_CompleteTask_Implementation(FName TaskID)
{
    TryCompleteTask(TaskID);
}

bool UTaskComponent::GetTaskStaticData(FName TaskID, FTaskStaticData& OutData) const
{
    UDataTable* TaskTable = LoadTaskDataTable();
    if (!TaskTable) return false;
    FTaskStaticData* Data = TaskTable->FindRow<FTaskStaticData>(TaskID, TEXT("GetTaskData"));
    if (Data)
    {
        OutData = *Data;
        return true;
    }
    return false;
}

void UTaskComponent::OnRep_TaskList()
{
    // 如果是权威端（服务器），不打印客户端日志（服务器端已有完整日志），仅刷新UI
    if (GetOwnerRole() == ROLE_Authority)
    {
        OnTaskUpdated.Broadcast();
        return;
    }

    // 以下仅在客户端执行
    FString PlayerName = TEXT("未知");
    if (ACoopAdventurePlayerState* PS = Cast<ACoopAdventurePlayerState>(GetOwner()))
    {
        PlayerName = PS->GetPlayerName();
    }

    UE_LOG(LogTemp, Log, TEXT("[TaskComponent] 客户端：玩家 %s 任务列表已更新，当前任务数：%d"),
        *PlayerName, TaskList.Num());

    for (const FTaskInstance& TaskInst : TaskList)
    {
        FTaskStaticData Data;
        if (GetTaskStaticData(TaskInst.TaskID, Data))
        {
            UE_LOG(LogTemp, Log, TEXT("  - 任务ID: %s, 类型: %s, 进度: %d/%d, 假任务: %d"),
                *TaskInst.TaskID.ToString(),
                (Data.TaskType == ETaskType::Major ? TEXT("主线") : TEXT("每日")),
                TaskInst.CurrentProgress, TaskInst.TargetProgress,
                TaskInst.bIsFake ? 1 : 0);
        }
    }

    OnTaskUpdated.Broadcast();
}

UDataTable* UTaskComponent::LoadTaskDataTable() const
{
    // 硬编码路径，建议后续改为配置变量
    static const FString DataTablePath = TEXT("/Game/MainFile/Tasks/DT_Tasks.DT_Tasks");
    return Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *DataTablePath));
}

bool UTaskComponent::IsTaskCompleted(FName TaskID) const
{
    int32 Index = FindTaskIndex(TaskID);
    return Index != INDEX_NONE && TaskList[Index].bCompleted;
}

int32 UTaskComponent::GetTaskProgress(FName TaskID) const
{
    int32 Index = FindTaskIndex(TaskID);
    if (Index == INDEX_NONE) return 0;
    return TaskList[Index].CurrentProgress;
}

int32 UTaskComponent::FindTaskIndex(FName TaskID) const
{
    for (int32 i = 0; i < TaskList.Num(); i++)
    {
        if (TaskList[i].TaskID == TaskID)
            return i;
    }
    return INDEX_NONE;
}

void UTaskComponent::RefreshDailyTasks()
{
    if (GetOwnerRole() != ROLE_Authority) return;

    // 移除所有每日任务
    TaskList.RemoveAll([](const FTaskInstance& Task) { return Task.bIsDaily; });

    // 重新添加三个固定每日任务
    UDataTable* TaskTable = LoadTaskDataTable();
    if (!TaskTable) return;

    static const TArray<FName> FixedDailyTasks = { FName("Eat"), FName("Wash"), FName("Sleep") };
    static const FString Context(TEXT("RefreshDaily"));

    for (FName TaskID : FixedDailyTasks)
    {
        FTaskStaticData* Data = TaskTable->FindRow<FTaskStaticData>(TaskID, Context);
        if (!Data) continue;

        FTaskInstance Instance;
        Instance.TaskID = TaskID;
        Instance.TargetProgress = Data->CompletionValue;
        Instance.CurrentProgress = 0;
        Instance.bCompleted = false;
        Instance.bIsDaily = true;
        Instance.bIsFake = false;
        TaskList.Add(Instance);
    }

    // 广播更新
    OnRep_TaskList();

    UE_LOG(LogTemp, Log, TEXT("TaskComponent: 玩家 %s 的每日任务已刷新"), *GetOwner()->GetName());
}