// InventoryComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemData.h"
#include "InventoryComponent.generated.h"

// 委托声明：背包变化时广播
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryUpdated);

// 【新增】装备变化委托：参数是（槽位索引，物品ID）
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEquippedSlotChanged, int32, SlotIndex, FName, ItemID);

UCLASS(ClassGroup = (Inventory), meta = (BlueprintSpawnableComponent))
class COOPADVENTURE_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

	// 初始化
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ========== 查询接口（Client/Server 都能调用）==========

	// 获取所有物品
	UFUNCTION(BlueprintPure, Category = "Inventory")
	const TArray<FInventoryEntry>& GetInventoryItems() const { return InventoryItems; }

	// 获取物品数量
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetItemQuantity(FName ItemID) const;

	// 检查能否添加物品
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool CanAddItem(FName ItemID, int32 Quantity = 1) const;

	// 检查背包是否已满
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsFull() const { return InventoryItems.Num() >= MaxSlots; }

	// 【新增】获取当前装备的物品ID（None表示空手）
	UFUNCTION(BlueprintPure, Category = "Equipment")
	FName GetEquippedItemID() const;

	// ========== 操作接口（自动路由到 Server）==========

	// 添加物品（入口）
	UFUNCTION(BlueprintCallable, Category = "Inventory", meta = (AutoCreateRefTerm = "ItemID"))
	void AddItem(FName ItemID, int32 Quantity = 1);

	// 移除物品（入口）
	UFUNCTION(BlueprintCallable, Category = "Inventory", meta = (AutoCreateRefTerm = "ItemID"))
	void RemoveItem(FName ItemID, int32 Quantity = 1);

	// 按索引移除（用于 UI 点击删除）
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void RemoveItemAtIndex(int32 Index, int32 Quantity = 1);

	// 移动物品（排序）
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void MoveItem(int32 FromIndex, int32 ToIndex);

	// 使用物品
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void UseItem(int32 Index);

	// 丢弃物品（扔地上）
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void DropItem(int32 Index, int32 Quantity = 1);

	// ========== 事件委托（Blueprint 可绑定）==========

	// 背包变化时触发（用于刷新 UI）
	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnInventoryUpdated OnInventoryUpdated;

	// 【新增】装备变化时广播（槽位索引，当前装备的物品ID）
	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnEquippedSlotChanged OnEquippedSlotChanged;

	// ========== 手持装备系统 ==========

	// 【查询】当前装备的背包槽位（-1=空手，0~19=装备对应格子）
	UFUNCTION(BlueprintPure, Category = "Equipment")
	int32 GetEquippedSlotIndex() const { return EquippedSlotIndex; }

	// 【客户端调用】点击背包按钮时调用这个（参数：背包格子索引）
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void RequestEquipSlot(int32 SlotIndex);

protected:
	// ========== 配置属性 ==========

	// 最大格子数
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	int32 MaxSlots;

	// 最大负重（0 = 无限）
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float MaxWeight;

	// ========== 运行时数据（Replicated）==========

	// 背包物品数组（核心数据）
	UPROPERTY(ReplicatedUsing = OnRep_InventoryItems)
	TArray<FInventoryEntry> InventoryItems;

	// 当前负重
	UPROPERTY(Replicated)
	float CurrentWeight;

	// 【新增】当前装备的背包槽位索引（-1表示未装备任何物品）
	UPROPERTY(ReplicatedUsing = OnRep_EquippedSlot)
	int32 EquippedSlotIndex;

	// ========== Server RPC（权威操作）==========

	// Server：执行添加
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_AddItem(FName ItemID, int32 Quantity);
	void Server_AddItem_Implementation(FName ItemID, int32 Quantity);
	bool Server_AddItem_Validate(FName ItemID, int32 Quantity);

	// Server：执行移除
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RemoveItemAtIndex(int32 Index, int32 Quantity);
	void Server_RemoveItemAtIndex_Implementation(int32 Index, int32 Quantity);
	bool Server_RemoveItemAtIndex_Validate(int32 Index, int32 Quantity);

	// Server：执行移动
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_MoveItem(int32 FromIndex, int32 ToIndex);
	void Server_MoveItem_Implementation(int32 FromIndex, int32 ToIndex);
	bool Server_MoveItem_Validate(int32 FromIndex, int32 ToIndex);

	// Server：执行使用
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_UseItem(int32 Index);
	void Server_UseItem_Implementation(int32 Index);
	bool Server_UseItem_Validate(int32 Index);

	// Server：执行丢弃（带位置）
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_DropItemWithLocation(int32 Index, int32 Quantity, FVector DropLocation, FRotator DropRotation);
	void Server_DropItemWithLocation_Implementation(int32 Index, int32 Quantity, FVector DropLocation, FRotator DropRotation);
	bool Server_DropItemWithLocation_Validate(int32 Index, int32 Quantity, FVector DropLocation, FRotator DropRotation);

	// 【新增】Server：执行装备/卸下
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_EquipSlot(int32 SlotIndex);
	void Server_EquipSlot_Implementation(int32 SlotIndex);
	bool Server_EquipSlot_Validate(int32 SlotIndex);

	// ========== 同步回调 ==========

	// 背包数据同步到 Client 时调用
	UFUNCTION()
	void OnRep_InventoryItems();

	// 【新增】装备槽位同步到 Client 时调用
	UFUNCTION()
	void OnRep_EquippedSlot();

	// ========== 辅助函数 ==========

	// 查找物品索引（-1 = 不存在）
	int32 FindItemIndex(FName ItemID) const;

	// 获取子系统
	class UInventorySubsystem* GetInventorySubsystem() const;

	// 广播更新事件
	void BroadcastUpdate();

public:
	// 调试：打印背包内容
	UFUNCTION(BlueprintCallable, Category = "Inventory|Debug")
	void PrintInventory() const;

	// 供 Character 调用的公开接口（解决 RPC 所有权问题）
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void ExecuteEquipSlot(int32 SlotIndex);

public:
	// 新增：丢弃所有物品（死亡时调用）
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void DropAllItems();
};