// GameAbilityFile/SkillTreeWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AGameplayFile/IdentityTypes.h"
#include "SkillData.h"
#include "SkillTreeWidget.generated.h"

// 前向声明
class ACoopAdventurePlayerState;
class UIdentityComponent;
class USkillComponent;
class USkillSubsystem;

// 技能树单项UI数据（打包所有必要信息，蓝图只读）
USTRUCT(BlueprintType)
struct FSkillTreeItemInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FName SkillID;

    UPROPERTY(BlueprintReadOnly)
    FText SkillName;

    UPROPERTY(BlueprintReadOnly)
    TSoftObjectPtr<UTexture2D> Icon;

    UPROPERTY(BlueprintReadOnly)
    int32 SkillPointCost;

    UPROPERTY(BlueprintReadOnly)
    bool bIsLearned;

    UPROPERTY(BlueprintReadOnly)
    bool bIsEquipped;

    UPROPERTY(BlueprintReadOnly)
    int32 EquippedSlotIndex; // -1表示未装备

    UPROPERTY(BlueprintReadOnly)
    bool bCanLearn; // 技能点是否足够且未学习

    UPROPERTY(BlueprintReadOnly)
    EPlayerIdentityType RequiredIdentity;

    UPROPERTY(BlueprintReadOnly)
    FText Description; // 技能描述

    FSkillTreeItemInfo()
        : SkillID(NAME_None)
        , Icon(nullptr)              // 关键：初始化为nullptr！
        , SkillPointCost(1)
        , bIsLearned(false)
        , bIsEquipped(false)
        , EquippedSlotIndex(-1)
        , bCanLearn(false)
        , RequiredIdentity(EPlayerIdentityType::None)
    {
    }
};

// 槽位选择请求委托（无空槽时触发）
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRequestSlotSelection, FName, SkillID);

// 数据更新委托（通知蓝图刷新列表）
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSkillTreeDataUpdated);

UCLASS(Abstract, Blueprintable)
class COOPADVENTURE_API USkillTreeWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // ========== 初始化 ==========

    UFUNCTION(BlueprintCallable, Category = "SkillTree")
    void SetupSkillTree(ACoopAdventurePlayerState* InPlayerState);

    // ========== 蓝图简化接口（核心） ==========

    /**
     * 获取所有技能项的显示数据
     * 蓝图只需遍历此数组创建WBP_SkillTreeItem
     */
    UFUNCTION(BlueprintCallable, Category = "SkillTree|Data")
    TArray<FSkillTreeItemInfo> GetSkillItemsForDisplay() const;

    /**
     * 处理技能项点击（C++内部自动判断是学习/装备/卸载）
     * @param SkillID - 点击的技能ID
     */
    UFUNCTION(BlueprintCallable, Category = "SkillTree|Interaction")
    void OnSkillItemClicked(FName SkillID);

    /**
     * 确认装备到指定槽位（由槽位选择弹窗调用）
     * @param SkillID - 要装备的技能
     * @param SlotIndex - 选择的槽位(0-3)
     */
    UFUNCTION(BlueprintCallable, Category = "SkillTree|Interaction")
    void ConfirmEquipSlot(FName SkillID, int32 SlotIndex);

    /**
     * 手动强制刷新（通常不需要，数据变化会自动触发）
     */
    UFUNCTION(BlueprintCallable, Category = "SkillTree|Data")
    void RefreshSkillTree();

    // ========== 基础查询（保持原有功能） ==========

    UFUNCTION(BlueprintPure, Category = "SkillTree")
    EPlayerIdentityType GetCurrentIdentity() const;

    UFUNCTION(BlueprintPure, Category = "SkillTree")
    int32 GetRemainingPoints() const;

    UFUNCTION(BlueprintPure, Category = "SkillTree")
    bool HasLearnedSkill(FName SkillID) const;

    UFUNCTION(BlueprintPure, Category = "SkillTree")
    TArray<FName> GetLearnableSkills() const;

    UFUNCTION(BlueprintPure, Category = "SkillTree")
    bool GetSkillData(FName SkillID, FSkillStaticData& OutData) const;

    UFUNCTION(BlueprintPure, Category = "SkillTree")
    FName GetEquippedSkillAtSlot(int32 SlotIndex) const;

    // ========== 原始请求接口（蓝图可选使用） ==========

    UFUNCTION(BlueprintCallable, Category = "SkillTree")
    void RequestLearnSkill(FName SkillID);

    UFUNCTION(BlueprintCallable, Category = "SkillTree")
    void RequestEquipToSlot(FName SkillID, int32 SlotIndex);

    UFUNCTION(BlueprintCallable, Category = "SkillTree")
    void RequestUnequipSlot(int32 SlotIndex);

    // ========== 委托事件（蓝图绑定） ==========

    /** 当需要选择装备槽位时触发（无空槽时） */
    UPROPERTY(BlueprintAssignable, Category = "SkillTree|Events")
    FOnRequestSlotSelection OnRequestSlotSelection;

    /** 当数据已更新（学习/装备/卸载成功），通知蓝图刷新列表 */
    UPROPERTY(BlueprintAssignable, Category = "SkillTree|Events")
    FOnSkillTreeDataUpdated OnSkillDataUpdated;

    /** 技能点变化 - 蓝图可实现此事件 */
    UFUNCTION(BlueprintImplementableEvent, Category = "SkillTree|Events")
    void OnPointsUpdated(int32 NewPoints);

    /** 学习成功 - 蓝图可实现此事件 */
    UFUNCTION(BlueprintImplementableEvent, Category = "SkillTree|Events")
    void OnSkillLearned(FName SkillID, const FText& SkillName);

    /** 槽位更新（装备/卸载） - 蓝图可实现此事件 */
    UFUNCTION(BlueprintImplementableEvent, Category = "SkillTree|Events")
    void OnSlotUpdated(int32 SlotIndex, FName SkillID, const FText& SkillName);

    /** 学习失败 - 蓝图可实现此事件 */
    UFUNCTION(BlueprintImplementableEvent, Category = "SkillTree|Events")
    void OnLearnFailed(FName SkillID, const FText& Reason);

  

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    UPROPERTY(BlueprintReadOnly, Category = "SkillTree")
    TObjectPtr<ACoopAdventurePlayerState> OwnerPlayerState;

    UPROPERTY(BlueprintReadOnly, Category = "SkillTree")
    TObjectPtr<UIdentityComponent> IdentityComp;

    UPROPERTY(BlueprintReadOnly, Category = "SkillTree")
    TObjectPtr<USkillComponent> SkillComp;

private:
    // ========== 内部逻辑 ==========

    /** 尝试装备技能（自动找空槽或触发选择） */
    void TryEquipSkill(FName SkillID);

    /** 寻找第一个空技能槽（0-3），没找到返回-1 */
    int32 FindEmptySkillSlot() const;

    /** 获取技能当前装备在哪个槽（-1=未装备） */
    int32 GetEquippedSlotForSkill(FName SkillID) const;

    /** 判断技能是否可被当前身份学习 */
    bool CanLearnByIdentity(const FSkillStaticData& Data) const;

    // ========== 委托回调 ==========

    UFUNCTION()
    void HandlePointsChanged();

    UFUNCTION()
    void HandleSlotChanged(int32 SlotIndex, FName SkillID);

    USkillSubsystem* GetSkillSubsystem() const;
};