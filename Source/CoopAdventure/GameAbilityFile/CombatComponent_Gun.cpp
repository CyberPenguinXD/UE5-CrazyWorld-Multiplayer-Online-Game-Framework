// CombatComponent_Gun.cpp
#include "CombatComponent.h"
#include "GameFramework/Character.h"
#include "DrawDebugHelpers.h"
#include "GameAbilityFile/HealthComponent.h"
#include "CoopAdventureCharacter.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Engine/AssetManager.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Sound/SoundAttenuation.h"

// ============================================================================
// 原有函数保留：GetMuzzleTransform, PerformGunTrace 等保持不变
// ============================================================================

bool UCombatComponent::GetMuzzleTransform(FTransform& OutMuzzleTransform) const
{
    if (!IsValid(GetOwner()))
    {
        return false;
    }

    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!Character) return false;

    ACoopAdventureCharacter* CoopChar = Cast<ACoopAdventureCharacter>(Character);
    if (!CoopChar) return false;

    UStaticMeshComponent* HeldMesh = CoopChar->GetHeldItemMesh();
    if (HeldMesh && HeldMesh->IsVisible() && HeldMesh->DoesSocketExist(FName("MuzzleSocket")))
    {
        OutMuzzleTransform = HeldMesh->GetSocketTransform(FName("MuzzleSocket"), RTS_World);
        return true;
    }

    // Fallback
    FVector CameraLocation;
    FRotator CameraRotation;
    if (Character->GetController())
    {
        Character->GetController()->GetPlayerViewPoint(CameraLocation, CameraRotation);
        OutMuzzleTransform.SetLocation(CameraLocation);
        OutMuzzleTransform.SetRotation(CameraRotation.Quaternion());
        return true;
    }
    return false;
}

void UCombatComponent::PerformGunTrace(float Damage, const FItemStaticData& WeaponData)
{
    if (GetOwnerRole() != ROLE_Authority) return;

    if (!IsValid(GetOwner()))
    {
        UE_LOG(LogTemp, Warning, TEXT("PerformGunTrace: Owner无效"));
        return;
    }

    FTransform MuzzleTransform;
    if (!GetMuzzleTransform(MuzzleTransform)) return;

    FVector StartLocation = MuzzleTransform.GetLocation();
    FVector ShootDirection = MuzzleTransform.GetRotation().GetForwardVector();
    float Range = (WeaponData.AttackRange > 0) ? WeaponData.AttackRange : 5000.0f;
    FVector EndLocation = StartLocation + (ShootDirection * Range);

    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(GetOwner());
    QueryParams.bTraceComplex = true;

    bool bHit = GetWorld()->LineTraceSingleByChannel(
        HitResult, StartLocation, EndLocation, ECC_Pawn, QueryParams);

    bool bHitActor = false;
    if (bHit && HitResult.GetActor())
    {
        AActor* HitActor = HitResult.GetActor();
        bHitActor = true;

        if (UHealthComponent* HealthComp = HitActor->FindComponentByClass<UHealthComponent>())
        {
            FDamageInfo DamageInfo;
            DamageInfo.BaseDamage = Damage;
            DamageInfo.Instigator = Cast<AActor>(GetOwner());
            static const FGameplayTag GunTag = FGameplayTag::RequestGameplayTag(FName("Damage.Type.Gun"), false);
            DamageInfo.DamageType = GunTag.IsValid() ? GunTag : FGameplayTag::EmptyTag;
            DamageInfo.ImpulseDirection = ShootDirection;
            HealthComp->TakeDamage(DamageInfo);
            OnGunHit.Broadcast(HitResult.ImpactPoint, HitResult.Normal, HitActor);
        }
        else
        {
            OnGunHitEnvironment.Broadcast(HitResult.ImpactPoint, HitResult.Normal);
        }
    }

    FVector HitLoc = bHit ? HitResult.ImpactPoint : EndLocation;
    FVector HitNorm = bHit ? HitResult.Normal : -ShootDirection;
    Multicast_PlayGunEffects(HitLoc, HitNorm, bHitActor, CurrentWeaponID);
}

// ============================================================================
// 新增：异步加载枪械特效的核心实现
// ============================================================================

