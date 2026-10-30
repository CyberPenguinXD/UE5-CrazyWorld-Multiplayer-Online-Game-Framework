#include "SkillSubsystem.h"
#include "Engine/DataTable.h"
#include "Engine/AssetManager.h"
#include "SkillData.h"

void USkillSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    bInitialized = false;
    bIsLoading = false;

    if (SkillDataTablePath.ToString().IsEmpty())
    {
        SkillDataTablePath = FSoftObjectPath(TEXT("/Game/MainFile/Skills/DT_Skills.DT_Skills"));
    }

    StartAsyncLoad();
}

void USkillSubsystem::Deinitialize()
{
    // 取消所有进行中的异步加载
    if (CurrentStreamableHandle.IsValid())
    {
        CurrentStreamableHandle->CancelHandle();
        CurrentStreamableHandle.Reset();
    }

    if (SkillAssetPreloadHandle.IsValid())
    {
        SkillAssetPreloadHandle->CancelHandle();
        SkillAssetPreloadHandle.Reset();
    }

    SkillDataCache.Empty();
    SkillAssetsToPreload.Empty();

    Super::Deinitialize();
}

void USkillSubsystem::StartAsyncLoad()
{
    if (bIsLoading || bInitialized) return;

    FString PathString = SkillDataTablePath.ToString();
    if (PathString.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("SkillSubsystem: DataTable path is empty!"));
        return;
    }

#if WITH_EDITOR
    // 编辑器模式下强制同步加载（避免异步问题）
    UDataTable* DataTable = SkillDataTablePath.LoadSynchronous();
    if (DataTable)
    {
        ParseDataTable(DataTable);
        return;
    }
#endif

    bIsLoading = true;

    FStreamableDelegate OnLoaded;
    OnLoaded.BindUObject(this, &USkillSubsystem::OnDataTableLoaded);

    CurrentStreamableHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
        SkillDataTablePath.ToSoftObjectPath(),
        OnLoaded
    );

    if (CurrentStreamableHandle.IsValid() && CurrentStreamableHandle->HasLoadCompleted())
    {
        OnDataTableLoaded();
    }
}

void USkillSubsystem::OnDataTableLoaded()
{
    UDataTable* DataTable = nullptr;
    if (CurrentStreamableHandle.IsValid())
    {
        DataTable = Cast<UDataTable>(CurrentStreamableHandle->GetLoadedAsset());
        CurrentStreamableHandle.Reset();
    }

    // 降级同步加载
    if (!DataTable)
    {
        DataTable = SkillDataTablePath.LoadSynchronous();
    }

    if (DataTable)
    {
        ParseDataTable(DataTable);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("SkillSubsystem: Failed to load DataTable"));
        bIsLoading = false;
        OnDataLoaded.Broadcast();
    }
}

void USkillSubsystem::ParseDataTable(UDataTable* DataTable)
{
    if (!DataTable)
    {
        bIsLoading = false;
        OnDataLoaded.Broadcast();
        return;
    }

    SkillDataCache.Empty();
    SkillAssetsToPreload.Empty();

    TArray<FName> RowNames = DataTable->GetRowNames();
    static const FString ContextString(TEXT("SkillSubsystem_LoadData"));

    for (const FName& RowName : RowNames)
    {
        FSkillStaticData* RowData = DataTable->FindRow<FSkillStaticData>(RowName, ContextString);
        if (RowData)
        {
            // 确保SkillID与RowName一致
            RowData->SkillID = RowName;
            SkillDataCache.Add(RowName, *RowData);

            // ========== 新增：收集所有软引用资源路径 ==========
            if (RowData->CastEffect.IsValid())
                SkillAssetsToPreload.Add(RowData->CastEffect.ToSoftObjectPath());
            if (RowData->ProjectileEffect.IsValid())
                SkillAssetsToPreload.Add(RowData->ProjectileEffect.ToSoftObjectPath());
            if (RowData->ImpactEffect.IsValid())
                SkillAssetsToPreload.Add(RowData->ImpactEffect.ToSoftObjectPath());
            if (RowData->BuffEffect.IsValid())
                SkillAssetsToPreload.Add(RowData->BuffEffect.ToSoftObjectPath());
            if (RowData->Icon.IsValid())
                SkillAssetsToPreload.Add(RowData->Icon.ToSoftObjectPath());

            // 音效
            if (RowData->CastSound.IsValid())
                SkillAssetsToPreload.Add(RowData->CastSound.ToSoftObjectPath());
            if (RowData->ProjectileSound.IsValid())
                SkillAssetsToPreload.Add(RowData->ProjectileSound.ToSoftObjectPath());
            if (RowData->ImpactSound.IsValid())
                SkillAssetsToPreload.Add(RowData->ImpactSound.ToSoftObjectPath());
            if (RowData->ActivationSound.IsValid())
                SkillAssetsToPreload.Add(RowData->ActivationSound.ToSoftObjectPath());
        }

        AddAssetToPreload(RowData->Icon);
        AddAssetToPreload(RowData->CastEffect);
        AddAssetToPreload(RowData->ProjectileEffect);
        AddAssetToPreload(RowData->ImpactEffect);
        AddAssetToPreload(RowData->BuffEffect);
        AddAssetToPreload(RowData->CastSound);
        AddAssetToPreload(RowData->ProjectileSound);
        AddAssetToPreload(RowData->ImpactSound);
        AddAssetToPreload(RowData->ActivationSound);
    }

    UE_LOG(LogTemp, Log, TEXT("SkillSubsystem: Parsed %d skills, found %d assets to preload"),
        SkillDataCache.Num(), SkillAssetsToPreload.Num());

    // 启动资源预加载（而不是直接标记初始化完成）
    StartAsyncAssetPreload();
}

