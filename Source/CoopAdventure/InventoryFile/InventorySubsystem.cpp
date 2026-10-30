// InventorySubsystem.cpp
#include "InventorySubsystem.h"
#include "Engine/DataTable.h"
#include "Engine/AssetManager.h"
#include "NiagaraSystem.h"
#include "Sound/SoundCue.h"
#include "Sound/SoundAttenuation.h"
#include "CoopAdventure.h"

void UInventorySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    bInitialized = false;
    bIsLoading = false;

    UE_LOG(LogTemp, Log, TEXT("InventorySubsystem: Initializing..."));

    // 设置默认数据表路径（如果未在子类中配置）
    if (ItemDataTablePath.ToString().IsEmpty())
    {
        ItemDataTablePath = FSoftObjectPath(TEXT("/Game/MainFile/Inventory/DT_items.DT_items"));
        UE_LOG(LogTemp, Log, TEXT("InventorySubsystem: Using default path: %s"), *ItemDataTablePath.ToString());
    }

    StartAsyncLoad();
}

void UInventorySubsystem::Deinitialize()
{
    UE_LOG(LogTemp, Log, TEXT("InventorySubsystem: Deinitializing..."));

    // 取消所有异步加载
    if (CurrentStreamableHandle.IsValid())
    {
        CurrentStreamableHandle->CancelHandle();
        CurrentStreamableHandle.Reset();
    }

    if (AssetPreloadHandle.IsValid())
    {
        AssetPreloadHandle->CancelHandle();
        AssetPreloadHandle.Reset();
    }

    // 清空缓存
    ItemDataCache.Empty();
    AssetsToPreload.Empty();

    bInitialized = false;
    bIsLoading = false;

    Super::Deinitialize();
}

void UInventorySubsystem::StartAsyncLoad()
{
    if (bIsLoading || bInitialized)
    {
        return;
    }

    FString PathString = ItemDataTablePath.ToString();
    if (PathString.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("InventorySubsystem: DataTable path is empty!"));
        return;
    }

    static FString LastFailedPath;
    if (!LastFailedPath.IsEmpty() && PathString == LastFailedPath)
    {
        UE_LOG(LogTemp, Verbose, TEXT("InventorySubsystem: Skipping reload of previously failed path: %s"), *PathString);
        return;
    }

#if WITH_EDITOR
    // 编辑器模式同步加载（避免异步时序问题，也方便预览）
    UE_LOG(LogTemp, Log, TEXT("Editor mode: Synchronous loading DataTable"));
    UDataTable* DataTable = ItemDataTablePath.LoadSynchronous();
    if (DataTable)
    {
        ParseDataTable(DataTable);
        return;
    }
#endif

    bIsLoading = true;
    UE_LOG(LogTemp, Log, TEXT("InventorySubsystem: Starting async load of %s"), *PathString);

    FStreamableDelegate OnLoaded;
    OnLoaded.BindUObject(this, &UInventorySubsystem::OnDataTableLoaded);

    CurrentStreamableHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
        ItemDataTablePath.ToSoftObjectPath(),
        OnLoaded
    );

    // 如果已加载完成（例如缓存命中），立即回调
    if (CurrentStreamableHandle.IsValid() && CurrentStreamableHandle->HasLoadCompleted())
    {
        OnDataTableLoaded();
    }
}

void UInventorySubsystem::OnDataTableLoaded()
{
    UDataTable* DataTable = nullptr;
    if (CurrentStreamableHandle.IsValid())
    {
        DataTable = Cast<UDataTable>(CurrentStreamableHandle->GetLoadedAsset());
        CurrentStreamableHandle.Reset();
    }

    if (!DataTable)
    {
        // 降级同步加载（保险）
        DataTable = ItemDataTablePath.LoadSynchronous();
    }

    if (DataTable)
    {
        ParseDataTable(DataTable);
        // 清除失败记录
        static FString LastFailedPath;
        LastFailedPath.Empty();
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("InventorySubsystem: Failed to load DataTable from path: %s"),
            *ItemDataTablePath.ToString());

        static FString LastFailedPath;
        LastFailedPath = ItemDataTablePath.ToString();
        bIsLoading = false;
        OnDataLoaded.Broadcast(); // 即使失败也要广播，避免UI卡死
    }
}