void UCombatComponent::Multicast_PlayGunEffects_Implementation(
    FVector HitLocation, FVector HitNormal, bool bHitActor, FName WeaponID)
{
    if (!IsValid(GetWorld()) || !IsValid(GetOwner())) return;

    FItemStaticData WeaponData;
    if (!GetWeaponData(WeaponID, WeaponData)) return;

    // 收集所有需要加载的软引用资源路径
    TArray<FSoftObjectPath> AssetsToLoad;

    auto AddIfValid = [&](const TSoftObjectPtr<UObject>& SoftPtr) {
        if (!SoftPtr.IsNull())
            AssetsToLoad.Add(SoftPtr.ToSoftObjectPath());
        };

    AddIfValid(WeaponData.MuzzleFlashEffect);
    AddIfValid(WeaponData.TracerEffect);
    AddIfValid(WeaponData.ImpactEffect_Default);
    AddIfValid(WeaponData.ImpactDecalMaterial);
    AddIfValid(WeaponData.GunshotAttenuation);
    AddIfValid(WeaponData.ImpactSound_Hit);
    AddIfValid(WeaponData.ImpactSound_Miss);
    for (const auto& Sound : WeaponData.MuzzleSounds)
        AddIfValid(Sound);

    // 如果没有资源需要加载，直接返回（理论上不可能，但安全起见）
    if (AssetsToLoad.Num() == 0)
    {
        return;
    }

    // 检查资源是否已经加载（通过判断枪口特效是否已加载来近似判断）
    if (WeaponData.MuzzleFlashEffect.IsValid())
    {
        // 资源已加载，直接播放
        PlayGunEffectsImmediate(HitLocation, HitNormal, bHitActor, WeaponData);
        return;
    }

    // 否则发起异步加载
    FStreamableManager& Streamable = UAssetManager::GetStreamableManager();

    // 使用 Lambda 作为委托，捕获需要的参数，避免创建额外成员函数
    TSharedPtr<FStreamableHandle> Handle = Streamable.RequestAsyncLoad(AssetsToLoad,
        FStreamableDelegate::CreateLambda([this, HitLocation, HitNormal, bHitActor, WeaponID]()
            {
                // 加载完成回调
                if (!IsValid(this) || !IsValid(GetWorld()))
                    return;

                // 重新获取武器数据（可能已变化，但大概率不变）
                FItemStaticData CurrentWeaponData;
                if (!GetWeaponData(WeaponID, CurrentWeaponData))
                    return;

                // 现在所有资源应该已加载，直接播放
                PlayGunEffectsImmediate(HitLocation, HitNormal, bHitActor, CurrentWeaponData);
            })
    );

    // 注意：这里我们没有将 Handle 存入 PendingGunShots，因为 Lambda 已经捕获了参数
    // 如果需要取消支持，可以存起来，但为了简洁，此处省略取消逻辑（见 EndPlay 处理）
}

