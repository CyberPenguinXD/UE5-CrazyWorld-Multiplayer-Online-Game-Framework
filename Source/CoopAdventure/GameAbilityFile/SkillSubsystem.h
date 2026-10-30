#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/StreamableManager.h"
#include "SkillTypes.h"
#include "SkillData.h"
#include "SkillSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSkillDataLoaded);

UCLASS()
class COOPADVENTURE_API USkillSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // 查询接口
    UFUNCTION(BlueprintPure, Category = "Skill|Query")
    bool IsInitialized() const { return bInitialized; }

    UFUNCTION(BlueprintPure, Category = "Skill|Query")
    bool GetSkillData(FName SkillID, FSkillStaticData& OutData) const;

    UFUNCTION(BlueprintPure, Category = "Skill|Query")
    bool HasSkillData(FName SkillID) const;

    UFUNCTION(BlueprintPure, Category = "Skill|Query")
    TArray<FName> GetAllSkillIDs() const;

    // 按绑定类型筛选
    UFUNCTION(BlueprintPure, Category = "Skill|Query")
    TArray<FName> GetSkillsByBindingType(ESkillBindingType BindingType) const;

    // 事件
    UPROPERTY(BlueprintAssignable, Category = "Skill|Events")
    FOnSkillDataLoaded OnDataLoaded;

    // 强制重新加载
    UFUNCTION(BlueprintCallable, Category = "Skill|Debug")
    void ReloadDataTable();

    // ========== 新增：资源加载接口 ==========

    // 强制同步加载特定技能的所有资源（用于关键技能，确保立即可用）
    UFUNCTION(BlueprintCallable, Category = "Skill|Assets")
    void ForceLoadSkillAssets(FName SkillID);

protected:
    void StartAsyncLoad();
    void OnDataTableLoaded();
    void ParseDataTable(class UDataTable* DataTable);

    // ========== 新增：资源预加载 ==========

    // 异步预加载所有技能资源
    void StartAsyncAssetPreload();

    // 预加载完成回调
    void OnSkillAssetsPreloaded();

    // 存储需要预加载的资源路径
    TArray<FSoftObjectPath> SkillAssetsToPreload;

    // 预加载句柄（防止被垃圾回收）
    TSharedPtr<FStreamableHandle> SkillAssetPreloadHandle;

    UPROPERTY(EditDefaultsOnly, Category = "Config")
    TSoftObjectPtr<UDataTable> SkillDataTablePath;

    TMap<FName, FSkillStaticData> SkillDataCache;

    bool bInitialized = false;
    bool bIsLoading = false;
    TSharedPtr<FStreamableHandle> CurrentStreamableHandle;

protected:
    template<typename T>
    void AddAssetToPreload(const TSoftObjectPtr<T>& SoftObject)
    {
        if (!SoftObject.IsNull())
        {
            SkillAssetsToPreload.Add(SoftObject.ToSoftObjectPath());
        }
    }
};