void USkillSubsystem::StartAsyncAssetPreload()
{
    if (SkillAssetsToPreload.Num() == 0)
    {
        // 没有资源需要预加载，直接完成
        bInitialized = true;
        bIsLoading = false;
        OnDataLoaded.Broadcast();
        return;
    }

    // 异步预加载所有收集到的资源
    FStreamableDelegate OnLoaded;
    OnLoaded.BindUObject(this, &USkillSubsystem::OnSkillAssetsPreloaded);

    SkillAssetPreloadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
        SkillAssetsToPreload,
        OnLoaded,
        FStreamableManager::AsyncLoadHighPriority  // 高优先级，但仍是异步
    );

    // 超时保护：如果10秒内没加载完，强制标记完成
    FTimerHandle TimeoutHandle;
    GetWorld()->GetTimerManager().SetTimer(TimeoutHandle, [this]()
        {
            if (!bInitialized && SkillAssetPreloadHandle.IsValid())
            {
                UE_LOG(LogTemp, Warning, TEXT("SkillSubsystem: Asset preload timeout, forcing completion"));
                SkillAssetPreloadHandle.Reset();
                bInitialized = true;
                bIsLoading = false;
                OnDataLoaded.Broadcast();
            }
        }, 10.0f, false);
}

void USkillSubsystem::OnSkillAssetsPreloaded()
{
    if (SkillAssetPreloadHandle.IsValid())
    {
        TArray<UObject*> LoadedAssets;
        SkillAssetPreloadHandle->GetLoadedAssets(LoadedAssets);
        UE_LOG(LogTemp, Log, TEXT("SkillSubsystem: Successfully preloaded %d assets"), LoadedAssets.Num());
        SkillAssetPreloadHandle.Reset();
    }

    bInitialized = true;
    bIsLoading = false;
    OnDataLoaded.Broadcast();
}

void USkillSubsystem::ForceLoadSkillAssets(FName SkillID)
{
    FSkillStaticData Data;
    if (!GetSkillData(SkillID, Data)) return;

    // 同步加载该技能的所有资源（阻塞式，确保立即可用）
    if (Data.CastEffect.IsValid()) Data.CastEffect.LoadSynchronous();
    if (Data.ProjectileEffect.IsValid()) Data.ProjectileEffect.LoadSynchronous();
    if (Data.ImpactEffect.IsValid()) Data.ImpactEffect.LoadSynchronous();
    if (Data.BuffEffect.IsValid()) Data.BuffEffect.LoadSynchronous();
    if (Data.Icon.IsValid()) Data.Icon.LoadSynchronous();

    if (Data.CastSound.IsValid()) Data.CastSound.LoadSynchronous();
    if (Data.ProjectileSound.IsValid()) Data.ProjectileSound.LoadSynchronous();
    if (Data.ImpactSound.IsValid()) Data.ImpactSound.LoadSynchronous();
    if (Data.ActivationSound.IsValid()) Data.ActivationSound.LoadSynchronous();

    UE_LOG(LogTemp, Log, TEXT("SkillSubsystem: Force loaded all assets for skill %s"), *SkillID.ToString());
}

bool USkillSubsystem::GetSkillData(FName SkillID, FSkillStaticData& OutData) const
{
    if (bInitialized)
    {
        const FSkillStaticData* FoundData = SkillDataCache.Find(SkillID);
        if (FoundData)
        {
            OutData = *FoundData;
            return true;
        }
        return false;
    }

    if (bIsLoading)
    {
        return false;
    }

    const_cast<USkillSubsystem*>(this)->StartAsyncLoad();
    return false;
}

bool USkillSubsystem::HasSkillData(FName SkillID) const
{
    return bInitialized && SkillDataCache.Contains(SkillID);
}

TArray<FName> USkillSubsystem::GetAllSkillIDs() const
{
    TArray<FName> IDs;
    if (bInitialized)
    {
        SkillDataCache.GetKeys(IDs);
    }
    return IDs;
}

TArray<FName> USkillSubsystem::GetSkillsByBindingType(ESkillBindingType BindingType) const
{
    TArray<FName> Result;
    if (!bInitialized) return Result;

    for (const auto& Pair : SkillDataCache)
    {
        if (Pair.Value.BindingType == BindingType)
        {
            Result.Add(Pair.Key);
        }
    }
    return Result;
}

void USkillSubsystem::ReloadDataTable()
{
    bInitialized = false;
    bIsLoading = false;
    SkillDataCache.Empty();
    SkillAssetsToPreload.Empty();

    if (SkillAssetPreloadHandle.IsValid())
    {
        SkillAssetPreloadHandle->CancelHandle();
        SkillAssetPreloadHandle.Reset();
    }

    StartAsyncLoad();
}