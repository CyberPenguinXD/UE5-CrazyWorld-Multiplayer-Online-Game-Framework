// CombatComponent_Melee.cpp
#include "CombatComponent.h"
#include "GameFramework/Character.h"
#include "DrawDebugHelpers.h"
#include "GameAbilityFile/HealthComponent.h"
#include "CoopAdventureCharacter.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/AssetManager.h"
#include "Sound/SoundCue.h"

void UCombatComponent::AnimNotify_DealDamage()
{
    if (GetOwnerRole() != ROLE_Authority) return;
    if (CurrentAttackMode == EAttackMode::Melee && !CurrentWeaponID.IsNone())
    {
        FItemStaticData WeaponData;
        if (GetWeaponData(CurrentWeaponID, WeaponData))
        {
            PerformMeleeTrace(WeaponData.BaseDamage, WeaponData);
        }
    }
}

void UCombatComponent::PerformMeleeTrace(float Damage, const FItemStaticData& WeaponData)
{
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!Character) return;

    FVector StartLocation = Character->GetActorLocation();
    FVector ForwardVector = Character->GetActorForwardVector();
    TArray<FHitResult> HitResults;
    FCollisionShape CollisionShape;
    CollisionShape.ShapeType = ECollisionShape::Sphere;
    CollisionShape.SetSphere(MeleeTraceRadius);
    FVector EndLocation = StartLocation + ForwardVector * MeleeTraceDistance;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(Character);

    bool bHit = GetWorld()->SweepMultiByChannel(
        HitResults, StartLocation, EndLocation, FQuat::Identity,
        ECC_Pawn, CollisionShape, QueryParams);

    bool bHitAnyValidTarget = false;

    if (bHit)
    {
        for (const FHitResult& Hit : HitResults)
        {
            AActor* HitActor = Hit.GetActor();
            if (!HitActor) continue;

            FVector DirToTarget = (HitActor->GetActorLocation() - StartLocation).GetSafeNormal();
            float Angle = FMath::Acos(FVector::DotProduct(ForwardVector, DirToTarget));
            float AngleDegrees = FMath::RadiansToDegrees(Angle);

            if (AngleDegrees <= MeleeTraceAngle * 0.5f)
            {
                bool bActuallyHit = false;

                if (UHealthComponent* HealthComp = HitActor->FindComponentByClass<UHealthComponent>())
                {
                    FDamageInfo DamageInfo;
                    DamageInfo.BaseDamage = Damage;
                    DamageInfo.Instigator = Character;
                    static const FGameplayTag MeleeTag = FGameplayTag::RequestGameplayTag(FName("Damage.Type.Melee"), false);
                    DamageInfo.DamageType = MeleeTag.IsValid() ? MeleeTag : FGameplayTag::EmptyTag;
                    HealthComp->TakeDamage(DamageInfo);
                    bActuallyHit = true;
                    bHitAnyValidTarget = true;
                }

                Multicast_PlayMeleeImpact(Hit.ImpactPoint, bActuallyHit, CurrentWeaponID);
            }
        }
    }

    if (!bHitAnyValidTarget)
    {
        Multicast_PlayMeleeMiss(CurrentWeaponID, EndLocation);
    }
}

