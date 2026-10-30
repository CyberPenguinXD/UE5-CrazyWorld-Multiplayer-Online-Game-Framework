#include "DailyTaskPenaltyComponent.h"
#include "CoopAdventureGameState.h"
#include "AGameplayFile/GamePhaseManager.h"
#include "PlayerStateFile/CoopAdventurePlayerState.h"
#include "AGameplayFile/TaskComponent.h"
#include "GameAbilityFile/HealthComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GameplayTagContainer.h"  // 确保包含

const FGameplayTag UDailyTaskPenaltyComponent::DamageTag = FGameplayTag::RequestGameplayTag(FName("Damage.Type.Penalty"), false);

UDailyTaskPenaltyComponent::UDailyTaskPenaltyComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true); // 组件需要复制，但事件只在服务器执行
}

void UDailyTaskPenaltyComponent::BeginPlay()
{
    Super::BeginPlay();

    // 只在服务器绑定事件
    if (GetOwnerRole() != ROLE_Authority) return;  // 修复：使用 GetOwnerRole()

    // 获取 GameState 中的 GamePhaseManager
    ACoopAdventureGameState* GS = Cast<ACoopAdventureGameState>(GetWorld()->GetGameState());
    if (!GS)
    {
        UE_LOG(LogTemp, Error, TEXT("DailyTaskPenaltyComponent: GameState not found or not ACoopAdventureGameState!"));
        return;
    }

    UGamePhaseManager* PhaseManager = GS->FindComponentByClass<UGamePhaseManager>();
    if (PhaseManager)
    {
        PhaseManager->OnDailySettlement.AddDynamic(this, &UDailyTaskPenaltyComponent::OnDailySettlement);
        UE_LOG(LogTemp, Log, TEXT("DailyTaskPenaltyComponent: 已绑定每日结算事件"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("DailyTaskPenaltyComponent: GamePhaseManager not found!"));
    }
}

void UDailyTaskPenaltyComponent::OnDailySettlement()
{
    if (GetOwnerRole() != ROLE_Authority) return;  // 修复

    UE_LOG(LogTemp, Log, TEXT("DailyTaskPenaltyComponent: 每日结算开始，检查玩家每日任务完成情况..."));

    // 获取所有玩家状态
    AGameStateBase* GameState = GetWorld()->GetGameState();
    if (!GameState) return;

    for (APlayerState* PS : GameState->PlayerArray)
    {
        ACoopAdventurePlayerState* CoopPS = Cast<ACoopAdventurePlayerState>(PS);
        if (CoopPS)
        {
            ApplyPenaltyToPlayer(CoopPS);
        }
    }
}

void UDailyTaskPenaltyComponent::ApplyPenaltyToPlayer(ACoopAdventurePlayerState* PS)
{
    // 检查玩家是否存活（通过 Pawn 的 HealthComponent）
    APawn* Pawn = PS->GetPawn();
    if (!Pawn) return;

    UHealthComponent* HealthComp = Pawn->FindComponentByClass<UHealthComponent>();
    if (!HealthComp || HealthComp->IsDead()) return; // 死人不受惩罚

    UTaskComponent* TaskComp = PS->GetTaskComponent();
    if (!TaskComp) return;

    // 检查三个每日任务是否全部完成
    bool bAllDailyCompleted = true;
    int32 DailyCount = 0;
    for (const FTaskInstance& Task : TaskComp->TaskList)
    {
        if (Task.bIsDaily)
        {
            DailyCount++;
            if (!Task.bCompleted)
            {
                bAllDailyCompleted = false;
                break;
            }
        }
    }

    if (!bAllDailyCompleted && DailyCount > 0) // 有每日任务且未全部完成
    {
        // 扣除 50 生命
        FDamageInfo PenaltyDamage;
        PenaltyDamage.BaseDamage = 50.0f;
        PenaltyDamage.DamageType = DamageTag;
        PenaltyDamage.Instigator = nullptr; // 无凶手

        HealthComp->TakeDamage(PenaltyDamage);

        UE_LOG(LogTemp, Warning, TEXT("玩家 %s 未完成所有每日任务，扣除 50 生命！当前生命: %.1f"),
            *PS->GetPlayerName(),
            HealthComp->GetCurrentHealth());
    }
}