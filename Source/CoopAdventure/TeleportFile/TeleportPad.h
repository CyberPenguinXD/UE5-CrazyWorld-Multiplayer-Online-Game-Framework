#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Blueprint/UserWidget.h"
#include "TeleportPad.generated.h"

UCLASS()
class COOPADVENTURE_API ATeleportPad : public AActor
{
    GENERATED_BODY()

public:
    ATeleportPad();

   
    UPROPERTY(EditAnywhere, Category = "Teleport Set")
    FName TargetMapName = FName("NewMap");
    // 目标地图名，在编辑器中设置
    // 加载界面Widget类，在蓝图中设置
    UPROPERTY(EditAnywhere, Category = "Teleport Set")
    TSubclassOf<UUserWidget> LoadingWidgetClass;

    // 蓝图调用的传送函数
    UFUNCTION(BlueprintCallable, Category = "TeleportAllPlayer")
    void ExecuteTeleport();

protected:
    void DoServerTravel();
};