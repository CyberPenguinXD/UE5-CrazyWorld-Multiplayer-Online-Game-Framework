// GameAbilityFile/SkillTreeWidget.cpp
#include "SkillTreeWidget.h"
#include "PlayerStateFile/CoopAdventurePlayerState.h"
#include "AGameplayFile/IdentityComponent.h"
#include "SkillComponent.h"
#include "SkillSubsystem.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

void USkillTreeWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (!OwnerPlayerState)
    {
        if (APlayerController* PC = GetOwningPlayer())
        {
            SetupSkillTree(PC->GetPlayerState<ACoopAdventurePlayerState>());
        }
    }
}

void USkillTreeWidget::NativeDestruct()
{
    // 解绑所有委托
    if (IdentityComp)
    {
        IdentityComp->OnSkillPointsChanged.RemoveDynamic(this, &USkillTreeWidget::HandlePointsChanged);
    }
    if (SkillComp)
    {
        SkillComp->OnSkillSlotChanged.RemoveDynamic(this, &USkillTreeWidget::HandleSlotChanged);
    }

    Super::NativeDestruct();
}

void USkillTreeWidget::SetupSkillTree(ACoopAdventurePlayerState* InPlayerState)
{
    if (!InPlayerState)
    {
        UE_LOG(LogTemp, Warning, TEXT("SkillTreeWidget: Setup failed, PlayerState is null"));
        return;
    }

    OwnerPlayerState = InPlayerState;
    IdentityComp = InPlayerState->GetIdentityComponent();
    SkillComp = InPlayerState->GetSkillComponent();

    // 绑定委托
    if (IdentityComp)
    {
        IdentityComp->OnSkillPointsChanged.AddDynamic(this, &USkillTreeWidget::HandlePointsChanged);
    }

    if (SkillComp)
    {
        SkillComp->OnSkillSlotChanged.AddDynamic(this, &USkillTreeWidget::HandleSlotChanged);
    }

    // 初始刷新
    RefreshSkillTree();
    OnPointsUpdated(GetRemainingPoints());
}

TArray<FSkillTreeItemInfo> USkillTreeWidget::GetSkillItemsForDisplay() const
{
    TArray<FSkillTreeItemInfo> Result;

    if (!IsValid(IdentityComp) || !IsValid(SkillComp))
    {
        UE_LOG(LogTemp, Warning, TEXT("SkillTreeWidget: IdentityComp或SkillComp无效"));
        return Result;
    }

    TArray<FName> SkillIDs = GetLearnableSkills();
    int32 CurrentPoints = GetRemainingPoints();
    EPlayerIdentityType CurrentIdentity = GetCurrentIdentity();

    for (const FName& ID : SkillIDs)
    {
        FSkillStaticData Data;
        if (!GetSkillData(ID, Data))
            continue;

        if (Data.IdentityType != EPlayerIdentityType::None && Data.IdentityType != CurrentIdentity)
            continue;

        FSkillTreeItemInfo Info;
        Info.SkillID = ID;
        Info.SkillName = Data.SkillName;
        Info.Description = Data.Description;
        Info.Icon = Data.Icon;  // 直接赋值软引用，不加载
        Info.SkillPointCost = Data.SkillPointCost;
        Info.RequiredIdentity = Data.IdentityType;
        Info.bIsLearned = HasLearnedSkill(ID);
        Info.bCanLearn = !Info.bIsLearned && CurrentPoints >= Data.SkillPointCost;
        Info.EquippedSlotIndex = GetEquippedSlotForSkill(ID);
        Info.bIsEquipped = Info.EquippedSlotIndex != -1;

        Result.Add(Info);
    }

    return Result;
}


void USkillTreeWidget::OnSkillItemClicked(FName SkillID)
{
    if (!IdentityComp || !SkillComp)
        return;

    FSkillStaticData Data;
    if (!GetSkillData(SkillID, Data))
        return;

    // 状态机处理
    if (!HasLearnedSkill(SkillID))
    {
        // 未学习 -> 尝试学习
        if (GetRemainingPoints() >= Data.SkillPointCost)
        {
            RequestLearnSkill(SkillID);
        }
        else
        {
            OnLearnFailed(SkillID, FText::FromString(TEXT("技能点不足")));
        }
    }
    else if (GetEquippedSlotForSkill(SkillID) == -1)
    {
        // 已学习但未装备 -> 尝试装备
        TryEquipSkill(SkillID);
    }
    else
    {
        // 已装备 -> 卸载
        int32 SlotIdx = GetEquippedSlotForSkill(SkillID);
        RequestUnequipSlot(SlotIdx);
    }
}

void USkillTreeWidget::TryEquipSkill(FName SkillID)
{
    // 先找空槽
    int32 EmptySlot = FindEmptySkillSlot();

    if (EmptySlot != -1)
    {
        // 有空槽，直接装备
        RequestEquipToSlot(SkillID, EmptySlot);
        UE_LOG(LogTemp, Log, TEXT("SkillTree: Auto-equipped %s to slot %d"), *SkillID.ToString(), EmptySlot);
    }
    else
    {
        // 无空槽，通知蓝图显示选择器（替换哪个槽）
        OnRequestSlotSelection.Broadcast(SkillID);
        UE_LOG(LogTemp, Log, TEXT("SkillTree: No empty slot, requesting selection for %s"), *SkillID.ToString());
    }
}

