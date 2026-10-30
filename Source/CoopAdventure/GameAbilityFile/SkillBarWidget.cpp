#include "SkillBarWidget.h"
#include "SkillComponent.h"
#include "SkillSubsystem.h"
#include "PlayerStateFile/CoopAdventurePlayerState.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

void USkillBarWidget::NativeConstruct()
{
    Super::NativeConstruct();

    UE_LOG(LogTemp, Warning, TEXT("[SkillBar] NativeConstruct 被调用"));

    // 立即尝试绑定
    TryBindToPlayerState();

    // 如果没绑定成功，延迟重试（处理网络延迟）
    if (!SkillComp)
    { 

        FTimerHandle RetryTimer;
        GetWorld()->GetTimerManager().SetTimer(RetryTimer, [this]()
            {
                if (!SkillComp)
                {
                 
                    TryBindToPlayerState();
                }
            }, 1.0f, false);
    }
}

void USkillBarWidget::TryBindToPlayerState()
{
    if (SkillComp)
    {
        return;
    }

    APlayerController* PC = GetOwningPlayer();
    if (!PC)
    {
        return;
    }

    // 关键修正：去 PlayerState 上查找，不是去 Pawn 上！
    ACoopAdventurePlayerState* PS = PC->GetPlayerState<ACoopAdventurePlayerState>();
    if (!PS)
    {
        return;
    }

    USkillComponent* FoundSkillComp = PS->FindComponentByClass<USkillComponent>();
    if (!FoundSkillComp)
    {
        return;
    }

    BindToSkillComponent(FoundSkillComp);
}

void USkillBarWidget::BindToSkillComponent(USkillComponent* InSkillComp)
{
    if (SkillComp == InSkillComp)
    {
        return;
    }

    // 解绑旧的
    UnbindFromSkillComponent();

    SkillComp = InSkillComp;

    if (SkillComp)
    {
        // 绑定委托
        SkillComp->OnSkillSlotChanged.AddDynamic(this, &USkillBarWidget::HandleSlotChanged);
        SkillComp->OnCooldownUpdated.AddDynamic(this, &USkillBarWidget::HandleCooldownUpdated);
        SkillComp->OnSkillActivated.AddDynamic(this, &USkillBarWidget::HandleSkillActivated);
        SkillComp->OnSkillCastFinished.AddDynamic(this, &USkillBarWidget::HandleCastFinished);

       // UE_LOG(LogTemp, Log, TEXT("[SkillBar] 委托绑定完成，开始初次刷新..."));

        // 初次刷新所有槽位
        RefreshAllSlots();

        LogCurrentState("绑定完成后");
    }
}

void USkillBarWidget::UnbindFromSkillComponent()
{
    if (SkillComp)
    {
        //UE_LOG(LogTemp, Log, TEXT("[SkillBar] 解绑旧组件"));
        SkillComp->OnSkillSlotChanged.RemoveDynamic(this, &USkillBarWidget::HandleSlotChanged);
        SkillComp->OnCooldownUpdated.RemoveDynamic(this, &USkillBarWidget::HandleCooldownUpdated);
        SkillComp->OnSkillActivated.RemoveDynamic(this, &USkillBarWidget::HandleSkillActivated);
        SkillComp->OnSkillCastFinished.RemoveDynamic(this, &USkillBarWidget::HandleCastFinished);
        SkillComp = nullptr;
    }
}

void USkillBarWidget::NativeDestruct()
{
   // UE_LOG(LogTemp, Log, TEXT("[SkillBar] NativeDestruct 被调用"));
    UnbindFromSkillComponent();
    Super::NativeDestruct();
}

void USkillBarWidget::RefreshAllSlots()
{
    if (!SkillComp)
    {
       // UE_LOG(LogTemp, Error, TEXT("[SkillBar] RefreshAllSlots 失败: SkillComp 为 nullptr"));
        return;
    }

  //  UE_LOG(LogTemp, Log, TEXT("[SkillBar] 开始刷新所有槽位"));

    LastSlotData.SetNum(4);

    for (int32 i = 0; i < 4; ++i)
    {
        FSkillSlotUIData Data = BuildSlotData(i);
        LastSlotData[i] = Data;



        OnSlotDataUpdated.Broadcast(Data);
    }

   // UE_LOG(LogTemp, Log, TEXT("[SkillBar] 刷新完成"));
}

