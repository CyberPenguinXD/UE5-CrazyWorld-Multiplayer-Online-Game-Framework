#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../SkillTypes.h"
#include "FireballProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UNiagaraSystem;
class UNiagaraComponent;
class USoundCue;

UCLASS()
class COOPADVENTURE_API AFireballProjectile : public AActor
{
    GENERATED_BODY()

public:
    AFireballProjectile();

    // 初始化投射物（服务器调用，客户端通过复制接收）
    void InitProjectile(float InDamage, float InRadius, float InSpeed, float InLifeTime, FName InSkillID = NAME_None);

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION()
    void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

    UFUNCTION()
    void OnLifeTimeEnd();

    void DoExplode(const FVector& Location);

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_PlayExplosion(FVector Location);
    void Multicast_PlayExplosion_Implementation(FVector Location);

    // 立即播放爆炸特效（资源已加载时调用）
    void PlayExplosionImmediate(FVector Location);

    void ApplyDamage(const FVector& Center);

    // 从技能数据加载特效资源（支持异步加载）
    void LoadEffectsFromSkillData();

    UPROPERTY(VisibleAnywhere, Category = "Components")
    TObjectPtr<USphereComponent> CollisionSphere;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    TObjectPtr<UProjectileMovementComponent> MovementComp;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    TObjectPtr<UNiagaraComponent> TrailSystem;

    float Damage;
    float ExplosionRadius;
    float LifeTime;

    UPROPERTY(ReplicatedUsing = OnRep_SkillID)
    FName SkillID;

    // 软引用资源（从技能数据获取，支持异步加载）
    TSoftObjectPtr<UNiagaraSystem> TrailEffect;
    TSoftObjectPtr<UNiagaraSystem> ImpactEffect;
    TSoftObjectPtr<USoundCue> ImpactSound;

    bool bExploded;
    FTimerHandle LifeTimerHandle;

    // 防止重复加载轨迹特效的标志
    bool bTrailLoadingStarted = false;

    UFUNCTION()
    void OnRep_SkillID();
};