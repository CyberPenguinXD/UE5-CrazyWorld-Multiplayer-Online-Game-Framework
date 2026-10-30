// InventoryComponent.cpp
#include "InventoryComponent.h"
#include "InventorySubsystem.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/PlayerState.h"
#include "CoopAdventureCharacter.h"  // 添加这行
#include "GameFramework/Pawn.h"
#include "PickupItem.h"

UInventoryComponent::UInventoryComponent()
{
    // 设置复制
    SetIsReplicatedByDefault(true);

    // 默认值
    MaxSlots = 20;
    MaxWeight = 100.0f;
    CurrentWeight = 0.0f;

    EquippedSlotIndex = -1;
}

void UInventoryComponent::BeginPlay()
{
    Super::BeginPlay();

    // 关键：大幅提高网络更新频率到100Hz（使用新的API）
    if (AActor* Owner = GetOwner())
    {
        Owner->SetNetUpdateFrequency(100.0f);      // 每秒最多更新100次
        Owner->SetMinNetUpdateFrequency(60.0f);    // 最低保证60Hz（16ms间隔）

        UE_LOG(LogTemp, Log, TEXT("InventoryComponent: NetUpdateFrequency set to 100Hz"));
    }
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UInventoryComponent, InventoryItems);
    DOREPLIFETIME(UInventoryComponent, CurrentWeight);

    // 【新增】同步装备槽位
    DOREPLIFETIME(UInventoryComponent, EquippedSlotIndex);
}

// ========== 查询接口 ==========

int32 UInventoryComponent::GetItemQuantity(FName ItemID) const
{
    int32 Total = 0;
    for (const FInventoryEntry& Entry : InventoryItems)
    {
        if (Entry.ItemID == ItemID)
        {
            Total += Entry.Quantity;
        }
    }
    return Total;
}

bool UInventoryComponent::CanAddItem(FName ItemID, int32 Quantity) const
{
    if (Quantity <= 0) return false;
    if (IsFull()) return false;

    // 查物品信息
    if (UInventorySubsystem* Subsystem = GetInventorySubsystem())
    {
        FItemStaticData Data;
        if (Subsystem->GetItemData(ItemID, Data))
        {
            // 检查负重
            if (MaxWeight > 0)
            {
                float AddedWeight = Data.Weight * Quantity;
                if (CurrentWeight + AddedWeight > MaxWeight)
                {
                    return false;
                }
            }

            // 检查能否堆叠
            int32 ExistingIndex = FindItemIndex(ItemID);
            if (ExistingIndex != -1)
            {
                // 已有该物品，检查堆叠空间
                int32 CurrentQty = InventoryItems[ExistingIndex].Quantity;
                return CurrentQty + Quantity <= Data.MaxStackSize;
            }
            else
            {
                // 新物品，检查空位
                return InventoryItems.Num() < MaxSlots;
            }
        }
    }
    return false;
}

// ========== 操作接口（入口，自动路由到Server） ==========

void UInventoryComponent::AddItem(FName ItemID, int32 Quantity)
{
    if (GetOwnerRole() == ROLE_Authority)
    {
        // Server直接执行
        Server_AddItem_Implementation(ItemID, Quantity);
    }
    else
    {
        // Client发送RPC
        Server_AddItem(ItemID, Quantity);
    }
}

void UInventoryComponent::RemoveItem(FName ItemID, int32 Quantity)
{
    int32 Index = FindItemIndex(ItemID);
    if (Index != -1)
    {
        RemoveItemAtIndex(Index, Quantity);
    }
}

void UInventoryComponent::RemoveItemAtIndex(int32 Index, int32 Quantity)
{
    if (GetOwnerRole() == ROLE_Authority)
    {
        Server_RemoveItemAtIndex_Implementation(Index, Quantity);
    }
    else
    {
        Server_RemoveItemAtIndex(Index, Quantity);
    }
}

void UInventoryComponent::MoveItem(int32 FromIndex, int32 ToIndex)
{
    if (GetOwnerRole() == ROLE_Authority)
    {
        Server_MoveItem_Implementation(FromIndex, ToIndex);
    }
    else
    {
        Server_MoveItem(FromIndex, ToIndex);
    }
}

