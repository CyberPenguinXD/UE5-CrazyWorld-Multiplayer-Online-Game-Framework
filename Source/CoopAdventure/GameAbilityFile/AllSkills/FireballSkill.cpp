#include "FireballSkill.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundCue.h"
#include "FireballProjectile.h"
#include "../SkillData.h"
#include "Engine/AssetManager.h"
#include "Engine/World.h"

UFireballSkill::UFireballSkill()
{
    PrimaryComponentTick.bCanEverTick = true;
    bAutoActivate = true;
}

void UFireballSkill::ExecuteEffect()
{
    ACharacter* OwnerChar = GetOwnerCharacter();
    if (!OwnerChar) return;

    FSkillStaticData Data = GetSkillData();

    FVector SpawnLocation = OwnerChar->GetActorLocation()
        + OwnerChar->GetActorForwardVector() * 100.0f
        + FVector(0, 0, 50.0f);
    FRotator SpawnRotation = OwnerChar->GetControlRotation();

    // 播放施法特效（异步加载由 Multicast 处理）
    Multicast_PlayCastEffects(SpawnLocation, SpawnRotation);

    if (OwnerChar->HasAuthority())
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = OwnerChar;
        SpawnParams.Instigator = OwnerChar;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        AFireballProjectile* Projectile = OwnerChar->GetWorld()->SpawnActor<AFireballProjectile>(
            AFireballProjectile::StaticClass(),
            SpawnLocation,
            SpawnRotation,
            SpawnParams
        );

        if (Projectile)
        {
            float LifeTime = Data.Range > 0 ? Data.Range / Data.ProjectileSpeed : 5.0f;
            // 只传递技能ID，特效由客户端自行加载
            Projectile->InitProjectile(
                Data.GetDamageAtLevel(1),
                Data.EffectRadius,
                Data.ProjectileSpeed,
                LifeTime,
                Data.SkillID
            );

            Projectile->SetReplicates(true);
            Projectile->SetReplicateMovement(true);
        }
    }
}

void UFireballSkill::Multicast_PlayCastEffects_Implementation(FVector Location, FRotator Rotation)
{
    FSkillStaticData Data = GetSkillData();

    // 收集需要加载的资源
    TArray<FSoftObjectPath> AssetsToLoad;
    auto AddIfValid = [&](const TSoftObjectPtr<UObject>& SoftPtr) {
        if (!SoftPtr.IsNull())
            AssetsToLoad.Add(SoftPtr.ToSoftObjectPath());
        };

    AddIfValid(Data.CastSound);
    AddIfValid(Data.CastEffect);

    // 如果关键资源已加载，立即播放
    if (Data.CastEffect.IsValid())
    {
        PlayCastEffectsImmediate(Location, Rotation, Data);
        return;
    }

    if (AssetsToLoad.Num() == 0) return;

    // 异步加载资源
    FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
    TSharedPtr<FStreamableHandle> Handle = Streamable.RequestAsyncLoad(AssetsToLoad,
        [this, Location, Rotation, Data]()
        {
            if (!IsValid(this) || !IsValid(GetWorld())) return;
            PlayCastEffectsImmediate(Location, Rotation, Data);
        }
    );
}

void UFireballSkill::PlayCastEffectsImmediate(FVector Location, FRotator Rotation, const FSkillStaticData& Data)
{
    // 施法音效
    USoundCue* CastSound = Data.GetCastSoundSafe();
    if (CastSound != nullptr)
    {
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), CastSound, Location, Rotation);
    }

    // 施法特效
    UNiagaraSystem* CastEffect = Data.GetCastEffectSafe();
    if (CastEffect != nullptr)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            GetWorld(),
            CastEffect,
            Location,
            Rotation,
            FVector(1.0f),
            true,
            true
        );
    }
}