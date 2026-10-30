// UIFile/VictoryWidget.cpp
#include "VictoryWidget.h"
#include "Kismet/GameplayStatics.h"
#include "../CoopAdventureGameState.h"

void UVictoryWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 初始隐藏，等待胜利事件
    SetVisibility(ESlateVisibility::Collapsed);

    // 绑定到胜利仲裁器
    BindToVictoryArbiter();
}

void UVictoryWidget::NativeDestruct()
{
    UnbindFromVictoryArbiter();
    Super::NativeDestruct();
}

void UVictoryWidget::BindToVictoryArbiter()
{
    if (GetWorld())
    {
        if (ACoopAdventureGameState* GS = Cast<ACoopAdventureGameState>(UGameplayStatics::GetGameState(this)))
        {
            CachedArbiter = GS->FindComponentByClass<UVictoryArbiter>();
            if (CachedArbiter)
            {
                // 绑定到胜利委托
                CachedArbiter->OnVictoryDetected.AddDynamic(this, &UVictoryWidget::HandleVictoryDetected);

                // 如果游戏已经结束（晚加入或重新创建UI），立即显示
                if (CachedArbiter->IsGameEnded())
                {
                    HandleVictoryDetected(CachedArbiter->GetCurrentVictoryState());
                }
            }
        }
    }
}

void UVictoryWidget::UnbindFromVictoryArbiter()
{
    if (CachedArbiter)
    {
        CachedArbiter->OnVictoryDetected.RemoveDynamic(this, &UVictoryWidget::HandleVictoryDetected);
    }
}

void UVictoryWidget::HandleVictoryDetected(EVictoryType VictoryType)
{
    if (VictoryType == EVictoryType::None) return;

    CurrentVictoryType = VictoryType;

    // 根据胜利类型设置文本（C++ 负责逻辑和文本）
    switch (VictoryType)
    {
    case EVictoryType::HumanVictory:
        VictoryTitleText = FText::FromString(TEXT("人类胜利"));
        VictoryDescriptionText = FText::FromString(TEXT("恶魔已被全部消灭，人类获得了胜利！"));
        break;

    case EVictoryType::DemonVictory:
        VictoryTitleText = FText::FromString(TEXT("恶魔胜利"));
        VictoryDescriptionText = FText::FromString(TEXT("人类已被全部消灭，恶魔统治了世界！"));
        break;

    default:
        VictoryTitleText = FText::FromString(TEXT("游戏结束"));
        VictoryDescriptionText = FText::FromString(TEXT(""));
        break;
    }

    // 调用蓝图事件，蓝图在这里实现具体的UI表现（设置TextBlock、播放动画等）
    BP_OnShowVictory(VictoryType, VictoryTitleText, VictoryDescriptionText);
}

void UVictoryWidget::CloseVictoryScreen()
{
    // 从视口移除
    RemoveFromParent();

    // 可选：返回主菜单（取消注释下方代码）
    // UGameplayStatics::OpenLevel(this, FName("MainMenu"));
}

FText UVictoryWidget::GetVictoryTypeDisplayName(EVictoryType Type)
{
    switch (Type)
    {
    case EVictoryType::HumanVictory: return FText::FromString(TEXT("人类阵营"));
    case EVictoryType::DemonVictory: return FText::FromString(TEXT("恶魔阵营"));
    default: return FText::FromString(TEXT("未知"));
    }
}