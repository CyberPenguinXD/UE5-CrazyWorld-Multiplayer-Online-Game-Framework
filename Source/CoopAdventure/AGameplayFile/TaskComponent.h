#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TaskData.h"
#include "TaskComponent.generated.h"

// 任务更新委托（用于UI刷新）
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTaskUpdated);

// 主线任务完成委托（供胜利条件检测）
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMajorTaskCompleted, FName, TaskID);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class COOPADVENTURE_API UTaskComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UTaskComponent();

    // ===== 网络复制 =====
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // ===== 任务数据 =====
    UPROPERTY(ReplicatedUsing = OnRep_TaskList, BlueprintReadOnly, Category = "Task")
    TArray<FTaskInstance> TaskList;

    // ===== 委托 =====
    UPROPERTY(BlueprintAssignable, Category = "Task")
    FOnTaskUpdated OnTaskUpdated;

    // 当某个主线任务完成时触发（可用于胜利条件检测）
    UPROPERTY(BlueprintAssignable, Category = "Task")
    FOnMajorTaskCompleted OnMajorTaskCompleted;

    // ===== 服务器调用：初始化任务（分配每日和主线任务）=====
    UFUNCTION(BlueprintCallable, Category = "Task")
    void InitializeTasks();

    // ===== 服务器调用：尝试完成任务（由任务区域触发）=====
    UFUNCTION(BlueprintCallable, Category = "Task")
    void TryCompleteTask(FName TaskID);

    // ===== 客户端请求完成任务（RPC）=====
    UFUNCTION(Server, Reliable)
    void Server_CompleteTask(FName TaskID);

    // ===== 查询接口 =====
    UFUNCTION(BlueprintPure, Category = "Task")
    bool IsTaskCompleted(FName TaskID) const;

    UFUNCTION(BlueprintPure, Category = "Task")
    int32 GetTaskProgress(FName TaskID) const;

    UFUNCTION(BlueprintPure, Category = "Task")
    bool GetTaskStaticData(FName TaskID, FTaskStaticData& OutData) const;


protected:
    virtual void BeginPlay() override;

    // 当任务列表复制到客户端时触发
    UFUNCTION()
    void OnRep_TaskList();

    // 从数据表加载任务配置
    class UDataTable* LoadTaskDataTable() const;

    // 辅助：查找任务索引
    int32 FindTaskIndex(FName TaskID) const;

public:
    // 刷新每日任务（清除现有每日任务，重新添加）
    UFUNCTION(BlueprintCallable, Category = "Task")
    void RefreshDailyTasks();


public:
    // 获取已完成主线任务数量（不包括假任务）
    UFUNCTION(BlueprintPure, Category = "Task")
    int32 GetCompletedMajorTaskCount() const { return CompletedMajorTaskCount; }

    // 检查是否已完成所有主线任务（5个）
    UFUNCTION(BlueprintPure, Category = "Task")
    bool HasCompletedAllMajorTasks() const { return CompletedMajorTaskCount >= 5; }

protected:
    // 已完成的主线任务数量（非假任务）
    UPROPERTY()
    int32 CompletedMajorTaskCount = 0;
};