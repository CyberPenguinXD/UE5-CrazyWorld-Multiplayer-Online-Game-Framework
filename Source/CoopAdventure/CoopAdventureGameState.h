// CoopAdventureGameState.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "AGameplayFile/DemonManagerComponent.h"  //  DemonManager 头文件（必须在 .generated.h 之前）
#include "AGameplayFile/GamePhaseManager.h"  // <-- 添加这一行
#include "AGameplayFile/KillRegulator.h"
#include "AGameplayFile/VictoryArbiter.h"
#include "AGameplayFile/DailyTaskPenaltyComponent.h"
#include "AGameplayFile/DailyTaskRefreshComponent.h"
#include "CoopAdventureGameState.generated.h"      //  必须是最后一个 include！

UCLASS()
class COOPADVENTURE_API ACoopAdventureGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    ACoopAdventureGameState();

    // 获取恶魔管理组件（蓝图可用）
    UFUNCTION(BlueprintPure, Category = "Demon Manager")
    UDemonManagerComponent* GetDemonManager() const { return DemonManager; }

protected:
    virtual void BeginPlay() override;

    // 恶魔管理组件（自动创建）
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UDemonManagerComponent> DemonManager;

    // 在原有DemonManager下添加
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UGamePhaseManager> PhaseManager;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UKillRegulator> KillRegulator;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UVictoryArbiter> VictoryArbiter;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UDailyTaskPenaltyComponent> DailyTaskPenalty;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UDailyTaskRefreshComponent> DailyTaskRefresh;
};