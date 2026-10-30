// CombatComponent.h
#pragma once
#include "Engine/StreamableManager.h"
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "InventoryFile/ItemData.h"
#include "CombatComponent.generated.h"

class UHealthComponent;
class ACoopAdventureCharacter;

UENUM(BlueprintType)
enum class EAttackMode : uint8
{
    None    UMETA(DisplayName = "None"),
    Melee   UMETA(DisplayName = "Melee"),
    Gun     UMETA(DisplayName = "Gun"),
    Magic   UMETA(DisplayName = "Magic")
};

USTRUCT(BlueprintType)
struct FAttackResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    bool bSuccess;

    UPROPERTY(BlueprintReadOnly)
    float DamageDealt;

    UPROPERTY(BlueprintReadOnly)
    AActor* HitTarget;

    UPROPERTY(BlueprintReadOnly)
    FName WeaponID;

    FAttackResult()
        : bSuccess(false)
        , DamageDealt(0.0f)
        , HitTarget(nullptr)
    {
    }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMeleeAttack, FName, WeaponID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGunAttack, FName, WeaponID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMagicAttack, FName, WeaponID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAttackFailed, FName, WeaponID);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnGunHit, FVector, HitLocation, FVector, HitNormal, AActor*, HitActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGunHitEnvironment, FVector, HitLocation, FVector, HitNormal);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMagicHit, FVector, HitLocation, AActor*, HitActor);

UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class COOPADVENTURE_API UCombatComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCombatComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // ========== ��Ҫ�ӿ� ==========

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void Attack();

    UFUNCTION(BlueprintPure, Category = "Combat")
    EAttackMode GetCurrentAttackMode() const { return CurrentAttackMode; }

    UFUNCTION(BlueprintPure, Category = "Combat")
    bool CanAttack() const;

    UFUNCTION(BlueprintPure, Category = "Combat")
    bool GetCurrentWeaponData(FItemStaticData& OutData) const;

    UFUNCTION()
    void TryInitialize();

    // ========== ��ͼ�¼�ί�� ==========

    UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
    FOnMeleeAttack OnMeleeAttack;

    UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
    FOnGunAttack OnGunAttack;

    UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
    FOnMagicAttack OnMagicAttack;

    UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
    FOnAttackFailed OnAttackFailed;

    UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
    FOnGunHit OnGunHit;

    UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
    FOnGunHitEnvironment OnGunHitEnvironment;

    UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
    FOnMagicHit OnMagicHit;

    // ========== ����֪ͨ�ӿ� ==========

    UFUNCTION(BlueprintCallable, Category = "Combat|Animation")
    void AnimNotify_DealDamage();

    UFUNCTION(BlueprintPure, Category = "Combat")
    bool IsMeleeWeaponEquipped() const { return CurrentAttackMode == EAttackMode::Melee; }

    // ========== ����ϵͳ ==========

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void StartAutoFire();

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void StopAutoFire();

    UFUNCTION(BlueprintPure, Category = "Combat")
    bool IsAutoFiring() const { return bIsAutoFiring; }

protected:
    // ========== ��ս���� ==========

    UPROPERTY(EditDefaultsOnly, Category = "Melee Config")
    float MeleeTraceRadius;

    UPROPERTY(EditDefaultsOnly, Category = "Melee Config")
    float MeleeTraceAngle;

    UPROPERTY(EditDefaultsOnly, Category = "Melee Config")
    float MeleeTraceDistance;

    // ========== ��ս��Чϵͳ ==========

    // ��������ʱ���Żӿ���Ч����Ч
    void PlayMeleeSwingEffects(const FItemStaticData& WeaponData);

    // Multicast�����Żӿ���Ч����Ч����������ʱ��
    UFUNCTION(NetMulticast, Reliable)
    void Multicast_PlayMeleeSwing(FName WeaponID, FVector WeaponLocation);
    void Multicast_PlayMeleeSwing_Implementation(FName WeaponID, FVector WeaponLocation);

    // Multicast������������Ч����Ч������Ŀ��ʱ��
    UFUNCTION(NetMulticast, Reliable)
    void Multicast_PlayMeleeImpact(FVector HitLocation, bool bHitActor, FName WeaponID);
    void Multicast_PlayMeleeImpact_Implementation(FVector HitLocation, bool bHitActor, FName WeaponID);

    // Multicast������δ������Ч������ʱ��
    UFUNCTION(NetMulticast, Reliable)
    void Multicast_PlayMeleeMiss(FName WeaponID, FVector MissLocation);
    void Multicast_PlayMeleeMiss_Implementation(FName WeaponID, FVector MissLocation);

    // ========== ����ʱ״̬��Replicated��==========

    UPROPERTY(Replicated)
    EAttackMode CurrentAttackMode;

    UPROPERTY(Replicated)
    FName CurrentWeaponID;

    UPROPERTY(Replicated)
    float LastAttackTime;

    UPROPERTY(Replicated)
    bool bIsInCooldown;

    // ========== ��ʼ����� ==========

    FTimerHandle InitTimerHandle;
    int32 InitRetryCount;
    static constexpr int32 MAX_INIT_RETRIES = 5;

    // ========== Server RPC ==========

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_AttemptAttack(FName WeaponID, EAttackMode Mode);
    void Server_AttemptAttack_Implementation(FName WeaponID, EAttackMode Mode);
    bool Server_AttemptAttack_Validate(FName WeaponID, EAttackMode Mode);

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_PlayAttackAnimation(EAttackMode Mode, FName WeaponID);
    void Multicast_PlayAttackAnimation_Implementation(EAttackMode Mode, FName WeaponID);

    // ========== ����ʵ�� ==========

    UFUNCTION()
    void OnEquippedItemChanged(int32 SlotIndex, FName ItemID);

    void UpdateAttackMode();

    void ExecuteAttack(EAttackMode Mode, FName WeaponID);

    void PerformMeleeTrace(float Damage, const FItemStaticData& WeaponData);

    void PerformGunTrace(float Damage, const FItemStaticData& WeaponData);