void USkillTreeWidget::ConfirmEquipSlot(FName SkillID, int32 SlotIndex)
{
    if (SlotIndex >= 0 && SlotIndex < 4)
    {
        // 直接请求装备（SkillComp内部会自动处理替换逻辑）
        RequestEquipToSlot(SkillID, SlotIndex);
    }
}

void USkillTreeWidget::RefreshSkillTree()
{
    // 触发蓝图更新事件
    OnSkillDataUpdated.Broadcast();

    // 同时更新技能点显示
    OnPointsUpdated(GetRemainingPoints());
}

int32 USkillTreeWidget::FindEmptySkillSlot() const
{
    if (!SkillComp)
        return -1;

    for (int32 i = 0; i < 4; ++i)
    {
        if (SkillComp->GetSkillInSlot(i).IsNone())
        {
            return i;
        }
    }
    return -1;
}

int32 USkillTreeWidget::GetEquippedSlotForSkill(FName SkillID) const
{
    if (!SkillComp || SkillID.IsNone())
        return -1;

    for (int32 i = 0; i < 4; ++i)
    {
        if (SkillComp->GetSkillInSlot(i) == SkillID)
        {
            return i;
        }
    }
    return -1;
}

void USkillTreeWidget::HandlePointsChanged()
{
    OnPointsUpdated(GetRemainingPoints());
    RefreshSkillTree(); // 技能点变化可能影响可学习状态
}

void USkillTreeWidget::HandleSlotChanged(int32 SlotIndex, FName SkillID)
{
    FSkillStaticData Data;
    FText DisplayName = FText::FromString(TEXT("空"));
    if (GetSkillData(SkillID, Data))
    {
        DisplayName = Data.SkillName;
    }

    OnSlotUpdated(SlotIndex, SkillID, DisplayName);
    RefreshSkillTree(); // 装备变化刷新列表
}

// ========== 原有基础接口实现 ==========

EPlayerIdentityType USkillTreeWidget::GetCurrentIdentity() const
{
    if (IdentityComp)
    {
        return IdentityComp->GetCurrentIdentity();
    }
    return EPlayerIdentityType::None;
}

int32 USkillTreeWidget::GetRemainingPoints() const
{
    if (IdentityComp)
    {
        return IdentityComp->GetSkillPoints(GetCurrentIdentity());
    }
    return 0;
}

bool USkillTreeWidget::HasLearnedSkill(FName SkillID) const
{
    if (IdentityComp)
    {
        return IdentityComp->HasLearnedSkill(SkillID);
    }
    return false;
}

TArray<FName> USkillTreeWidget::GetLearnableSkills() const
{
    TArray<FName> Result;
    if (!IdentityComp) return Result;

    USkillSubsystem* Subsystem = GetSkillSubsystem();
    if (!Subsystem) return Result;

    EPlayerIdentityType CurrentIdentity = GetCurrentIdentity();
    TArray<FName> AllSkills = Subsystem->GetAllSkillIDs();

    for (FName SkillID : AllSkills)
    {
        FSkillStaticData Data;
        if (Subsystem->GetSkillData(SkillID, Data))
        {
            // 身份匹配检查
            bool bIdentityMatch = (Data.IdentityType == EPlayerIdentityType::None)
                || (Data.IdentityType == CurrentIdentity)
                || (CurrentIdentity == EPlayerIdentityType::None); // 新增：None身份可以看到所有

            if (bIdentityMatch)
            {
                Result.Add(SkillID);
            }
        }
    }

    return Result;
}

bool USkillTreeWidget::GetSkillData(FName SkillID, FSkillStaticData& OutData) const
{
    if (USkillSubsystem* Subsystem = GetSkillSubsystem())
    {
        return Subsystem->GetSkillData(SkillID, OutData);
    }
    return false;
}

FName USkillTreeWidget::GetEquippedSkillAtSlot(int32 SlotIndex) const
{
    if (SkillComp && SlotIndex >= 0 && SlotIndex < 4)
    {
        return SkillComp->GetSkillInSlot(SlotIndex);
    }
    return NAME_None;
}

void USkillTreeWidget::RequestLearnSkill(FName SkillID)
{
    if (!IdentityComp) return;

    if (HasLearnedSkill(SkillID))
    {
        OnLearnFailed(SkillID, FText::FromString(TEXT("已学习该技能")));
        return;
    }

    FSkillStaticData Data;
    if (GetSkillData(SkillID, Data))
    {
        if (GetRemainingPoints() < Data.SkillPointCost)
        {
            OnLearnFailed(SkillID, FText::FromString(TEXT("技能点不足")));
            return;
        }
    }

    IdentityComp->Server_LearnSkill(SkillID);

    // 立即触发本地UI更新（ optimistic update ）
    OnSkillLearned(SkillID, Data.SkillName);
}

void USkillTreeWidget::RequestEquipToSlot(FName SkillID, int32 SlotIndex)
{
    if (!SkillComp) return;

    if (!HasLearnedSkill(SkillID))
    {
        OnLearnFailed(SkillID, FText::FromString(TEXT("未学习该技能")));
        return;
    }

    SkillComp->EquipSkill(SkillID, SlotIndex);
}

void USkillTreeWidget::RequestUnequipSlot(int32 SlotIndex)
{
    if (SkillComp)
    {
        SkillComp->UnequipSkill(SlotIndex);
    }
}

USkillSubsystem* USkillTreeWidget::GetSkillSubsystem() const
{
    if (UWorld* World = GetWorld())
    {
        if (UGameInstance* GI = World->GetGameInstance())
        {
            return GI->GetSubsystem<USkillSubsystem>();
        }
    }
    return nullptr;
}