// CombatComponent.cpp - 基础生命周期与攻击调度
#include "CombatComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "InventoryFile/InventoryComponent.h"
#include "InventoryFile/InventorySubsystem.h"
#include "GameAbilityFile/HealthComponent.h"
#include "CoopAdventureCharacter.h"

// ========== 基础生命周期 ==========

UCombatComponent::UCombatComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);

    MeleeTraceRadius = 50.0f;
    MeleeTraceAngle = 60.0f;
    MeleeTraceDistance = 150.0f;

    CurrentAttackMode = EAttackMode::None;
    CurrentWeaponID = NAME_None;
    LastAttackTime = 0.0f;
    bIsInCooldown = false;
    InitRetryCount = 0;

    // 连射系统初始化
    bIsAutoFiring = false;
    AutoFireWeaponID = NAME_None;
    AutoFireInterval = 0.1f;
}

void UCombatComponent::BeginPlay()
{
    Super::BeginPlay();
    TryInitialize();
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UCombatComponent, CurrentAttackMode);
    DOREPLIFETIME(UCombatComponent, CurrentWeaponID);
    DOREPLIFETIME(UCombatComponent, LastAttackTime);
    DOREPLIFETIME(UCombatComponent, bIsInCooldown);
    DOREPLIFETIME(UCombatComponent, bIsAutoFiring);
}

// ========== 初始化 ==========

void UCombatComponent::TryInitialize()
{
    if (InitRetryCount >= MAX_INIT_RETRIES)
    {
        GetWorld()->GetTimerManager().ClearTimer(InitTimerHandle);
        return;
    }
    InitRetryCount++;

    AActor* Owner = GetOwner();
    if (!Owner) return;

    APlayerState* PS = nullptr;
    if (ACharacter* Character = Cast<ACharacter>(Owner))
    {
        PS = Character->GetPlayerState();
    }
    if (!PS) PS = Cast<APlayerState>(Owner);

    if (PS && PS->IsValidLowLevel())
    {
        UInventoryComponent* InvComp = PS->FindComponentByClass<UInventoryComponent>();
        if (InvComp && !bCombatInventoryDelegatesBound)  // 关键：检查是否已绑定！
        {
            InvComp->OnEquippedSlotChanged.AddDynamic(this, &UCombatComponent::OnEquippedItemChanged);
            bCombatInventoryDelegatesBound = true;  // 标记为已绑定

            int32 CurrentSlot = InvComp->GetEquippedSlotIndex();
            FName CurrentItem = InvComp->GetEquippedItemID();
            OnEquippedItemChanged(CurrentSlot, CurrentItem);

            GetWorld()->GetTimerManager().ClearTimer(InitTimerHandle);
            return;
        }
    }

    if (InitRetryCount >= MAX_INIT_RETRIES)
    {
        UE_LOG(LogTemp, Error, TEXT("CombatComponent: 达到最大重试次数，初始化失败"));
    }
}

// ========== 装备与模式切换 ==========

void UCombatComponent::OnEquippedItemChanged(int32 SlotIndex, FName ItemID)
{
    CurrentWeaponID = ItemID;
    UpdateAttackMode();

    // 关键：切换武器时强制停止连射（防止切枪后还继续射）
    if (bIsAutoFiring)
    {
        Server_StopAutoFire_Implementation();
    }
}

void UCombatComponent::UpdateAttackMode()
{
    if (CurrentWeaponID.IsNone())
    {
        CurrentAttackMode = EAttackMode::None;
        return;
    }

    FItemStaticData Data;
    if (GetWeaponData(CurrentWeaponID, Data))
    {
        switch (Data.WeaponType)
        {
        case EWeaponType::Melee:
            CurrentAttackMode = EAttackMode::Melee;
            break;
        case EWeaponType::Gun:
            CurrentAttackMode = EAttackMode::Gun;
            break;
        case EWeaponType::Magic:
            CurrentAttackMode = EAttackMode::Magic;
            break;
        default:
            CurrentAttackMode = EAttackMode::None;
            break;
        }
    }
    else
    {
        CurrentAttackMode = EAttackMode::None;
    }
}

// ========== 攻击接口 ==========

bool UCombatComponent::CanAttack() const
{
    if (bIsInCooldown) return false;
    if (CurrentAttackMode == EAttackMode::None) return false;
    return true;
}