void UInventoryComponent::UseItem(int32 Index)
{
    if (GetOwnerRole() == ROLE_Authority)
    {
        Server_UseItem_Implementation(Index);
    }
    else
    {
        Server_UseItem(Index);
    }
}

// ========== 丢弃物品（关键修改）==========

void UInventoryComponent::DropItem(int32 Index, int32 Quantity)
{
    // 计算丢弃位置（客户端和服务器都计算，确保一致）
    FVector DropLocation = FVector::ZeroVector;
    FRotator DropRotation = FRotator::ZeroRotator;

    // 通过 PlayerState 获取 Pawn
    APlayerState* PS = Cast<APlayerState>(GetOwner());
    if (PS)
    {
        APawn* Pawn = PS->GetPawn();
        if (Pawn)
        {
            DropLocation = Pawn->GetActorLocation()
                + Pawn->GetActorForwardVector() * 100.0f
                + FVector(0, 0, 50.0f);
            DropRotation = Pawn->GetActorRotation();
        }
    }

    if (GetOwnerRole() == ROLE_Authority)
    {
        // Server 直接执行，使用计算好的位置
        Server_DropItemWithLocation_Implementation(Index, Quantity, DropLocation, DropRotation);
    }
    else
    {
        // Client 发送 RPC，带上计算好的位置
        Server_DropItemWithLocation(Index, Quantity, DropLocation, DropRotation);
    }
}

// ========== Server RPC 实现 ==========

void UInventoryComponent::Server_AddItem_Implementation(FName ItemID, int32 Quantity)
{
    if (!CanAddItem(ItemID, Quantity)) return;

    if (UInventorySubsystem* Subsystem = GetInventorySubsystem())
    {
        FItemStaticData Data;
        if (!Subsystem->GetItemData(ItemID, Data)) return;

        int32 ExistingIndex = FindItemIndex(ItemID);

        if (ExistingIndex != -1 && Data.MaxStackSize > 1)
        {
            // 堆叠到已有物品
            FInventoryEntry& Entry = InventoryItems[ExistingIndex];
            Entry.Quantity += Quantity;
        }
        else
        {
            // 新建条目
            FInventoryEntry NewEntry;
            NewEntry.ItemID = ItemID;
            NewEntry.Quantity = Quantity;
            NewEntry.SlotIndex = InventoryItems.Num();
            InventoryItems.Add(NewEntry);
        }

        // 更新负重
        CurrentWeight += Data.Weight * Quantity;

        BroadcastUpdate();
    }
}

bool UInventoryComponent::Server_AddItem_Validate(FName ItemID, int32 Quantity)
{
    return Quantity > 0 && !ItemID.IsNone();
}

void UInventoryComponent::Server_RemoveItemAtIndex_Implementation(int32 Index, int32 Quantity)
{
    if (!InventoryItems.IsValidIndex(Index)) return;
    if (Quantity <= 0) return;

    FInventoryEntry& Entry = InventoryItems[Index];

    if (UInventorySubsystem* Subsystem = GetInventorySubsystem())
    {
        FItemStaticData Data;
        if (Subsystem->GetItemData(Entry.ItemID, Data))
        {
            int32 RemoveQty = FMath::Min(Quantity, Entry.Quantity);
            Entry.Quantity -= RemoveQty;
            CurrentWeight -= Data.Weight * RemoveQty;

            if (Entry.Quantity <= 0)
            {
                InventoryItems.RemoveAt(Index);
                // 重新计算SlotIndex
                for (int32 i = 0; i < InventoryItems.Num(); ++i)
                {
                    InventoryItems[i].SlotIndex = i;
                }
            }

            BroadcastUpdate();
        }
    }
}

bool UInventoryComponent::Server_RemoveItemAtIndex_Validate(int32 Index, int32 Quantity)
{
    return Index >= 0 && Quantity > 0;
}

