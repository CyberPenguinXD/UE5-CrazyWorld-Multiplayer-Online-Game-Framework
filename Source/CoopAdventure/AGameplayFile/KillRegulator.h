// AGameplayFile/KillRegulator.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KillRegulator.generated.h"

UCLASS(ClassGroup = (Gameplay), meta = (BlueprintSpawnableComponent))
class COOPADVENTURE_API UKillRegulator : public UActorComponent
{
    GENERATED_BODY()

public:
    UKillRegulator();

    // 处理击杀事件（由 HealthComponent::HandleDeath 调用）
    // 不拦截伤害，只进行事后惩罚
    UFUNCTION(BlueprintCallable, Category = "Kill Regulation")
    void ProcessKill(AActor* Victim, AActor* Killer);

protected:
    virtual void BeginPlay() override;

    // 执行惩罚（让违规者立即死亡）
    void PunishPlayer(AActor* Player);

    // 获取玩家的恶魔身份（简化封装）
    bool IsPlayerDemon(AActor* Player) const;

    // KillRegulator.h
protected:
    bool IsPlayerHoly(AActor* Player) const;
};