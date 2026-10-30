// UIFile/VictoryWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../AGameplayFile/VictoryArbiter.h"  // 为了获取 EVictoryType 和委托
#include "VictoryWidget.generated.h"

UCLASS(Abstract, Blueprintable)  // Abstract 表示必须在蓝图继承才能使用
class COOPADVENTURE_API UVictoryWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    // ===== 蓝图可读取的胜利数据（供绑定使用）=====
    UPROPERTY(BlueprintReadOnly, Category = "Victory")
    EVictoryType CurrentVictoryType;

    UPROPERTY(BlueprintReadOnly, Category = "Victory")
    FText VictoryTitleText;

    UPROPERTY(BlueprintReadOnly, Category = "Victory")
    FText VictoryDescriptionText;

    // ===== 蓝图实现的界面显示事件 =====
    // C++ 调用此事件，蓝图在这里实现：设置文本、播放动画、显示界面等
    UFUNCTION(BlueprintImplementableEvent, Category = "Victory", meta = (DisplayName = "On Show Victory"))
    void BP_OnShowVictory(EVictoryType VictoryType, const FText& Title, const FText& Description);

    // ===== 蓝图可调的关闭接口 =====
    UFUNCTION(BlueprintCallable, Category = "Victory")
    void CloseVictoryScreen();

    // ===== 辅助函数：获取胜利类型对应的显示文本 =====
    UFUNCTION(BlueprintPure, Category = "Victory")
    static FText GetVictoryTypeDisplayName(EVictoryType Type);

protected:
    // 内部回调：响应 VictoryArbiter 的胜利事件
    UFUNCTION()
    void HandleVictoryDetected(EVictoryType VictoryType);

    void BindToVictoryArbiter();
    void UnbindFromVictoryArbiter();

    UPROPERTY()
    TObjectPtr<UVictoryArbiter> CachedArbiter;
};