void UCombatComponent::Attack()
{
    if (!CanAttack())
    {
        OnAttackFailed.Broadcast(CurrentWeaponID);
        return;
    }

    if (GetOwnerRole() == ROLE_Authority)
    {
        Server_AttemptAttack_Implementation(CurrentWeaponID, CurrentAttackMode);
    }
    else
    {
        Server_AttemptAttack(CurrentWeaponID, CurrentAttackMode);
    }
}

bool UCombatComponent::Server_AttemptAttack_Validate(FName WeaponID, EAttackMode Mode)
{
    return !WeaponID.IsNone() && Mode != EAttackMode::None;
}

void UCombatComponent::Server_AttemptAttack_Implementation(FName WeaponID, EAttackMode Mode)
{
    if (!CanAttack()) return;

    FItemStaticData WeaponData;
    if (!GetWeaponData(WeaponID, WeaponData)) return;

    bIsInCooldown = true;
    LastAttackTime = GetWorld()->GetTimeSeconds();

    FTimerHandle CooldownHandle;
    GetWorld()->GetTimerManager().SetTimer(CooldownHandle, [this]()
        {
            bIsInCooldown = false;
        }, WeaponData.AttackCooldown, false);

    Multicast_PlayAttackAnimation(Mode, WeaponID);

    // ========== 新增：近战挥砍特效（只加这几行）==========
    if (Mode == EAttackMode::Melee)
    {
        PlayMeleeSwingEffects(WeaponData);
    }
    // =====================================================

    ExecuteAttack(Mode, WeaponID);
}

void UCombatComponent::Multicast_PlayAttackAnimation_Implementation(EAttackMode Mode, FName WeaponID)
{
    switch (Mode)
    {
    case EAttackMode::Melee:
        OnMeleeAttack.Broadcast(WeaponID);
        break;
    case EAttackMode::Gun:
        OnGunAttack.Broadcast(WeaponID);
        break;
    case EAttackMode::Magic:
        OnMagicAttack.Broadcast(WeaponID);
        break;
    default:
        break;
    }
}

// ========== 攻击分发器（具体实现在其他文件）==========

void UCombatComponent::ExecuteAttack(EAttackMode Mode, FName WeaponID)
{
    FItemStaticData WeaponData;
    if (!GetWeaponData(WeaponID, WeaponData)) return;

    switch (Mode)
    {
    case EAttackMode::Melee:
        PerformMeleeTrace(WeaponData.BaseDamage, WeaponData);
        break;
    case EAttackMode::Gun:
        PerformGunTrace(WeaponData.BaseDamage, WeaponData);
        break;
    case EAttackMode::Magic:
        PerformMagicAttack(WeaponData.BaseDamage, WeaponData);
        break;
    default:
        break;
    }
}

// ========== 自动射击系统（连射）==========

void UCombatComponent::StartAutoFire()
{
    // 客户端只发指令，不直接射击
    if (GetOwnerRole() == ROLE_Authority)
    {
        Server_StartAutoFire_Implementation();
    }
    else
    {
        Server_StartAutoFire();
    }
}

void UCombatComponent::Server_StartAutoFire_Implementation()
{
    // 已经在连射中，不重复启动
    if (bIsAutoFiring) return;

    // 检查当前武器是否支持连射（只有Gun和Magic支持，Melee不支持）
    if (CurrentAttackMode != EAttackMode::Gun && CurrentAttackMode != EAttackMode::Magic)
    {
        //UE_LOG(LogTemp, Warning, TEXT("AutoFire: Only Gun/Magic weapons support autofire"));
        return;
    }

    // 获取武器数据确定射速
    FItemStaticData WeaponData;
    if (!GetCurrentWeaponData(WeaponData)) return;

    // 计算射击间隔（基于AttackCooldown，但连射通常有最小值限制）
    AutoFireInterval = FMath::Max(WeaponData.AttackCooldown, 0.08f); // 最小80ms间隔（约750 RPM）
    AutoFireWeaponID = CurrentWeaponID;

    bIsAutoFiring = true;

   // UE_LOG(LogTemp, Log, TEXT("AutoFire started: %s, Interval: %.2fs"), *CurrentWeaponID.ToString(), AutoFireInterval);

    // 立即射击第一发（不等待定时器）
    ExecuteAutoFireShot();

    // 启动循环定时器
    GetWorld()->GetTimerManager().SetTimer(
        AutoFireTimerHandle,
        this,
        &UCombatComponent::AutoFireTick,
        AutoFireInterval,
        true  // 循环
    );
}

void UCombatComponent::StopAutoFire()
{
    if (GetOwnerRole() == ROLE_Authority)
    {
        Server_StopAutoFire_Implementation();
    }
    else
    {
        Server_StopAutoFire();
    }
}

