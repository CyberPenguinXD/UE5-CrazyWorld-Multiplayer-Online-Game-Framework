#include "DailyTaskRefreshComponent.h"
#include "CoopAdventureGameState.h"
#include "AGameplayFile/GamePhaseManager.h"
#include "PlayerStateFile/CoopAdventurePlayerState.h"
#include "AGameplayFile/TaskComponent.h"
#include "GameFramework/GameStateBase.h"

UDailyTaskRefreshComponent::UDailyTaskRefreshComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UDailyTaskRefreshComponent::BeginPlay()
{
    Super::BeginPlay();

    if (GetOwnerRole() != ROLE_Authority) return;

    ACoopAdventureGameState* GS = Cast<ACoopAdventureGameState>(GetWorld()->GetGameState());
    if (!GS) return;

    UGamePhaseManager* PhaseManager = GS->FindComponentByClass<UGamePhaseManager>();
    if (PhaseManager)
    {
        PhaseManager->OnDailyStart.AddDynamic(this, &UDailyTaskRefreshComponent::OnDailyStart);
        UE_LOG(LogTemp, Log, TEXT("DailyTaskRefreshComponent: 已绑定每日开始事件"));
    }
}

void UDailyTaskRefreshComponent::OnDailyStart()
{
    if (GetOwnerRole() != ROLE_Authority) return;

    UE_LOG(LogTemp, Log, TEXT("DailyTaskRefreshComponent: 每日开始，刷新所有玩家的每日任务..."));

    AGameStateBase* GameState = GetWorld()->GetGameState();
    if (!GameState) return;

    for (APlayerState* PS : GameState->PlayerArray)
    {
        ACoopAdventurePlayerState* CoopPS = Cast<ACoopAdventurePlayerState>(PS);
        if (CoopPS)
        {
            UTaskComponent* TaskComp = CoopPS->GetTaskComponent();
            if (TaskComp)
            {
                TaskComp->RefreshDailyTasks();
            }
        }
    }
}