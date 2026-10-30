// AGameplayFile/IdentityComponent.cpp
#include "IdentityComponent.h"
#include "Net/UnrealNetwork.h"
            // 需要包含SkillSubsystem头文件
#include "GameAbilityFile/SkillSubsystem.h"
#include "GameFramework/PlayerState.h"

UIdentityComponent::UIdentityComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);

    CurrentIdentity = EPlayerIdentityType::None;
    bIsIdentityRevealed = false;
    HumanSkillPoints = 0;
    DemonSkillPoints = 0;
    SpecialSkillPoints = 0;
}

void UIdentityComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UIdentityComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UIdentityComponent, CurrentIdentity);
    DOREPLIFETIME(UIdentityComponent, bIsIdentityRevealed);
    DOREPLIFETIME(UIdentityComponent, HumanSkillPoints);
    DOREPLIFETIME(UIdentityComponent, DemonSkillPoints);
    DOREPLIFETIME(UIdentityComponent, SpecialSkillPoints);
    DOREPLIFETIME(UIdentityComponent, LearnedSkills);
}

// ========== 查询接口实现 ==========

bool UIdentityComponent::CanSeeOtherIdentity(APlayerState* OtherPlayer) const
{
    if (!OtherPlayer) return false;

    // 如果自己死了或身份已暴露，可以看到所有人身份
    // TODO: 需要结合HealthComponent检查是否死亡

    // 如果身份已公开，所有人可见
    UIdentityComponent* OtherIdentity = OtherPlayer->FindComponentByClass<UIdentityComponent>();
    if (OtherIdentity && OtherIdentity->bIsIdentityRevealed)
        return true;

    // 内鬼之间互相可见
    if (IsImpostor() && OtherIdentity && OtherIdentity->IsImpostor())
        return true;

    // 其他情况只能看到自己的身份
    return false;
}

int32 UIdentityComponent::GetSkillPoints(EPlayerIdentityType ForIdentity) const
{
    switch (ForIdentity)
    {
    case EPlayerIdentityType::Human:   return HumanSkillPoints;
    case EPlayerIdentityType::Demon:   return DemonSkillPoints;
    case EPlayerIdentityType::Holy:    return SpecialSkillPoints;
    case EPlayerIdentityType::Neutral: return SpecialSkillPoints;
    case EPlayerIdentityType::None:    return SpecialSkillPoints; // 新增：None使用特殊点
    default: return 0;
    }
}

// ========== 技能点操作 ==========

void UIdentityComponent::AddSkillPoints(EPlayerIdentityType IdentityType, int32 Amount)
{
    if (GetOwnerRole() != ROLE_Authority) return;
    if (Amount <= 0) return;

    switch (IdentityType)
    {
    case EPlayerIdentityType::Human:
        HumanSkillPoints += Amount;
        break;
    case EPlayerIdentityType::Demon:
        DemonSkillPoints += Amount;
        break;
    case EPlayerIdentityType::Holy:
    case EPlayerIdentityType::Neutral:
        SpecialSkillPoints += Amount;
        break;
    default:
        break;
    }

    OnRep_SkillPoints();
    OnSkillPointsChanged.Broadcast();
}

bool UIdentityComponent::ConsumeSkillPoints(FName SkillID)
{
    if (GetOwnerRole() != ROLE_Authority) return false;

    if (UWorld* World = GetWorld())
    {
        if (UGameInstance* GI = World->GetGameInstance())
        {

            if (USkillSubsystem* Subsystem = GI->GetSubsystem<USkillSubsystem>())
            {
                FSkillStaticData Data;
                if (Subsystem->GetSkillData(SkillID, Data))
                {
                    int32 Cost = Data.SkillPointCost;
                    if (Cost <= 0) Cost = 1;

                    int32* PointsPtr = nullptr;
                    switch (CurrentIdentity)
                    {
                    case EPlayerIdentityType::Demon:
                        PointsPtr = &DemonSkillPoints;
                        break;
                    case EPlayerIdentityType::Holy:
                        PointsPtr = &SpecialSkillPoints;
                        break;
                    case EPlayerIdentityType::Human:
                    default:
                        PointsPtr = &HumanSkillPoints;
                        break;
                    }

                    if (PointsPtr && *PointsPtr >= Cost)
                    {
                        *PointsPtr -= Cost;
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

void UIdentityComponent::Server_LearnSkill_Implementation(FName SkillID)
{
    if (HasLearnedSkill(SkillID))
    {
        UE_LOG(LogTemp, Warning, TEXT("Skill already learned: %s"), *SkillID.ToString());
        return;
    }

    if (ConsumeSkillPoints(SkillID))
    {
        LearnedSkills.Add(SkillID);
        OnRep_SkillPoints();
        OnSkillPointsChanged.Broadcast();
        UE_LOG(LogTemp, Log, TEXT("Learned skill: %s"), *SkillID.ToString());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Not enough skill points for: %s"), *SkillID.ToString());
    }
}

bool UIdentityComponent::Server_LearnSkill_Validate(FName SkillID)
{
    return !SkillID.IsNone();
}

// ========== 服务器权限接口 ==========

void UIdentityComponent::AssignIdentity(EPlayerIdentityType NewIdentity)
{
    if (GetOwnerRole() != ROLE_Authority)
    {
        UE_LOG(LogTemp, Error, TEXT("AssignIdentity: Not on authority! Role=%d"), (int32)GetOwnerRole());
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("AssignIdentity: Changing from %d to %d"), (int32)CurrentIdentity, (int32)NewIdentity);

    EPlayerIdentityType OldIdentity = CurrentIdentity;
    CurrentIdentity = NewIdentity;
    bIsIdentityRevealed = false;

    // 根据身份给予初始技能点
    switch (NewIdentity)
    {
    case EPlayerIdentityType::None:
        SpecialSkillPoints = 0;
        break;
    case EPlayerIdentityType::Demon:
        DemonSkillPoints = 3;
        break;
    case EPlayerIdentityType::Holy:
        SpecialSkillPoints = 2;
        break;
    case EPlayerIdentityType::Human:
    default:
        HumanSkillPoints = 2;
        break;
    }

    OnIdentityChanged.Broadcast(NewIdentity, OldIdentity);
    OnSkillPointsChanged.Broadcast();

    UE_LOG(LogTemp, Log, TEXT("AssignIdentity: Completed. New identity=%d"), (int32)CurrentIdentity);
}

void UIdentityComponent::RevealIdentity()
{
    if (GetOwnerRole() != ROLE_Authority) return;

    bIsIdentityRevealed = true;
    OnIdentityRevealed.Broadcast();
}

void UIdentityComponent::ResetIdentity()
{
    if (GetOwnerRole() != ROLE_Authority) return;

    CurrentIdentity = EPlayerIdentityType::None;
    bIsIdentityRevealed = false;
    LearnedSkills.Empty();
    HumanSkillPoints = 0;
    DemonSkillPoints = 0;
    SpecialSkillPoints = 0;

    OnIdentityChanged.Broadcast(EPlayerIdentityType::None, CurrentIdentity);
}

// ========== RepNotify ==========

void UIdentityComponent::OnRep_CurrentIdentity(EPlayerIdentityType OldIdentity)
{
    OnIdentityChanged.Broadcast(CurrentIdentity, OldIdentity);
}

void UIdentityComponent::OnRep_bIsRevealed()
{
    if (bIsIdentityRevealed)
    {
        OnIdentityRevealed.Broadcast();
    }
}

void UIdentityComponent::OnRep_SkillPoints()
{
    OnSkillPointsChanged.Broadcast();
}