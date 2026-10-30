#include "SkillInstance.h"
#include "SkillComponent.h"
#include "SkillSubsystem.h"
#include "PlayerStateFile/CoopAdventurePlayerState.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

USkillInstance::USkillInstance()
{
    PrimaryComponentTick.bCanEverTick = true;
    bTickInEditor = false;
    SetIsReplicatedByDefault(true);  // 保持启用网络复制

    CurrentState = ESkillState::Ready;
    CurrentCharges = 0;
    CooldownRemaining = 0.0f;
    CastTimeRemaining = 0.0f;
    SlotIndex = -1;
}

void USkillInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(USkillInstance, SkillID);
    DOREPLIFETIME(USkillInstance, CurrentState);
    DOREPLIFETIME(USkillInstance, CooldownRemaining);
    DOREPLIFETIME(USkillInstance, CastTimeRemaining);
    DOREPLIFETIME(USkillInstance, CurrentCharges);
    DOREPLIFETIME(USkillInstance, SlotIndex);  // 关键：注册 SlotIndex 复制
}

void USkillInstance::BeginPlay()
{
    Super::BeginPlay();
}

void USkillInstance::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    OnStateChanged.Clear();
    Super::EndPlay(EndPlayReason);
}

void USkillInstance::InitializeSkill(USkillComponent* OwnerComp, FName InSkillID)
{
    OwnerComponent = OwnerComp;
    SkillID = InSkillID;

    FSkillStaticData Data = GetSkillData();
    CurrentCharges = Data.MaxCharges > 0 ? Data.MaxCharges : -1;
    CooldownRemaining = 0.0f;
    CastTimeRemaining = 0.0f;
    CurrentState = ESkillState::Ready;
}

void USkillInstance::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (CurrentState == ESkillState::Casting)
    {
        UpdateCasting(DeltaTime);
    }
    else if (CurrentState == ESkillState::Cooldown)
    {
        UpdateCooldown(DeltaTime);
    }
}

bool USkillInstance::TryActivate()
{
    if (CurrentState != ESkillState::Ready) return false;
    if (CurrentCharges == 0) return false;

    FSkillStaticData Data = GetSkillData();

    if (Data.CastTime > 0.0f)
    {
        SetState(ESkillState::Casting);
        CastTimeRemaining = Data.CastTime;
    }
    else
    {
        ExecuteEffect();
        StartCooldown();
        ConsumeCharge();
    }

    return true;
}

void USkillInstance::Cancel()
{
    if (CurrentState == ESkillState::Casting)
    {
        SetState(ESkillState::Cooldown);
        StartCooldown();
    }
}

float USkillInstance::GetCooldownPercent() const
{
    FSkillStaticData Data = GetSkillData();
    if (Data.Cooldown <= 0.0f) return 0.0f;
    return FMath::Clamp(CooldownRemaining / Data.Cooldown, 0.0f, 1.0f);
}

float USkillInstance::GetCastPercent() const
{
    FSkillStaticData Data = GetSkillData();
    if (Data.CastTime <= 0.0f) return 1.0f;
    return 1.0f - FMath::Clamp(CastTimeRemaining / Data.CastTime, 0.0f, 1.0f);
}

FSkillStaticData USkillInstance::GetSkillData() const
{
    if (UWorld* World = GetWorld())
    {
        if (UGameInstance* GI = World->GetGameInstance())
        {
            if (USkillSubsystem* Subsystem = GI->GetSubsystem<USkillSubsystem>())
            {
                FSkillStaticData Data;
                if (Subsystem->GetSkillData(SkillID, Data))
                {
                    return Data;
                }
            }
        }
    }
    return FSkillStaticData();
}

ACharacter* USkillInstance::GetOwnerCharacter() const
{
    if (!OwnerComponent) return nullptr;

    if (ACharacter* Char = Cast<ACharacter>(OwnerComponent->GetOwner()))
    {
        return Char;
    }

    if (APlayerState* PS = Cast<APlayerState>(OwnerComponent->GetOwner()))
    {
        return Cast<ACharacter>(PS->GetPawn());
    }

    return nullptr;
}

void USkillInstance::SetState(ESkillState NewState)
{
    ESkillState OldState = CurrentState;
    CurrentState = NewState;
    OnStateChanged.Broadcast(OldState, NewState);
}

void USkillInstance::OnRep_State()
{
    OnStateChanged.Broadcast(CurrentState, CurrentState);
}

void USkillInstance::StartCooldown()
{
    FSkillStaticData Data = GetSkillData();
    if (Data.Cooldown > 0.0f)
    {
        CooldownRemaining = Data.Cooldown;
        SetState(ESkillState::Cooldown);
    }
    else
    {
        SetState(ESkillState::Ready);
    }
}

void USkillInstance::ConsumeCharge()
{
    if (CurrentCharges > 0)
    {
        CurrentCharges--;
    }
}

void USkillInstance::UpdateCooldown(float DeltaTime)
{
    if (CooldownRemaining > 0.0f)
    {
        float OldRemaining = CooldownRemaining;
        CooldownRemaining -= DeltaTime;

        // 通知 UI 刷新（只要有实质变化）
        if (OwnerComponent && SlotIndex != -1)
        {
            if (FMath::Abs(OldRemaining - CooldownRemaining) > KINDA_SMALL_NUMBER)
            {
                OwnerComponent->NotifyCooldownUpdated(SlotIndex);
            }
        }

        if (CooldownRemaining <= 0.0f)
        {
            CooldownRemaining = 0.0f;
            OnCooldownEnd();
            SetState(ESkillState::Ready);
            if (OwnerComponent && SlotIndex != -1)
            {
                OwnerComponent->NotifyCooldownUpdated(SlotIndex);
            }
        }
    }
}

void USkillInstance::UpdateCasting(float DeltaTime)
{
    if (CastTimeRemaining > 0.0f)
    {
        CastTimeRemaining -= DeltaTime;
        if (CastTimeRemaining <= 0.0f)
        {
            CastTimeRemaining = 0.0f;
            OnCastFinished();
        }
    }
}

void USkillInstance::OnActivate() {}
void USkillInstance::OnCastFinished()
{
    ExecuteEffect();
    StartCooldown();
    ConsumeCharge();
}
void USkillInstance::OnCooldownEnd() {}
void USkillInstance::ExecuteEffect() {}