void UInventoryComponent::Server_MoveItem_Implementation(int32 FromIndex, int32 ToIndex)
{
    if (!InventoryItems.IsValidIndex(FromIndex)) return;
    if (!InventoryItems.IsValidIndex(ToIndex)) return;
    if (FromIndex == ToIndex) return;

    // 交换位置
    InventoryItems.Swap(FromIndex, ToIndex);

    // 更新SlotIndex
    InventoryItems[FromIndex].SlotIndex = FromIndex;
    InventoryItems[ToIndex].SlotIndex = ToIndex;

    BroadcastUpdate();
}

bool UInventoryComponent::Server_MoveItem_Validate(int32 FromIndex, int32 ToIndex)
{
    return FromIndex >= 0 && ToIndex >= 0;
}

void UInventoryComponent::Server_UseItem_Implementation(int32 Index)
{
    if (!InventoryItems.IsValidIndex(Index)) return;

    FInventoryEntry& Entry = InventoryItems[Index];

    if (UInventorySubsystem* Subsystem = GetInventorySubsystem())
    {
        FItemStaticData Data;
        if (Subsystem->GetItemData(Entry.ItemID, Data))
        {
            if (!Data.bCanUse) return;

            // TODO: 在这里实现使用效果
            // 例如：回血、装备武器等
            UE_LOG(LogTemp, Log, TEXT("Use Item: %s"), *Data.ItemName.ToString());

            // 使用成功后消耗一个
            Server_RemoveItemAtIndex_Implementation(Index, 1);
        }
    }
}

bool UInventoryComponent::Server_UseItem_Validate(int32 Index)
{
    return Index >= 0;
}

void UInventoryComponent::Server_DropItemWithLocation_Implementation(int32 Index, int32 Quantity, FVector DropLocation, FRotator DropRotation)
{
    // 检查索引是否有效
    if (!InventoryItems.IsValidIndex(Index))
    {
        UE_LOG(LogTemp, Warning, TEXT("DropItem: Invalid index %d"), Index);
        return;
    }

    if (Quantity <= 0) return;

    // 获取要丢弃的物品信息
    FInventoryEntry& Entry = InventoryItems[Index];
    int32 DropQty = FMath::Min(Quantity, Entry.Quantity);
    FName DropItemID = Entry.ItemID;

    // 获取物品数据（为了计算重量和生成）
    if (UInventorySubsystem* Subsystem = GetInventorySubsystem())
    {
        FItemStaticData Data;
        if (Subsystem->GetItemData(Entry.ItemID, Data))
        {
            // 1. 从背包移除物品
            Entry.Quantity -= DropQty;
            CurrentWeight -= Data.Weight * DropQty;

            // 如果数量归零，删除这个条目
            if (Entry.Quantity <= 0)
            {
                InventoryItems.RemoveAt(Index);
                // 重新计算所有条目的SlotIndex
                for (int32 i = 0; i < InventoryItems.Num(); ++i)
                {
                    InventoryItems[i].SlotIndex = i;
                }
            }

            // 2. 广播更新（刷新UI）
            BroadcastUpdate();

            // 3. 在指定位置生成掉落的物品（使用客户端传来的位置）
            FActorSpawnParameters SpawnParams;
            SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

            // 在服务端生成 PickupItem
            if (APickupItem* DroppedItem = GetWorld()->SpawnActor<APickupItem>(APickupItem::StaticClass(), DropLocation, DropRotation, SpawnParams))
            {
                // 初始化物品（设置ID和数量）
                DroppedItem->InitializePickup(DropItemID, DropQty);

                UE_LOG(LogTemp, Log, TEXT("Drop Item: %s x%d at location %s"),
                    *DropItemID.ToString(),
                    DropQty,
                    *DropLocation.ToString());
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to generate dropped items!"));
            }
        }
    }
}

bool UInventoryComponent::Server_DropItemWithLocation_Validate(int32 Index, int32 Quantity, FVector DropLocation, FRotator DropRotation)
{
    return Index >= 0 && Quantity > 0;
}

// ========== 同步回调 ==========

void UInventoryComponent::OnRep_InventoryItems()
{
    // Client收到同步数据，触发UI更新
    BroadcastUpdate();
}

// ========== 辅助函数 ==========

int32 UInventoryComponent::FindItemIndex(FName ItemID) const
{
    for (int32 i = 0; i < InventoryItems.Num(); ++i)
    {
        if (InventoryItems[i].ItemID == ItemID)
        {
            return i;
        }
    }
    return -1;
}