void UInventorySubsystem::ParseDataTable(UDataTable* DataTable)
{
    if (!DataTable)
    {
        bIsLoading = false;
        OnDataLoaded.Broadcast();
        return;
    }

    ItemDataCache.Empty();
    AssetsToPreload.Empty(); // 清空之前的预加载列表

    TArray<FName> RowNames = DataTable->GetRowNames();
    static const FString ContextString(TEXT("InventorySubsystem_LoadData"));

    for (const FName& RowName : RowNames)
    {
        FItemStaticData* RowData = DataTable->FindRow<FItemStaticData>(RowName, ContextString);
        if (RowData)
        {
            ItemDataCache.Add(RowName, *RowData);

            // ========== 收集所有软引用资源 ==========
            // 基础视觉资源
            AddAssetToPreload(RowData->Icon);
            AddAssetToPreload(RowData->Mesh);
            AddAssetToPreload(RowData->HeldMesh);

            // 枪械特效
            AddAssetToPreload(RowData->MuzzleFlashEffect);
            AddAssetToPreload(RowData->TracerEffect);
            AddAssetToPreload(RowData->ImpactEffect_Default);
            AddAssetToPreload(RowData->ImpactDecalMaterial);

            // 近战特效
            AddAssetToPreload(RowData->MeleeTrailEffect);
            AddAssetToPreload(RowData->MeleeImpactEffect);

            // 音效数组
            AddAssetArrayToPreload(RowData->SwingSounds);
            AddAssetArrayToPreload(RowData->MuzzleSounds);

            // 单个音效
            AddAssetToPreload(RowData->ImpactSound_Hit);
            AddAssetToPreload(RowData->ImpactSound_Miss);
            AddAssetToPreload(RowData->MeleeImpactSound_Hit);
            AddAssetToPreload(RowData->MeleeImpactSound_Miss);

            // 衰减设置
            AddAssetToPreload(RowData->GunshotAttenuation);
            AddAssetToPreload(RowData->MeleeSwingAttenuation);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("InventorySubsystem: Parsed %d items, collected %d assets to preload"),
        ItemDataCache.Num(), AssetsToPreload.Num());

    // 启动资源预加载（异步）
    StartAsyncAssetPreload();
}

void UInventorySubsystem::StartAsyncAssetPreload()
{
    if (AssetsToPreload.Num() == 0)
    {
        // 没有资源需要预加载，直接完成
        UE_LOG(LogTemp, Log, TEXT("InventorySubsystem: No assets to preload, initialization complete."));
        bInitialized = true;
        bIsLoading = false;
        OnDataLoaded.Broadcast();
        return;
    }

    // 取消之前的预加载（如果存在）
    if (AssetPreloadHandle.IsValid())
    {
        AssetPreloadHandle->CancelHandle();
        AssetPreloadHandle.Reset();
    }

    FStreamableDelegate OnLoaded;
    OnLoaded.BindUObject(this, &UInventorySubsystem::OnAssetsPreloaded);

    UE_LOG(LogTemp, Log, TEXT("InventorySubsystem: Starting async preload of %d assets"), AssetsToPreload.Num());

    AssetPreloadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
        AssetsToPreload,
        OnLoaded,
        FStreamableManager::AsyncLoadHighPriority // 高优先级，尽快加载
    );

    // 超时保护：10秒后如果还未完成，强制标记初始化完成
    FTimerHandle TimeoutHandle;
    GetWorld()->GetTimerManager().SetTimer(TimeoutHandle, [this]()
        {
            if (!bInitialized && AssetPreloadHandle.IsValid())
            {
                UE_LOG(LogTemp, Warning, TEXT("InventorySubsystem: Asset preload timeout, forcing completion"));
                AssetPreloadHandle.Reset();
                bInitialized = true;
                bIsLoading = false;
                OnDataLoaded.Broadcast();
            }
        }, 10.0f, false);
}

void UInventorySubsystem::OnAssetsPreloaded()
{
    if (AssetPreloadHandle.IsValid())
    {
        TArray<UObject*> LoadedAssets;
        AssetPreloadHandle->GetLoadedAssets(LoadedAssets);
        UE_LOG(LogTemp, Log, TEXT("InventorySubsystem: Successfully preloaded %d assets"), LoadedAssets.Num());
        AssetPreloadHandle.Reset();
    }

    bInitialized = true;
    bIsLoading = false;
    OnDataLoaded.Broadcast();

    UE_LOG(LogTemp, Log, TEXT("InventorySubsystem: Initialization complete."));
}

bool UInventorySubsystem::GetItemData(FName ItemID, FItemStaticData& OutData) const
{
    if (!bInitialized)
    {
        // 如果未初始化，可以尝试触发加载（但不应阻塞）
        UE_LOG(LogTemp, Warning, TEXT("InventorySubsystem: GetItemData called before initialization (ID=%s). Data may be stale."), *ItemID.ToString());
        return false;
    }

    const FItemStaticData* FoundData = ItemDataCache.Find(ItemID);
    if (FoundData)
    {
        OutData = *FoundData;
        return true;
    }

    UE_LOG(LogTemp, Warning, TEXT("InventorySubsystem: Item not found: %s"), *ItemID.ToString());
    return false;
}

bool UInventorySubsystem::HasItemData(FName ItemID) const
{
    return bInitialized && ItemDataCache.Contains(ItemID);
}

TArray<FName> UInventorySubsystem::GetAllItemIDs() const
{
    TArray<FName> IDs;
    if (bInitialized)
    {
        ItemDataCache.GetKeys(IDs);
    }
    return IDs;
}

TArray<FName> UInventorySubsystem::GetItemsByType(EItemType ItemType) const
{
    TArray<FName> Result;
    if (!bInitialized) return Result;

    for (const auto& Pair : ItemDataCache)
    {
        if (Pair.Value.ItemType == ItemType)
        {
            Result.Add(Pair.Key);
        }
    }
    return Result;
}

void UInventorySubsystem::ReloadDataTable()
{
    UE_LOG(LogTemp, Log, TEXT("InventorySubsystem: Reloading Data Table..."));

    // 重置状态
    static FString LastFailedPath;
    LastFailedPath.Empty();

    bInitialized = false;
    bIsLoading = false;
    ItemDataCache.Empty();
    AssetsToPreload.Empty();

    if (AssetPreloadHandle.IsValid())
    {
        AssetPreloadHandle->CancelHandle();
        AssetPreloadHandle.Reset();
    }

    StartAsyncLoad();
}