void USkillBarWidget::ForceRefreshSlot(int32 SlotIndex)
{
  //  UE_LOG(LogTemp, Warning, TEXT("[SkillBar] 强制刷新槽位%d"), SlotIndex);

    if (!SkillComp)
    {
    //    UE_LOG(LogTemp, Error, TEXT("[SkillBar] ForceRefreshSlot 失败: 未绑定 SkillComp"));
        return;
    }

    if (SlotIndex < 0 || SlotIndex >= 4)
    {
    //    UE_LOG(LogTemp, Error, TEXT("[SkillBar] ForceRefreshSlot 失败: 索引%d超出范围"), SlotIndex);
        return;
    }

    FSkillSlotUIData Data = BuildSlotData(SlotIndex);
    LastSlotData.SetNum(FMath::Max(LastSlotData.Num(), SlotIndex + 1));
    LastSlotData[SlotIndex] = Data;
    OnSlotDataUpdated.Broadcast(Data);

  //  UE_LOG(LogTemp, Log, TEXT("[SkillBar] 强制刷新完成: ID=%s"), *Data.SkillID.ToString());
}

FSkillSlotUIData USkillBarWidget::BuildSlotData(int32 SlotIndex) const
{
    FSkillSlotUIData Data;
    Data.SlotIndex = SlotIndex;

    if (!IsValid(SkillComp))
    {
        return Data;
    }

    Data.SkillID = SkillComp->GetSkillInSlot(SlotIndex);

    // 只有技能ID有效才查询数据
    if (!Data.SkillID.IsNone())
    {
        if (USkillSubsystem* Subsystem = GetSkillSubsystem())
        {
            FSkillStaticData SkillData;
            if (Subsystem->GetSkillData(Data.SkillID, SkillData))
            {
                Data.SkillName = SkillData.SkillName;

                // 修复：软引用安全获取图标
                if (SkillData.Icon.IsValid())  // 使用 IsValid() 而不是与 nullptr 比较
                {
                    Data.Icon = SkillData.Icon.LoadSynchronous();  // 加载为硬指针
                }
                else
                {
                    Data.Icon = nullptr;
                }

                Data.MaxCharges = SkillData.MaxCharges;
            }
            else
            {
                Data.SkillName = FText::FromString(TEXT("未知技能"));
                Data.Icon = nullptr;
            }
        }
    }
    else
    {
        // 槽位为空
        Data.SkillName = FText::FromString(TEXT("空"));
        Data.Icon = nullptr;
        Data.MaxCharges = -1;
    }

    Data.CooldownPercent = SkillComp->GetCooldownPercent(SlotIndex);
    Data.CooldownRemaining = SkillComp->GetCooldownRemaining(SlotIndex);
    Data.CurrentCharges = SkillComp->GetCurrentCharges(SlotIndex);
    Data.bIsReady = SkillComp->IsSlotReady(SlotIndex);
    Data.bIsCasting = SkillComp->IsCasting(SlotIndex);

    return Data;
}