// ========== 挥砍特效（异步加载版）==========
void UCombatComponent::Multicast_PlayMeleeSwing_Implementation(FName WeaponID, FVector WeaponLocation)
{
    if (!IsValid(GetWorld())) return;

    FItemStaticData WeaponData;
    if (!GetWeaponData(WeaponID, WeaponData)) return;

    // 收集需要加载的资源
    TArray<FSoftObjectPath> AssetsToLoad;
    auto AddIfValid = [&](const TSoftObjectPtr<UObject>& SoftPtr) {
        if (!SoftPtr.IsNull())
            AssetsToLoad.Add(SoftPtr.ToSoftObjectPath());
        };

    for (const auto& Sound : WeaponData.SwingSounds)
    {
        AddIfValid(Sound);
    }
    AddIfValid(WeaponData.MeleeTrailEffect);
    AddIfValid(WeaponData.MeleeSwingAttenuation);

    // 如果资源已经加载（例如 MeleeTrailEffect 有效），直接播放
    if (WeaponData.MeleeTrailEffect.IsValid())
    {
        PlayMeleeSwingImmediate(WeaponID, WeaponLocation);
        return;
    }

    // 如果没有资源需要加载，直接返回（理论上不会发生）
    if (AssetsToLoad.Num() == 0)
    {
        return;
    }

    // 发起异步加载，使用 Lambda 回调
    FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
    TSharedPtr<FStreamableHandle> Handle = Streamable.RequestAsyncLoad(AssetsToLoad,
        [this, WeaponID, WeaponLocation]()
        {
            // 加载完成后检查组件是否仍然有效
            if (!IsValid(this) || !IsValid(GetWorld()))
                return;

            PlayMeleeSwingImmediate(WeaponID, WeaponLocation);
        }
    );

    // 可选：存储句柄用于取消（如果需要在 EndPlay 中取消加载）
    // 但即使不存储，Lambda 中的有效性检查也能防止崩溃
    // 如果希望存储，可以像之前一样用 PendingMeleeSwings 保存
}

void UCombatComponent::OnMeleeSwingLoaded(TSharedPtr<FStreamableHandle> Handle)
{
    if (!PendingMeleeSwings.Contains(Handle)) return;
    FPendingMeleeSwing Pending = PendingMeleeSwings.FindAndRemoveChecked(Handle);

    if (!IsValid(this) || !IsValid(GetWorld())) return;
    PlayMeleeSwingImmediate(Pending.WeaponID, Pending.WeaponLocation);
}

void UCombatComponent::PlayMeleeSwingImmediate(FName WeaponID, FVector WeaponLocation)
{
    if (!IsValid(GetWorld())) return;

    FItemStaticData WeaponData;
    if (!GetWeaponData(WeaponID, WeaponData)) return;

    // 挥砍音效
    USoundCue* SwingSound = WeaponData.GetFirstSwingSoundSafe();
    if (SwingSound != nullptr)
    {
        USoundAttenuation* Attenuation = WeaponData.GetMeleeSwingAttenuationSafe();
        UGameplayStatics::SpawnSoundAtLocation(GetWorld(), SwingSound, WeaponLocation,
            FRotator::ZeroRotator, 1.0f, 1.0f, 0.0f, Attenuation);
    }

    // 刀光特效
    UNiagaraSystem* TrailEffect = WeaponData.GetMeleeTrailEffectSafe();
    if (TrailEffect != nullptr)
    {
        ACharacter* Character = Cast<ACharacter>(GetOwner());
        if (ACoopAdventureCharacter* CoopChar = Cast<ACoopAdventureCharacter>(Character))
        {
            if (UStaticMeshComponent* HeldMesh = CoopChar->GetHeldItemMesh())
            {
                if (IsValid(HeldMesh))
                {
                    UNiagaraFunctionLibrary::SpawnSystemAttached(
                        TrailEffect,
                        HeldMesh,
                        NAME_None,
                        FVector::ZeroVector,
                        FRotator::ZeroRotator,
                        EAttachLocation::SnapToTarget,
                        true
                    );
                }
            }
        }
    }
}

// ========== 命中特效（异步加载版）==========
void UCombatComponent::Multicast_PlayMeleeImpact_Implementation(FVector HitLocation, bool bHitActor, FName WeaponID)
{
    if (!IsValid(GetWorld())) return;

    FItemStaticData WeaponData;
    if (!GetWeaponData(WeaponID, WeaponData)) return;

    TArray<FSoftObjectPath> AssetsToLoad;
    auto AddIfValid = [&](const TSoftObjectPtr<UObject>& SoftPtr) {
        if (!SoftPtr.IsNull())
            AssetsToLoad.Add(SoftPtr.ToSoftObjectPath());
        };

    AddIfValid(WeaponData.MeleeImpactSound_Hit);
    AddIfValid(WeaponData.MeleeImpactSound_Miss);
    AddIfValid(WeaponData.MeleeImpactEffect);

    if (WeaponData.MeleeImpactEffect.IsValid())
    {
        PlayMeleeImpactImmediate(HitLocation, bHitActor, WeaponID);
        return;
    }

    if (AssetsToLoad.Num() == 0) return;

    FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
    TSharedPtr<FStreamableHandle> Handle = Streamable.RequestAsyncLoad(AssetsToLoad,
        [this, HitLocation, bHitActor, WeaponID]()
        {
            if (!IsValid(this) || !IsValid(GetWorld())) return;
            PlayMeleeImpactImmediate(HitLocation, bHitActor, WeaponID);
        }
    );
}

