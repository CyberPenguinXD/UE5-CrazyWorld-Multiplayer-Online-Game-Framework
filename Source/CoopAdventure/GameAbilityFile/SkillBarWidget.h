#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SkillTypes.h"
#include "SkillData.h"
#include "SkillBarWidget.generated.h"

class USkillComponent;
class USkillSubsystem;

USTRUCT(BlueprintType)
struct FSkillSlotUIData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    int32 SlotIndex;

    UPROPERTY(BlueprintReadOnly)
    FName SkillID;

    UPROPERTY(BlueprintReadOnly)
    FText SkillName;

    UPROPERTY(BlueprintReadOnly)
    TSoftObjectPtr<UTexture2D> Icon;

    UPROPERTY(BlueprintReadOnly)
    float CooldownPercent;

    UPROPERTY(BlueprintReadOnly)
    float CooldownRemaining;

    UPROPERTY(BlueprintReadOnly)
    int32 CurrentCharges;

    UPROPERTY(BlueprintReadOnly)
    int32 MaxCharges;

    UPROPERTY(BlueprintReadOnly)
    bool bIsReady;

    UPROPERTY(BlueprintReadOnly)
    bool bIsCasting;

    FSkillSlotUIData()
        : SlotIndex(0)
        , Icon(nullptr)       // 关键：确保图标初始为空
        , CooldownPercent(0.0f)
        , CooldownRemaining(0.0f)
        , CurrentCharges(0)
        , MaxCharges(-1)
        , bIsReady(false)
        , bIsCasting(false)
    {
    }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSkillSlotDataUpdated, const FSkillSlotUIData&, SlotData);

UCLASS(Abstract, Blueprintable)
class COOPADVENTURE_API USkillBarWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** 初始化和绑定到技能组件 */
    UFUNCTION(BlueprintCallable, Category = "Skill|UI")
    void BindToSkillComponent(USkillComponent* InSkillComp);

    /** 解除绑定 */
    UFUNCTION(BlueprintCallable, Category = "Skill|UI")
    void UnbindFromSkillComponent();

    /** 手动刷新所有槽位 */
    UFUNCTION(BlueprintCallable, Category = "Skill|UI")
    void RefreshAllSlots();

    /** 强制刷新指定槽位（调试/保险用） */
    UFUNCTION(BlueprintCallable, Category = "Skill|UI")
    void ForceRefreshSlot(int32 SlotIndex);

    /** 获取槽位数据 */
    UFUNCTION(BlueprintPure, Category = "Skill|UI")
    FSkillSlotUIData GetSlotData(int32 SlotIndex) const;

    /** 获取技能子系统 */
    UFUNCTION(BlueprintPure, Category = "Skill|UI")
    USkillSubsystem* GetSkillSubsystem() const;

    /** 委托：槽位数据更新（绑定到UI元素） */
    UPROPERTY(BlueprintAssignable, Category = "Skill|UI")
    FOnSkillSlotDataUpdated OnSlotDataUpdated;

    /** 委托：技能激活（播放动画等） */
    UPROPERTY(BlueprintAssignable, Category = "Skill|UI")
    FOnSkillSlotDataUpdated OnSkillActivated;

protected:
    UPROPERTY(BlueprintReadOnly, Category = "Skill|UI")
    TObjectPtr<USkillComponent> SkillComp;

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    /** C++层回调 */
    UFUNCTION()
    void HandleSlotChanged(int32 SlotIndex, FName SkillID);

    UFUNCTION()
    void HandleCooldownUpdated(int32 SlotIndex);

    UFUNCTION()
    void HandleSkillActivated(int32 SlotIndex, FName SkillID);

    UFUNCTION()
    void HandleCastFinished(int32 SlotIndex, bool bSuccess);

    /** 构建UI数据结构 */
    FSkillSlotUIData BuildSlotData(int32 SlotIndex) const;

    /** 上一次的槽位数据（用于比较变化） */
    UPROPERTY()
    TArray<FSkillSlotUIData> LastSlotData;

    /** 尝试绑定（带重试） */
    void TryBindToPlayerState();

    /** 调试：打印当前状态 */
    void LogCurrentState(const FString& Context) const;
};