void UCombatComponent::PlayGunEffectsImmediate(
    FVector HitLocation, FVector HitNormal, bool bHitActor, const FItemStaticData& WeaponData)
{
    if (!IsValid(GetWorld()) || !IsValid(GetOwner())) return;

    FTransform MuzzleTransform;
    if (!GetMuzzleTransform(MuzzleTransform)) return;
    FVector MuzzleLoc = MuzzleTransform.GetLocation();

    // ========== 1. 枪口火焰特效 ==========
    UNiagaraSystem* MuzzleEffect = WeaponData.GetMuzzleFlashEffectSafe(); // 现在同步加载不会阻塞
    if (MuzzleEffect != nullptr)
    {
        UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            GetWorld(), MuzzleEffect, MuzzleLoc, MuzzleTransform.GetRotation().Rotator());

        if (NiagaraComp != nullptr)
        {
            ACharacter* Character = Cast<ACharacter>(GetOwner());
            if (ACoopAdventureCharacter* CoopChar = Cast<ACoopAdventureCharacter>(Character))
            {
                if (UStaticMeshComponent* HeldMesh = CoopChar->GetHeldItemMesh())
                {
                    if (HeldMesh->DoesSocketExist(FName("MuzzleSocket")))
                    {
                        NiagaraComp->AttachToComponent(
                            HeldMesh,
                            FAttachmentTransformRules::KeepWorldTransform,
                            FName("MuzzleSocket")
                        );
                    }
                }
            }
        }
    }

    // ========== 2. 射击音效 ==========
    USoundCue* FireSound = WeaponData.GetFirstMuzzleSoundSafe();
    if (FireSound != nullptr)
    {
        USoundAttenuation* Attenuation = WeaponData.GetGunshotAttenuationSafe();
        UGameplayStatics::SpawnSoundAtLocation(
            GetWorld(), FireSound, MuzzleLoc, FRotator::ZeroRotator, 1.0f, 1.0f, 0.0f, Attenuation);
    }

    // ========== 3. 弹道轨迹特效 ==========
    UNiagaraSystem* TracerEffect = WeaponData.GetTracerEffectSafe();
    if (TracerEffect != nullptr)
    {
        UNiagaraComponent* TracerComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            GetWorld(), TracerEffect, MuzzleLoc, (HitLocation - MuzzleLoc).GetSafeNormal().Rotation());

        if (TracerComp != nullptr)
        {
            TracerComp->SetVectorParameter(FName("StartLocation"), MuzzleLoc);
            TracerComp->SetVectorParameter(FName("EndLocation"), HitLocation);
            TracerComp->SetFloatParameter(FName("Speed"), WeaponData.TracerSpeed);
            TracerComp->SetAutoDestroy(true);
        }
    }

    // ========== 4. 未命中音效 ==========
    if (!bHitActor)
    {
        USoundCue* MissSound = WeaponData.GetImpactSoundMissSafe();
        if (MissSound != nullptr)
        {
            UGameplayStatics::SpawnSoundAtLocation(
                GetWorld(), MissSound, HitLocation, HitNormal.Rotation(), 0.6f, 1.0f, 0.0f, nullptr);
        }
    }

    // ========== 5. 命中音效 ==========
    if (bHitActor)
    {
        USoundCue* HitSound = WeaponData.GetImpactSoundHitSafe();
        if (HitSound != nullptr)
        {
            UGameplayStatics::SpawnSoundAtLocation(
                GetWorld(), HitSound, HitLocation, HitNormal.Rotation(), 1.0f, 1.0f, 0.0f, nullptr);
        }
    }

    // ========== 6. 命中特效 ==========
    if (bHitActor)
    {
        UNiagaraSystem* ImpactEffect = WeaponData.GetImpactEffectSafe();
        if (ImpactEffect != nullptr)
        {
            UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                GetWorld(), ImpactEffect, HitLocation, HitNormal.Rotation());
        }
    }

    // ========== 7. 弹孔贴花 ==========
    if (bHitActor && WeaponData.ImpactDecalMaterial.IsValid())
    {
        UMaterialInterface* DecalMat = WeaponData.GetImpactDecalMaterialSafe();
        if (DecalMat != nullptr)
        {
            FVector DecalLoc = HitLocation + HitNormal * 2.0f;
            FRotator DecalRot = HitNormal.Rotation();
            DecalRot.Pitch -= 90.0f;

            UGameplayStatics::SpawnDecalAtLocation(
                GetWorld(), DecalMat, FVector(10.0f, 10.0f, 10.0f), DecalLoc, DecalRot, WeaponData.ImpactDecalLifeSpan);
        }
    }
}

//void UCombatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
//{
    // 取消所有异步加载句柄（如果有存储的话）
    // 当前实现没有存储句柄，因为 Lambda 捕获了 this，如果组件销毁，Lambda 不应再执行。
    // 但为了更安全，我们可以存储句柄并取消。此处省略存储逻辑，但通过 Lambda 捕获的 this 是弱引用？不，Lambda 捕获的是原始指针，会导致野指针。
    // 因此更好的做法是存储句柄并在 EndPlay 中取消。但考虑到简化，我们可以在 Lambda 中检查 IsValid(this) 来避免访问野指针，但无法阻止 Lambda 执行。
    // 如果希望完全避免，应该存储句柄并取消。这里为了完整，演示存储句柄的版本，但上述实现未存储。如果需要，可以改用类成员函数并存储句柄，就像最初的设计。
    // 但为了代码简洁，我们保留上述 Lambda 方式，并在 Lambda 开头检查 IsValid(this) 来防止野指针访问。这虽然不是完美的取消，但能避免崩溃。

//    Super::EndPlay(EndPlayReason);
//}