void UCombatComponent::OnMeleeImpactLoaded(TSharedPtr<FStreamableHandle> Handle)
{
    if (!PendingMeleeImpacts.Contains(Handle)) return;
    FPendingMeleeImpact Pending = PendingMeleeImpacts.FindAndRemoveChecked(Handle);

    if (!IsValid(this) || !IsValid(GetWorld())) return;
    PlayMeleeImpactImmediate(Pending.HitLocation, Pending.bHitActor, Pending.WeaponID);
}

void UCombatComponent::PlayMeleeImpactImmediate(FVector HitLocation, bool bHitActor, FName WeaponID)
{
    if (!IsValid(GetWorld())) return;

    FItemStaticData WeaponData;
    if (!GetWeaponData(WeaponID, WeaponData)) return;

    // 命中音效
    if (bHitActor)
    {
        USoundCue* HitSound = WeaponData.GetMeleeImpactSoundHitSafe();
        if (HitSound != nullptr)
        {
            UGameplayStatics::SpawnSoundAtLocation(GetWorld(), HitSound, HitLocation);
        }
    }

    // 未命中音效（如果有单独的未命中音效，可以在这里播放，但通常命中/未命中音效不同）
    // 这里省略未命中音效，因为未命中已经在另一个函数中处理

    // 命中特效
    UNiagaraSystem* ImpactEffect = WeaponData.GetMeleeImpactEffectSafe();
    if (ImpactEffect != nullptr)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            GetWorld(),
            ImpactEffect,
            HitLocation + FMath::VRand() * 5.0f,
            FRotator::ZeroRotator
        );
    }
}

// ========== 未命中特效（异步加载版）==========
void UCombatComponent::Multicast_PlayMeleeMiss_Implementation(FName WeaponID, FVector MissLocation)
{
    if (!IsValid(GetWorld())) return;

    FItemStaticData WeaponData;
    if (!GetWeaponData(WeaponID, WeaponData)) return;

    TArray<FSoftObjectPath> AssetsToLoad;
    auto AddIfValid = [&](const TSoftObjectPtr<UObject>& SoftPtr) {
        if (!SoftPtr.IsNull())
            AssetsToLoad.Add(SoftPtr.ToSoftObjectPath());
        };

    AddIfValid(WeaponData.MeleeImpactSound_Miss);

    if (WeaponData.MeleeImpactSound_Miss.IsValid())
    {
        PlayMeleeMissImmediate(WeaponID, MissLocation);
        return;
    }

    if (AssetsToLoad.Num() == 0) return;

    FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
    TSharedPtr<FStreamableHandle> Handle = Streamable.RequestAsyncLoad(AssetsToLoad,
        [this, WeaponID, MissLocation]()
        {
            if (!IsValid(this) || !IsValid(GetWorld())) return;
            PlayMeleeMissImmediate(WeaponID, MissLocation);
        }
    );
}

void UCombatComponent::OnMeleeMissLoaded(TSharedPtr<FStreamableHandle> Handle)
{
    if (!PendingMeleeMisses.Contains(Handle)) return;
    FPendingMeleeMiss Pending = PendingMeleeMisses.FindAndRemoveChecked(Handle);

    if (!IsValid(this) || !IsValid(GetWorld())) return;
    PlayMeleeMissImmediate(Pending.WeaponID, Pending.MissLocation);
}

void UCombatComponent::PlayMeleeMissImmediate(FName WeaponID, FVector MissLocation)
{
    if (!IsValid(GetWorld())) return;

    FItemStaticData WeaponData;
    if (!GetWeaponData(WeaponID, WeaponData)) return;

    USoundCue* MissSound = WeaponData.GetMeleeImpactSoundMissSafe();
    if (MissSound != nullptr)
    {
        UGameplayStatics::SpawnSoundAtLocation(GetWorld(), MissSound, MissLocation);
    }
}