UInventorySubsystem* UInventoryComponent::GetInventorySubsystem() const
{
    if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
    {
        return GameInstance->GetSubsystem<UInventorySubsystem>();
    }
    return nullptr;
}

void UInventoryComponent::BroadcastUpdate()
{
    // 触发蓝图事件
    OnInventoryUpdated.Broadcast();

    // 调试打印
    PrintInventory();
}

// ========== 调试 ==========

void UInventoryComponent::PrintInventory() const
{
    UE_LOG(LogTemp, Log, TEXT("=== backpack contents [%d/%d] Weight: %.1f/%.1f ==="),
        InventoryItems.Num(), MaxSlots, CurrentWeight, MaxWeight);

    for (const FInventoryEntry& Entry : InventoryItems)
    {
        if (UInventorySubsystem* Subsystem = GetInventorySubsystem())
        {
            FItemStaticData Data;
            if (Subsystem->GetItemData(Entry.ItemID, Data))
            {
                UE_LOG(LogTemp, Log, TEXT("  [%d] %s x%d (%.1fkg)"),
                    Entry.SlotIndex,
                    *Data.ItemName.ToString(),
                    Entry.Quantity,
                    Data.Weight * Entry.Quantity);
            }
        }
    }
}

void UInventoryComponent::RequestEquipSlot(int32 SlotIndex)
{
    if (GetOwnerRole() == ROLE_Authority)
    {
        Server_EquipSlot_Implementation(SlotIndex);
        return;
    }

    // ========== 客户端乐观更新（修复版）==========
    FName PredictedItemID = NAME_None;
    int32 PredictedSlotIndex = SlotIndex;

    // 关键修复：先判断是否是Toggle卸载（再点一次同一槽位 = 卸下）
    if (SlotIndex == EquippedSlotIndex)
    {
        // 再次点击已装备的槽位 = 卸下（服务器逻辑如此，客户端要同步预测）
        PredictedItemID = NAME_None;
        PredictedSlotIndex = -1; // 预测服务器会设为-1
        UE_LOG(LogTemp, Log, TEXT("[Client Optimistic] Toggle Unequip slot %d"), SlotIndex);
    }
    else if (SlotIndex >= 0 && SlotIndex < MaxSlots && InventoryItems.IsValidIndex(SlotIndex))
    {
        // 装备新槽位
        PredictedItemID = InventoryItems[SlotIndex].ItemID;
        UE_LOG(LogTemp, Log, TEXT("[Client Optimistic] Equip slot %d (%s)"), SlotIndex, *PredictedItemID.ToString());
    }
    else if (SlotIndex == -1)
    {
        // 直接请求卸下（通过UI的"卸下"按钮）
        PredictedItemID = NAME_None;
        UE_LOG(LogTemp, Log, TEXT("[Client Optimistic] Direct Unequip"));
    }
    else
    {
        // 无效槽位，不预测，等服务器返回
        UE_LOG(LogTemp, Warning, TEXT("[Client Optimistic] Invalid slot %d, skip prediction"), SlotIndex);
    }

    // 立即本地广播，让Character马上更新武器显示（无论是装备还是卸载）
    if (SlotIndex >= -1 && SlotIndex < MaxSlots)
    {
        OnEquippedSlotChanged.Broadcast(PredictedSlotIndex, PredictedItemID);
    }
    // ============================================

    // 通过Character发送RPC（保持现有逻辑）
    UWorld* World = GetWorld();
    if (!World) return;
    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC) return;
    APawn* Pawn = PC->GetPawn();
    if (!Pawn) return;
    ACoopAdventureCharacter* Char = Cast<ACoopAdventureCharacter>(Pawn);
    if (!Char) return;
    Char->Server_RequestEquipSlot(SlotIndex);
}

