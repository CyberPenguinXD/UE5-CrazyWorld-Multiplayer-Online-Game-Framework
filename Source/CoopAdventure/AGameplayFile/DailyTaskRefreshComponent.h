#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DailyTaskRefreshComponent.generated.h"

UCLASS(ClassGroup = (Gameplay), meta = (BlueprintSpawnableComponent))
class COOPADVENTURE_API UDailyTaskRefreshComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UDailyTaskRefreshComponent();

protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    void OnDailyStart();
};