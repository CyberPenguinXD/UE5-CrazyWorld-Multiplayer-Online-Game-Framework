#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TaskData.h"
#include "TaskBarWidget.generated.h"

class UTaskComponent;

UCLASS(Abstract, Blueprintable)
class COOPADVENTURE_API UTaskBarWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // 初始化绑定（通常在 Construct 中调用）
    UFUNCTION(BlueprintCallable, Category = "Task|UI")
    void BindToTaskComponent(UTaskComponent* InTaskComp);

    // 获取所有任务的 UI 数据（缓存版本，高效）
    UFUNCTION(BlueprintPure, Category = "Task|UI")
    const TArray<FTaskUIData>& GetTaskList() const { return CachedTaskUIData; }

    // 获取任务总数
    UFUNCTION(BlueprintPure, Category = "Task|UI")
    int32 GetTaskCount() const { return CachedTaskUIData.Num(); }

    // 获取已完成任务数量
    UFUNCTION(BlueprintPure, Category = "Task|UI")
    int32 GetCompletedTaskCount() const;

    // 获取主线任务完成数量
    UFUNCTION(BlueprintPure, Category = "Task|UI")
    int32 GetCompletedMajorTaskCount() const;

protected:
    UPROPERTY(BlueprintReadOnly, Category = "Task|UI")
    TObjectPtr<UTaskComponent> TaskComp;

    // 缓存的 UI 数据（避免重复构建）
    UPROPERTY()
    TArray<FTaskUIData> CachedTaskUIData;

    // 蓝图可覆盖的更新事件
    UFUNCTION(BlueprintImplementableEvent, Category = "Task|UI")
    void OnTaskListUpdated();

    // C++ 内部回调
    UFUNCTION()
    void HandleTaskUpdated();

    // 重建缓存
    void RebuildTaskCache();

    // 清空缓存（解绑时调用）
    void ClearCache();
};