void USkillBarWidget::HandleSlotChanged(int32 SlotIndex, FName SkillID)
{
    /**/
  //  UE_LOG(LogTemp, Warning, TEXT("[SkillBar] ====== HandleSlotChanged 被调用 ======"));
   // UE_LOG(LogTemp, Log, TEXT("       槽位: %d"), SlotIndex);
  //  UE_LOG(LogTemp, Log, TEXT("       新SkillID: %s"), *SkillID.ToString());
  //  UE_LOG(LogTemp, Log, TEXT("       SkillID 是否有效(IsNone): %s"), SkillID.IsNone() ? TEXT("是(None)") : TEXT("否"));

    if (!SkillComp)
    {
   //     UE_LOG(LogTemp, Error, TEXT("[SkillBar] HandleSlotChanged 错误: SkillComp 为 nullptr!"));
        return;
    }

    // 构建新数据
    FSkillSlotUIData Data = BuildSlotData(SlotIndex);

    // 确保数组足够大
    LastSlotData.SetNum(FMath::Max(LastSlotData.Num(), SlotIndex + 1));

    // 记录旧数据用于比较
    FName OldID = LastSlotData[SlotIndex].SkillID;
    LastSlotData[SlotIndex] = Data;

  //  UE_LOG(LogTemp, Log, TEXT("[SkillBar] 数据更新: %s -> %s"), *OldID.ToString(), *Data.SkillID.ToString());
  //  UE_LOG(LogTemp, Log, TEXT("[SkillBar] 准备广播 OnSlotDataUpdated..."));

    // 关键：无论是否为空，都广播事件（确保UI清空）
    OnSlotDataUpdated.Broadcast(Data);

  //  UE_LOG(LogTemp, Log, TEXT("[SkillBar] 广播完成"));
}

void USkillBarWidget::HandleCooldownUpdated(int32 SlotIndex)
{
   // UE_LOG(LogTemp, Verbose, TEXT("[SkillBar] HandleCooldownUpdated: 槽位%d"), SlotIndex);
    HandleSlotChanged(SlotIndex, SkillComp ? SkillComp->GetSkillInSlot(SlotIndex) : NAME_None);
}

void USkillBarWidget::HandleSkillActivated(int32 SlotIndex, FName SkillID)
{
 //   UE_LOG(LogTemp, Log, TEXT("[SkillBar] HandleSkillActivated: 槽位%d, 技能%s"), SlotIndex, *SkillID.ToString());
    FSkillSlotUIData Data = BuildSlotData(SlotIndex);
    OnSkillActivated.Broadcast(Data);
}

void USkillBarWidget::HandleCastFinished(int32 SlotIndex, bool bSuccess)
{
  //  UE_LOG(LogTemp, Log, TEXT("[SkillBar] HandleCastFinished: 槽位%d, 成功=%d"), SlotIndex, (int32)bSuccess);
    HandleSlotChanged(SlotIndex, SkillComp ? SkillComp->GetSkillInSlot(SlotIndex) : NAME_None);
}

void USkillBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    // 每帧检查冷却更新（仅对冷却中的槽位）
    if (SkillComp)
    {
        for (int32 i = 0; i < 4; ++i)
        {
            float Remaining = SkillComp->GetCooldownRemaining(i);

            // 如果正在冷却，或者刚结束冷却，需要刷新
            if (Remaining > 0.0f || (LastSlotData.IsValidIndex(i) && LastSlotData[i].CooldownRemaining > 0.0f))
            {
                FSkillSlotUIData Data = BuildSlotData(i);

                bool bChanged = !LastSlotData.IsValidIndex(i);
                if (!bChanged)
                {
                    bChanged = (FMath::Abs(Data.CooldownPercent - LastSlotData[i].CooldownPercent) > 0.01f) ||
                        (Data.SkillID != LastSlotData[i].SkillID);
                }

                if (bChanged)
                {
                    LastSlotData.SetNum(FMath::Max(LastSlotData.Num(), i + 1));
                    LastSlotData[i] = Data;
                    OnSlotDataUpdated.Broadcast(Data);
                }
            }
        }
    }
}

USkillSubsystem* USkillBarWidget::GetSkillSubsystem() const
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

void USkillBarWidget::LogCurrentState(const FString& Context) const
{
   // UE_LOG(LogTemp, Log, TEXT("[SkillBar] === 当前状态 [%s] ==="), *Context);
  //  UE_LOG(LogTemp, Log, TEXT("       SkillComp: %s"), SkillComp ? TEXT("有效") : TEXT("nullptr"));

    if (SkillComp)
    {
        for (int32 i = 0; i < 4; ++i)
        {
            FName ID = SkillComp->GetSkillInSlot(i);
   //         UE_LOG(LogTemp, Log, TEXT("       槽位%d: %s"), i, *ID.ToString());
        }
    }
}


FSkillSlotUIData USkillBarWidget::GetSlotData(int32 SlotIndex) const
{
    return BuildSlotData(SlotIndex);
}