void UInventoryComponent::Server_EquipSlot_Implementation(int32 SlotIndex)
{
    // 【安全验证】槽位必须在有效范围：-1（卸下）到 MaxSlots-1
    if (SlotIndex < -1 || SlotIndex >= MaxSlots)
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid slot index: %d"), SlotIndex);
        return;
    }

    // 【Toggle逻辑】如果点击的是已装备的槽位，则卸下（再点一次取消装备）
    if (EquippedSlotIndex == SlotIndex)
    {
        EquippedSlotIndex = -1;  // 卸下
        UE_LOG(LogTemp, Log, TEXT("Server: Unequipped"));
    }
    else
    {
        // 【验证】如果要装备新槽位，检查该槽位是否有物品
        if (SlotIndex >= 0 && !InventoryItems.IsValidIndex(SlotIndex))
        {
            UE_LOG(LogTemp, Warning, TEXT("Equip failed: Slot %d is empty"), SlotIndex);
            return;
        }

        // 【执行装备】
        EquippedSlotIndex = SlotIndex;

        if (EquippedSlotIndex >= 0)
        {
            FName ItemID = InventoryItems[EquippedSlotIndex].ItemID;
            UE_LOG(LogTemp, Log, TEXT("Server: Equipped slot %d (%s)"), EquippedSlotIndex, *ItemID.ToString());
        }
    }

    // 广播装备变化通知（让Character知道该换手了）
    FName EquippedItemID = GetEquippedItemID();
    OnEquippedSlotChanged.Broadcast(EquippedSlotIndex, EquippedItemID);

    // 通知UI刷新（广播背包更新事件）
    OnInventoryUpdated.Broadcast();
}

void UInventoryComponent::OnRep_EquippedSlot()
{
    // 客户端收到服务器同步
    UE_LOG(LogTemp, Log, TEXT("Client: Equipped slot changed to %d"), EquippedSlotIndex);

    // 【关键】客户端也要广播委托，让Character更新手上模型！
    FName EquippedItemID = GetEquippedItemID();
    OnEquippedSlotChanged.Broadcast(EquippedSlotIndex, EquippedItemID);

    OnInventoryUpdated.Broadcast();
}

bool UInventoryComponent::Server_EquipSlot_Validate(int32 SlotIndex)
{
    // 简单验证：槽位必须在 -1（卸下）到 MaxSlots-1 之间
    return SlotIndex >= -1 && SlotIndex < MaxSlots;
}

// 【新增】获取当前装备的物品ID
FName UInventoryComponent::GetEquippedItemID() const
{
    // 如果装备了物品（槽位索引 >= 0），返回该槽位的物品ID
    if (EquippedSlotIndex >= 0 && InventoryItems.IsValidIndex(EquippedSlotIndex))
    {
        return InventoryItems[EquippedSlotIndex].ItemID;
    }

    // 否则返回 None（空）
    return NAME_None;
}

void UInventoryComponent::ExecuteEquipSlot(int32 SlotIndex)
{
    // 在服务器上直接执行，在客户端通过 RPC
    if (GetOwnerRole() == ROLE_Authority)
    {
        Server_EquipSlot_Implementation(SlotIndex);
    }
    else
    {
        Server_EquipSlot(SlotIndex);
    }
}

void UInventoryComponent::DropAllItems()
{
    if (GetOwnerRole() != ROLE_Authority) return;

    FVector DropLocation = FVector::ZeroVector;
    FRotator DropRotation = FRotator::ZeroRotator;
    if (APlayerState* PS = Cast<APlayerState>(GetOwner()))
    {
        if (APawn* Pawn = PS->GetPawn())
        {
            DropLocation = Pawn->GetActorLocation() + Pawn->GetActorForwardVector() * 50.0f + FVector(0, 0, 50.0f);
            DropRotation = Pawn->GetActorRotation();
        }
    }

    if (EquippedSlotIndex != -1)
    {
        EquippedSlotIndex = -1;
        OnEquippedSlotChanged.Broadcast(-1, NAME_None);
    }

    // 从后往前遍历丢弃
    for (int32 i = InventoryItems.Num() - 1; i >= 0; i--)
    {
        const FInventoryEntry& Entry = InventoryItems[i];
        Server_DropItemWithLocation_Implementation(i, Entry.Quantity, DropLocation, DropRotation);
    }

    // 确保背包清空（Server_DropItemWithLocation_Implementation 已修改 InventoryItems）
    InventoryItems.Empty();
    CurrentWeight = 0.0f;
    BroadcastUpdate();
}