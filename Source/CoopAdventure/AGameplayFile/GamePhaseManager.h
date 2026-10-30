// AGameplayFile/GamePhaseManager.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GamePhaseManager.generated.h"

// 游戏阶段枚举
UENUM(BlueprintType)
enum class EDayPhase : uint8
{
    Dawn        UMETA(DisplayName = "黎明过渡"),    // 05:30-06:00 (可选)
    Day         UMETA(DisplayName = "白天"),        // 06:00-18:00
    Dusk        UMETA(DisplayName = "黄昏过渡"),    // 18:00-18:30 (可选)
    Night       UMETA(DisplayName = "夜晚"),        // 18:00-06:00 (次日)
    Waiting     UMETA(DisplayName = "等待开始")     // 游戏未开始
};

// 阶段变化委托（含新旧值供UI做动画判断）
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDayPhaseChanged, EDayPhase, NewPhase, EDayPhase, OldPhase);
// 天数变化委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDayCountChanged, int32, NewDayCount);
// 每日结算委托（18:00触发，用于任务惩罚）
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDailySettlement);
// 每日开始委托（06:00触发，重置计数器）
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDailyStart);

UCLASS(ClassGroup = (Gameplay), meta = (BlueprintSpawnableComponent))
class COOPADVENTURE_API UGamePhaseManager : public UActorComponent
{
    GENERATED_BODY()

public:
    UGamePhaseManager();

    // ===== 核心查询接口（蓝图可用）=====
    UFUNCTION(BlueprintPure, Category = "Game Phase")
    EDayPhase GetCurrentPhase() const { return CurrentPhase; }

    UFUNCTION(BlueprintPure, Category = "Game Phase")
    int32 GetCurrentDayCount() const { return CurrentDayCount; }

    UFUNCTION(BlueprintPure, Category = "Game Phase")
    bool IsDay() const { return CurrentPhase == EDayPhase::Day; }

    UFUNCTION(BlueprintPure, Category = "Game Phase")
    bool IsNight() const { return CurrentPhase == EDayPhase::Night; }

    UFUNCTION(BlueprintPure, Category = "Game Phase")
    bool IsInTransition() const { return CurrentPhase == EDayPhase::Dawn || CurrentPhase == EDayPhase::Dusk; }

    // ===== 时间更新接口（由蓝图TimeController调用）=====
    // 建议：在TimeController中每1分钟或每小时变化时调用
    UFUNCTION(BlueprintCallable, Category = "Game Phase", meta = (CallInEditor = "true"))
    void UpdatePhaseFromTime(int32 CurrentHour, int32 CurrentMinute = 0);

    // 强制切换阶段（调试/GM命令用）
    UFUNCTION(BlueprintCallable, Category = "Game Phase|Debug", meta = (CallInEditor = "true"))
    void ForceSetPhase(EDayPhase NewPhase);

    // ===== 击杀计数管理（供DemonManager使用）=====
    UFUNCTION(BlueprintCallable, Category = "Kill Regulation")
    void ResetDailyKillCount();

    UFUNCTION(BlueprintPure, Category = "Kill Regulation")
    int32 GetDailyKillCount() const { return DailyKillCount; }

    UFUNCTION(BlueprintCallable, Category = "Kill Regulation")
    void IncrementDailyKillCount();

    UFUNCTION(BlueprintPure, Category = "Kill Regulation")
    bool CanDemonKillToday() const { return DailyKillCount < 1; }

    // ===== 委托事件（蓝图绑定）=====
    UPROPERTY(BlueprintAssignable, Category = "Game Phase|Events")
    FOnDayPhaseChanged OnPhaseChanged;

    UPROPERTY(BlueprintAssignable, Category = "Game Phase|Events")
    FOnDayCountChanged OnDayCountChanged;

    UPROPERTY(BlueprintAssignable, Category = "Game Phase|Events")
    FOnDailySettlement OnDailySettlement;  // 18:00触发，任务惩罚在此处监听

    UPROPERTY(BlueprintAssignable, Category = "Game Phase|Events")
    FOnDailyStart OnDailyStart;            // 06:00触发，重置计数器

protected:
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // 内部阶段切换逻辑
    UFUNCTION()
    void SetGamePhase(EDayPhase NewPhase);

    // RepNotify回调
    UFUNCTION()
    void OnRep_CurrentPhase(EDayPhase OldPhase);

    UFUNCTION()
    void OnRep_DayCount();

    // 检查是否需要触发每日结算（防止重复触发）
    void CheckAndTriggerSettlement();

private:
    // ===== 网络同步属性 =====
    UPROPERTY(ReplicatedUsing = OnRep_CurrentPhase)
    EDayPhase CurrentPhase;

    UPROPERTY(ReplicatedUsing = OnRep_DayCount)
    int32 CurrentDayCount;

    // 当前白天的击杀计数（恶魔专用）
    UPROPERTY(Replicated)
    int32 DailyKillCount;

    // 防止每日结算重复触发
    UPROPERTY(Replicated)
    bool bSettlementTriggeredToday;

    // 最后记录的小时（用于检测时间倒流/跳跃）
    UPROPERTY()
    int32 LastRecordedHour;

    // 在类声明中添加

public:
    // 查询夜晚击杀计数
    UFUNCTION(BlueprintPure, Category = "Kill Regulation")
    int32 GetNightHumanKillCount() const { return NightHumanKillCount; }

    UFUNCTION(BlueprintPure, Category = "Kill Regulation")
    int32 GetNightDemonKillCount() const { return NightDemonKillCount; }

    // 增加夜晚击杀计数
    UFUNCTION(BlueprintCallable, Category = "Kill Regulation")
    void IncrementNightHumanKillCount();

    UFUNCTION(BlueprintCallable, Category = "Kill Regulation")
    void IncrementNightDemonKillCount();

    // 检查是否还能击杀
    UFUNCTION(BlueprintPure, Category = "Kill Regulation")
    bool CanHumanKillAtNight() const { return NightHumanKillCount < 1; }

    UFUNCTION(BlueprintPure, Category = "Kill Regulation")
    bool CanDemonKillAtNight() const { return NightDemonKillCount < 1; }

protected:
    // 重置夜晚击杀计数
    void ResetNightKillCounts();

private:
    // 夜晚人类击杀计数（所有人类共享）
    UPROPERTY(Replicated)
    int32 NightHumanKillCount;

    // 夜晚恶魔击杀计数（所有恶魔共享）
    UPROPERTY(Replicated)
    int32 NightDemonKillCount;
};