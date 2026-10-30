// TaskData.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Blueprint/UserWidget.h"
#include "TaskData.generated.h"

/**
 * 任务类型枚举
 */
UENUM(BlueprintType)
enum class ETaskType : uint8
{
    Daily       UMETA(DisplayName = "每日任务"),   // 吃饭、洗漱、睡觉等日常行为
    Major       UMETA(DisplayName = "主线大任务")   // 解除封印的五个大任务
};

/**
 * 任务静态数据（用于 DataTable 配置）
 */
USTRUCT(BlueprintType)
struct FTaskStaticData : public FTableRowBase
{
    GENERATED_BODY()

    // 任务唯一标识（通常与行名一致）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Task")
    FName TaskID;

    // 任务类型
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Task")
    ETaskType TaskType = ETaskType::Daily;

    // 显示名称
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
    FText DisplayName;

    // 描述文本
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
    FText Description;

    // 关联的任务区域类（可选，用于生成或检测）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Task")
    TSoftClassPtr<AActor> TargetActorClass;

    // 小游戏Widget类（仅对大任务有效）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Task")
    TSoftClassPtr<UUserWidget> MinigameClass;

    // 完成所需次数（例如吃饭需1次，大任务通常为1）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Task", meta = (ClampMin = "1"))
    int32 CompletionValue = 1;

    // 每日任务的时间窗口（24小时制，例如 600 表示 06:00，1800 表示 18:00）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Daily", meta = (ClampMin = "0", ClampMax = "2400"))
    int32 TimeWindowStart = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Daily", meta = (ClampMin = "0", ClampMax = "2400"))
    int32 TimeWindowEnd = 2400;

    // 是否可以被内鬼玩家领取（用于假任务）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Task")
    bool bAllowImpostor = false;

    FTaskStaticData()
        : TaskID(NAME_None)
        , CompletionValue(1)
        , TimeWindowStart(0)
        , TimeWindowEnd(2400)
        , bAllowImpostor(false)
    {
    }
};

/**
 * 任务实例数据结构（用于玩家状态中的任务组件）
 */
USTRUCT(BlueprintType)
struct FTaskInstance
{
    GENERATED_BODY()

    // 任务ID（指向数据表）
    UPROPERTY(BlueprintReadOnly, Category = "Task")
    FName TaskID;

    // 当前进度
    UPROPERTY(BlueprintReadOnly, Category = "Task")
    int32 CurrentProgress = 0;

    // 目标进度（来自静态数据）
    UPROPERTY(BlueprintReadOnly, Category = "Task")
    int32 TargetProgress = 1;

    // 是否已完成
    UPROPERTY(BlueprintReadOnly, Category = "Task")
    bool bCompleted = false;

    // 是否为每日任务（方便分类显示）
    UPROPERTY(BlueprintReadOnly, Category = "Task")
    bool bIsDaily = false;

    // 新增：是否为假任务（内鬼的主线任务）
    UPROPERTY(BlueprintReadOnly, Category = "Task")
    bool bIsFake = false;

    FTaskInstance()
        : TaskID(NAME_None)
        , CurrentProgress(0)
        , TargetProgress(1)
        , bCompleted(false)
        , bIsDaily(false)
        , bIsFake(false)
    {
    }

    // 辅助函数：判断任务是否可完成（未完成且进度未满）
    bool CanProgress() const { return !bCompleted && CurrentProgress < TargetProgress; }

};

USTRUCT(BlueprintType)
struct FTaskUIData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FName TaskID;

    UPROPERTY(BlueprintReadOnly)
    FText DisplayName;

    UPROPERTY(BlueprintReadOnly)
    int32 CurrentProgress;

    UPROPERTY(BlueprintReadOnly)
    int32 TargetProgress;

    UPROPERTY(BlueprintReadOnly)
    bool bCompleted;

    UPROPERTY(BlueprintReadOnly)
    bool bIsDaily;

    FTaskUIData()
        : TaskID(NAME_None)
        , CurrentProgress(0)
        , TargetProgress(1)
        , bCompleted(false)
        , bIsDaily(false)
    {
    }
};