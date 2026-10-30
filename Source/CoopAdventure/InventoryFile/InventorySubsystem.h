// InventorySubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ItemData.h"
#include "Engine/StreamableManager.h"
#include "InventorySubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryDataLoaded);

UCLASS()
class COOPADVENTURE_API UInventorySubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /** 数据表是否已加载并预完成资源预加载 */
    UFUNCTION(BlueprintPure, Category = "Inventory|System")
    bool IsInitialized() const { return bInitialized; }

    /** 是否正在加载中（用于显示加载动画） */
    UFUNCTION(BlueprintPure, Category = "Inventory|System")
    bool IsLoading() const { return bIsLoading; }

    /** 查询物品静态数据（数据表已加载时才返回 true） */
    UFUNCTION(BlueprintPure, Category = "Inventory|Query", meta = (AutoCreateRefTerm = "ItemID"))
    bool GetItemData(FName ItemID, FItemStaticData& OutData) const;

    /** 检查物品是否存在（数据表已加载时才可靠） */
    UFUNCTION(BlueprintPure, Category = "Inventory|Query", meta = (AutoCreateRefTerm = "ItemID"))
    bool HasItemData(FName ItemID) const;

    /** 获取所有物品ID（调试用） */
    UFUNCTION(BlueprintPure, Category = "Inventory|Debug")
    TArray<FName> GetAllItemIDs() const;

    /** 按类型筛选物品ID */
    UFUNCTION(BlueprintPure, Category = "Inventory|Query")
    TArray<FName> GetItemsByType(EItemType ItemType) const;

    /** 强制重新加载数据表（调试用） */
    UFUNCTION(BlueprintCallable, Category = "Inventory|Debug")
    void ReloadDataTable();

    /** 数据加载完成事件（包括资源预加载） */
    UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
    FOnInventoryDataLoaded OnDataLoaded;

protected:
    // ===== 数据表加载 =====
    void StartAsyncLoad();
    void OnDataTableLoaded();
    void ParseDataTable(UDataTable* DataTable);

    // ===== 资源预加载 =====
    void StartAsyncAssetPreload();
    void OnAssetsPreloaded();

    /** 辅助模板：将有效的软引用添加到预加载列表 */
    template<typename T>
    void AddAssetToPreload(const TSoftObjectPtr<T>& SoftObject)
    {
        if (!SoftObject.IsNull())
        {
            AssetsToPreload.Add(SoftObject.ToSoftObjectPath());
        }
    }

    /** 辅助模板：遍历数组，添加所有软引用 */
    template<typename T>
    void AddAssetArrayToPreload(const TArray<TSoftObjectPtr<T>>& SoftObjectArray)
    {
        for (const TSoftObjectPtr<T>& SoftObject : SoftObjectArray)
        {
            AddAssetToPreload(SoftObject);
        }
    }

protected:
    /** 数据表路径（可在蓝图默认值中配置） */
    UPROPERTY(EditDefaultsOnly, Category = "Config")
    TSoftObjectPtr<UDataTable> ItemDataTablePath;

    /** 缓存的物品数据（ID -> 静态数据） */
    TMap<FName, FItemStaticData> ItemDataCache;

    /** 初始化状态标志 */
    bool bInitialized = false;
    bool bIsLoading = false;

    /** 数据表异步加载句柄 */
    TSharedPtr<FStreamableHandle> CurrentStreamableHandle;

    /** 资源预加载句柄 */
    TSharedPtr<FStreamableHandle> AssetPreloadHandle;

    /** 需要预加载的资源路径列表 */
    TArray<FSoftObjectPath> AssetsToPreload;
};