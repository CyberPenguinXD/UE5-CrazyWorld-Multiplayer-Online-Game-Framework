#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"  // 添加此行
#include "DailyTaskPenaltyComponent.generated.h"

UCLASS(ClassGroup = (Gameplay), meta = (BlueprintSpawnableComponent))
class COOPADVENTURE_API UDailyTaskPenaltyComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UDailyTaskPenaltyComponent();

protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    void OnDailySettlement();

    void ApplyPenaltyToPlayer(class ACoopAdventurePlayerState* PlayerState);

private:
    // 静态成员不需要标记 UPROPERTY，因为它不属于 UObject 系统
    static const FGameplayTag DamageTag;
};