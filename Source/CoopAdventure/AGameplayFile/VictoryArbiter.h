#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VictoryArbiter.generated.h"

UENUM(BlueprintType)
enum class EVictoryType : uint8
{
    None            UMETA(DisplayName = "进行中"),
    HumanVictory    UMETA(DisplayName = "人类胜利"),
    DemonVictory    UMETA(DisplayName = "恶魔胜利")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVictoryDetected, EVictoryType, VictoryType);

UCLASS(ClassGroup = (Gameplay), meta = (BlueprintSpawnableComponent))
class COOPADVENTURE_API UVictoryArbiter : public UActorComponent
{
    GENERATED_BODY()

public:
    UVictoryArbiter();

    // 检查恶魔死亡（由DemonManager调用）
    UFUNCTION(BlueprintCallable, Category = "Victory")
    void CheckDemonDeath(ACoopAdventurePlayerState* DeadDemon);

    // 检查人类死亡（由HealthComponent调用）
    UFUNCTION(BlueprintCallable, Category = "Victory")
    void CheckHumanDeath(ACoopAdventurePlayerState* DeadHuman);

    UFUNCTION(BlueprintPure, Category = "Victory")
    EVictoryType GetCurrentVictoryState() const { return CurrentVictoryState; }

    UFUNCTION(BlueprintPure, Category = "Victory")
    bool IsGameEnded() const { return CurrentVictoryState != EVictoryType::None; }

    UPROPERTY(BlueprintAssignable, Category = "Victory|Events")
    FOnVictoryDetected OnVictoryDetected;

protected:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    void TriggerVictory(EVictoryType VictoryType);

    int32 GetAliveHumanCount() const;
    int32 GetAliveDemonCount() const;

    UFUNCTION()
    void OnRep_VictoryState();

private:
    UPROPERTY(ReplicatedUsing = OnRep_VictoryState)
    EVictoryType CurrentVictoryState;


public:

    UFUNCTION(BlueprintCallable, Category = "Victory")
    void CheckVictoryConditions(); // 通用检查，不依赖死者身份

 
};