void UCombatComponent::Server_StopAutoFire_Implementation()
{
    if (!bIsAutoFiring) return;

    bIsAutoFiring = false;
    AutoFireWeaponID = NAME_None;

    // 清除定时器
    GetWorld()->GetTimerManager().ClearTimer(AutoFireTimerHandle);

   // UE_LOG(LogTemp, Log, TEXT("AutoFire stopped"));
}

void UCombatComponent::AutoFireTick()
{
    // 安全验证：检查武器是否还在，是否还是原来那把
    if (!bIsAutoFiring) return;

    // 如果武器被切换了，自动停止
    if (CurrentWeaponID != AutoFireWeaponID)
    {
      //  UE_LOG(LogTemp, Warning, TEXT("AutoFire: Weapon changed, stopping"));
        Server_StopAutoFire_Implementation();
        return;
    }

    // 检查是否还能射击（弹药、状态等）
    if (!CanAutoFire())
    {
        Server_StopAutoFire_Implementation();
        return;
    }

    ExecuteAutoFireShot();
}

void UCombatComponent::ExecuteAutoFireShot()
{
    // 复用现有攻击逻辑，但绕过冷却检查（因为定时器已经控制了间隔）
    if (CurrentAttackMode == EAttackMode::Gun)
    {
        FItemStaticData WeaponData;
        if (GetCurrentWeaponData(WeaponData))
        {
            PerformGunTrace(WeaponData.BaseDamage, WeaponData);

            // 广播动画事件（使用 Multicast 确保所有人看到）
            Multicast_PlayAttackAnimation(CurrentAttackMode, CurrentWeaponID);
        }
    }
    else if (CurrentAttackMode == EAttackMode::Magic)
    {
        FItemStaticData WeaponData;
        if (GetCurrentWeaponData(WeaponData))
        {
            PerformMagicAttack(WeaponData.BaseDamage, WeaponData);
            Multicast_PlayAttackAnimation(CurrentAttackMode, CurrentWeaponID);
        }
    }
}

bool UCombatComponent::CanAutoFire() const
{
    // 基础检查：是否活着，是否有武器
    if (!IsValid(GetOwner())) return false;
    if (CurrentAttackMode == EAttackMode::None) return false;

    // TODO: 可以在这里加弹药检查
    // if (CurrentAmmo <= 0) return false;

    return true;
}

// ========== 辅助函数 ==========

bool UCombatComponent::GetWeaponData(FName WeaponID, FItemStaticData& OutData) const
{
    if (WeaponID.IsNone()) return false;
    if (UWorld* World = GetWorld())
    {
        if (UGameInstance* GI = World->GetGameInstance())
        {
            if (UInventorySubsystem* Subsystem = GI->GetSubsystem<UInventorySubsystem>())
            {
                return Subsystem->GetItemData(WeaponID, OutData);
            }
        }
    }
    return false;
}

bool UCombatComponent::GetCurrentWeaponData(FItemStaticData& OutData) const
{
    return GetWeaponData(CurrentWeaponID, OutData);
}


void UCombatComponent::PlayMeleeSwingEffects(const FItemStaticData& WeaponData)
{
    FVector WeaponLoc = GetOwner()->GetActorLocation();
    if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
    {
        if (ACoopAdventureCharacter* CoopChar = Cast<ACoopAdventureCharacter>(Character))
        {
            if (UStaticMeshComponent* HeldMesh = CoopChar->GetHeldItemMesh())
            {
                WeaponLoc = HeldMesh->GetComponentLocation();
            }
        }
    }

    Multicast_PlayMeleeSwing(CurrentWeaponID, WeaponLoc);
}


void UCombatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // 取消近战挥砍加载
    for (auto& Pair : PendingMeleeSwings)
    {
        if (Pair.Key.IsValid())
            Pair.Key->CancelHandle();
    }
    PendingMeleeSwings.Empty();

    // 取消近战命中加载
    for (auto& Pair : PendingMeleeImpacts)
    {
        if (Pair.Key.IsValid())
            Pair.Key->CancelHandle();
    }
    PendingMeleeImpacts.Empty();

    // 取消近战未命中加载
    for (auto& Pair : PendingMeleeMisses)
    {
        if (Pair.Key.IsValid())
            Pair.Key->CancelHandle();
    }
    PendingMeleeMisses.Empty();

    // 如果枪械部分也使用了句柄存储，同样需要取消
    // ...（之前枪械部分可能也有，需一并处理）

    Super::EndPlay(EndPlayReason);
}