public:

    UFUNCTION(BlueprintPure, Category = "Combat")
    bool GetMuzzleTransform(FTransform& OutMuzzleTransform) const;

protected:

    UFUNCTION()
    void PerformMagicAttack(float Damage, const FItemStaticData& WeaponData);

    void SpawnMagicProjectile(float Damage, float ProjectileSpeed, float Range);

    // ========== ����ϵͳ�ڲ� ==========

    void AutoFireTick();

    void ExecuteAutoFireShot();

    bool CanAutoFire() const;

    UFUNCTION(Server, Reliable)
    void Server_StartAutoFire();
    void Server_StartAutoFire_Implementation();

    UFUNCTION(Server, Reliable)
    void Server_StopAutoFire();
    void Server_StopAutoFire_Implementation();

    // ========== �������� ==========

    bool GetWeaponData(FName WeaponID, FItemStaticData& OutData) const;

    bool bInventoryDelegatesBound = false;

    bool bCombatInventoryDelegatesBound = false;

    // ========== ����״̬ ==========

    UPROPERTY(Replicated)
    bool bIsAutoFiring = false;

    FTimerHandle AutoFireTimerHandle;

    FName AutoFireWeaponID;

    float AutoFireInterval = 0.1f;

    // ========== ǹе��Чϵͳ ==========

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_PlayGunEffects(FVector HitLocation, FVector HitNormal, bool bHitActor, FName WeaponID);
    void Multicast_PlayGunEffects_Implementation(FVector HitLocation, FVector HitNormal, bool bHitActor, FName WeaponID);

    // 在类定义内部添加
private:
    // 用于存储一次枪击所需的所有参数，供异步加载完成后使用
    struct FPendingGunShot
    {
        FVector HitLocation;
        FVector HitNormal;
        bool bHitActor;
        FName WeaponID;
        float TimeStamp; // 可选，用于调试或防止过时请求
    };

    // 当前正在进行的异步加载请求对应的参数
    TMap<TSharedPtr<FStreamableHandle>, FPendingGunShot> PendingGunShots;

    void OnGunEffectsLoaded(TSharedPtr<FStreamableHandle> Handle);
    void PlayGunEffectsImmediate(FVector HitLocation, FVector HitNormal, bool bHitActor, const FItemStaticData& WeaponData);

public:
    // 确保 EndPlay 被声明（通常放在 public 或 protected）
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;




//近战
private:
    // 用于存储一次近战挥砍的参数（供异步加载完成后使用）
    struct FPendingMeleeSwing
    {
        FName WeaponID;
        FVector WeaponLocation;
        float TimeStamp;
    };

    // 用于存储一次近战命中的参数
    struct FPendingMeleeImpact
    {
        FVector HitLocation;
        bool bHitActor;
        FName WeaponID;
        float TimeStamp;
    };

    // 用于存储一次近战未命中的参数
    struct FPendingMeleeMiss
    {
        FName WeaponID;
        FVector MissLocation;
        float TimeStamp;
    };

    // 进行中的异步加载请求及其对应的参数
    TMap<TSharedPtr<FStreamableHandle>, FPendingMeleeSwing> PendingMeleeSwings;
    TMap<TSharedPtr<FStreamableHandle>, FPendingMeleeImpact> PendingMeleeImpacts;
    TMap<TSharedPtr<FStreamableHandle>, FPendingMeleeMiss> PendingMeleeMisses;

    // 加载完成回调
    void OnMeleeSwingLoaded(TSharedPtr<FStreamableHandle> Handle);
    void OnMeleeImpactLoaded(TSharedPtr<FStreamableHandle> Handle);
    void OnMeleeMissLoaded(TSharedPtr<FStreamableHandle> Handle);

    // 立即播放函数（资源已加载时调用）
    void PlayMeleeSwingImmediate(FName WeaponID, FVector WeaponLocation);
    void PlayMeleeImpactImmediate(FVector HitLocation, bool bHitActor, FName WeaponID);
    void PlayMeleeMissImmediate(FName WeaponID, FVector MissLocation);
};