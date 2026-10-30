#include "TaskBarWidget.h"
#include "AGameplayFile/TaskComponent.h"
#include "PlayerStateFile/CoopAdventurePlayerState.h"

void UTaskBarWidget::BindToTaskComponent(UTaskComponent* InTaskComp)
{
    if (TaskComp == InTaskComp) return;

    // 解绑旧组件
    if (TaskComp)
    {
        TaskComp->OnTaskUpdated.RemoveDynamic(this, &UTaskBarWidget::HandleTaskUpdated);
    }

    TaskComp = InTaskComp;

    if (TaskComp)
    {
        TaskComp->OnTaskUpdated.AddDynamic(this, &UTaskBarWidget::HandleTaskUpdated);
        HandleTaskUpdated(); // 立即刷新缓存和UI
    }
    else
    {
        // 清空缓存并通知UI清空
        ClearCache();
        OnTaskListUpdated();
    }
}

void UTaskBarWidget::HandleTaskUpdated()
{
    RebuildTaskCache();
    OnTaskListUpdated();
}

void UTaskBarWidget::RebuildTaskCache()
{
    CachedTaskUIData.Empty();

    if (!TaskComp) return;

    for (const FTaskInstance& Instance : TaskComp->TaskList)
    {
        FTaskUIData Data;
        Data.TaskID = Instance.TaskID;
        Data.CurrentProgress = Instance.CurrentProgress;
        Data.TargetProgress = Instance.TargetProgress;
        Data.bCompleted = Instance.bCompleted;
        Data.bIsDaily = Instance.bIsDaily;

        // 获取静态数据填充显示名称
        FTaskStaticData StaticData;
        if (TaskComp->GetTaskStaticData(Instance.TaskID, StaticData))
        {
            Data.DisplayName = StaticData.DisplayName;
        }
        else
        {
            Data.DisplayName = FText::FromName(Instance.TaskID);
        }

        CachedTaskUIData.Add(Data);
    }
}

void UTaskBarWidget::ClearCache()
{
    CachedTaskUIData.Empty();
}

int32 UTaskBarWidget::GetCompletedTaskCount() const
{
    int32 Count = 0;
    for (const FTaskUIData& Data : CachedTaskUIData)
    {
        if (Data.bCompleted) Count++;
    }
    return Count;
}

int32 UTaskBarWidget::GetCompletedMajorTaskCount() const
{
    int32 Count = 0;
    for (const FTaskUIData& Data : CachedTaskUIData)
    {
        if (!Data.bIsDaily && Data.bCompleted) Count++;
    }
    return Count;
}