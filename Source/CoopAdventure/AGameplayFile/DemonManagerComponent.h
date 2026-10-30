// AGameplayFile/DemonManagerComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DemonManagerComponent.generated.h"

class ACoopAdventurePlayerState;
class UIdentityComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDemonChanged, ACoopAdventurePlayerState*, NewDemon, ACoopAdventurePlayerState*, PreviousDemon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNoDemonAvailable);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGameInitialized); // 新增：游戏正式开始事件

UCLASS(ClassGroup = (Gameplay), meta = (BlueprintSpawnableComponent))
class COOPADVENTURE_API UDemonManagerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UDemonManagerComponent();

    // ========== 核心触发接口（由区域调用）==========

    /**
     * 触发游戏正式开始（首次分配）
     * 由 TriggerZone/TriggerVolume 调用，当玩家进入特定区域时触发
     * 只会执行一次，之后调用无效
     *
     * @param TriggerPlayer - 触发此次调用的玩家（用于日志，可为nullptr）
     * @return 是否成功初始化（false表示已经初始化过或没有可用玩家）
     */
    UFUNCTION(BlueprintCallable, Category = "Demon Manager")
    bool TriggerGameStart(ACoopAdventurePlayerState* TriggerPlayer = nullptr);

    /** 检查游戏是否已经开始（身份已分配） */
    UFUNCTION(BlueprintPure, Category = "Demon Manager")
    bool IsGameInitialized() const { return bGameInitialized; }

    // ========== 恶魔轮转接口（死亡后调用）==========

// 修改函数签名
    UFUNCTION(BlueprintCallable, Category = "Demon Manager")
    void OnPlayerDeath(ACoopAdventurePlayerState* DeadPlayer, AActor* Killer = nullptr);

    // ========== 查询接口 ==========

    UFUNCTION(BlueprintPure, Category = "Demon Manager")
    ACoopAdventurePlayerState* GetCurrentDemon() const { return CurrentDemonPlayer; }

    UFUNCTION(BlueprintPure, Category = "Demon Manager")
    bool IsPlayerDemon(ACoopAdventurePlayerState* Player) const;

    UFUNCTION(BlueprintPure, Category = "Demon Manager")
    TArray<ACoopAdventurePlayerState*> GetNormalPlayers() const;

    UFUNCTION(BlueprintPure, Category = "Demon Manager")
    TArray<ACoopAdventurePlayerState*> GetAlivePlayers() const;

    // ========== 事件委托 ==========

    UPROPERTY(BlueprintAssignable, Category = "Demon Manager|Events")
    FOnDemonChanged OnDemonChanged;

    UPROPERTY(BlueprintAssignable, Category = "Demon Manager|Events")
    FOnNoDemonAvailable OnNoDemonAvailable;

    /** 当游戏正式开始（首次身份分配完成）时触发 */
    UPROPERTY(BlueprintAssignable, Category = "Demon Manager|Events")
    FOnGameInitialized OnGameInitialized;

protected:
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // ========== 内部逻辑 ==========

    bool AssignNewDemon(bool bExcludeCurrentDemon = true);
    void SetPlayerAsDemon(ACoopAdventurePlayerState* NewDemon);

    /** 给所有存活玩家分配任务 */
    void InitializeTasksForAllPlayers();

    TArray<ACoopAdventurePlayerState*> GetAllAlivePlayerStates() const;

    // ========== 复制属性 ==========

    UPROPERTY(ReplicatedUsing = OnRep_CurrentDemon)
    TObjectPtr<ACoopAdventurePlayerState> CurrentDemonPlayer;

    UPROPERTY(Replicated)
    TObjectPtr<ACoopAdventurePlayerState> PreviousDemonPlayer;

    /** 游戏是否已初始化（首次分配已完成） */
    UPROPERTY(ReplicatedUsing = OnRep_GameInitialized)
    bool bGameInitialized;

    UFUNCTION()
    void OnRep_CurrentDemon();

    UFUNCTION()
    void OnRep_GameInitialized();

    // 已死亡玩家列表
// 替换原来的 TArray<TObjectPtr<...>>
    UPROPERTY()
    TArray<TWeakObjectPtr<APlayerState>> DeadPlayers;

    // 在 public 区域添加
public:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // 在 protected 添加辅助函数

protected:
    UFUNCTION()
    void OnSelectNewDemon(); // 定时器回调函数

    FTimerHandle NewDemonTimerHandle;

    
};