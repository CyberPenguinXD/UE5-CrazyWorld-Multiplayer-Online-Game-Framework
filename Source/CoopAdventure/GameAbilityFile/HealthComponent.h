// CombatFile/HealthComponent.h
#pragma once
#include "GameplayTagContainer.h"
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

// 伤害信息结构体
USTRUCT(BlueprintType)
struct FDamageInfo
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float BaseDamage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTag DamageType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    AActor* Instigator;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector ImpulseDirection;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsCritical;

    FDamageInfo()
        : BaseDamage(0.0f)
        , Instigator(nullptr)
        , ImpulseDirection(FVector::ZeroVector)
        , bIsCritical(false)
    {
    }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnHealthChanged, float, CurrentHealth, float, MaxHealth, float, Delta);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDeath, AActor*, Victim, AActor*, Killer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTakeDamage, const FDamageInfo&, DamageInfo, float, ActualDamage);

UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class COOPADVENTURE_API UHealthComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UHealthComponent();

    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // 查询接口
    UFUNCTION(BlueprintPure, Category = "Health")
    float GetCurrentHealth() const { return CurrentHealth; }

    UFUNCTION(BlueprintPure, Category = "Health")
    float GetMaxHealth() const { return MaxHealth; }

    UFUNCTION(BlueprintPure, Category = "Health")
    float GetHealthPercent() const { return MaxHealth > 0 ? CurrentHealth / MaxHealth : 0.0f; }

    UFUNCTION(BlueprintPure, Category = "Health")
    bool IsAlive() const { return !bIsDead; }

    UFUNCTION(BlueprintPure, Category = "Health")
    bool IsDead() const { return bIsDead; }

    // 战斗接口
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void TakeDamage(const FDamageInfo& DamageInfo);

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void Heal(float HealAmount, AActor* Healer = nullptr);

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void Revive(bool bResetToMaxHP = true);

    // 复活系统 - 立即复活版本
    UPROPERTY(EditDefaultsOnly, Category = "Respawn Config", meta = (ClampMin = "0"))
    float RespawnDelay = 0.0f;  // 保持兼容性，但默认0

    UPROPERTY(ReplicatedUsing = OnRep_CanRespawn, BlueprintReadOnly, Category = "Respawn")
    bool bCanRespawn = false;

    UFUNCTION(BlueprintCallable, Category = "Respawn")
    void RequestRespawn();

    // 事件委托
    UPROPERTY(BlueprintAssignable, Category = "Health|Events")
    FOnHealthChanged OnHealthChanged;

    UPROPERTY(BlueprintAssignable, Category = "Health|Events")
    FOnTakeDamage OnTakeDamage;

    UPROPERTY(BlueprintAssignable, Category = "Health|Events")
    FOnDeath OnDeath;

protected:
    // 配置属性
    UPROPERTY(EditDefaultsOnly, Category = "Config", Replicated)
    float MaxHealth;

    UPROPERTY(EditDefaultsOnly, Category = "Config")
    float InitialHealth;

    UPROPERTY(EditDefaultsOnly, Category = "Config")
    bool bFriendlyFire;

    UPROPERTY(EditDefaultsOnly, Category = "Config")
    bool bDestroyOnDeath;

    // 运行时数据
    UPROPERTY(ReplicatedUsing = OnRep_CurrentHealth)
    float CurrentHealth;

    UPROPERTY(ReplicatedUsing = OnRep_IsDead)
    bool bIsDead;

    UPROPERTY(Replicated)
    float LastDamageTime;

    // Server RPC
    UFUNCTION(Server, Reliable, WithValidation)
    void Server_TakeDamage(const FDamageInfo& DamageInfo);
    void Server_TakeDamage_Implementation(const FDamageInfo& DamageInfo);
    bool Server_TakeDamage_Validate(const FDamageInfo& DamageInfo);

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_Heal(float HealAmount);
    void Server_Heal_Implementation(float HealAmount);
    bool Server_Heal_Validate(float HealAmount);

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_Revive(bool bResetToMaxHP);
    void Server_Revive_Implementation(bool bResetToMaxHP);
    bool Server_Revive_Validate(bool bResetToMaxHP);

    // 复活系统Server RPC
    UFUNCTION(Server, Reliable, WithValidation)
    void Server_PerformRespawn();
    void Server_PerformRespawn_Implementation();
    bool Server_PerformRespawn_Validate();

    // RepNotify
    UFUNCTION()
    void OnRep_CurrentHealth();

    UFUNCTION()
    void OnRep_IsDead();

    UFUNCTION()
    void OnRep_CanRespawn();

    // 内部逻辑
    void ApplyDamage(const FDamageInfo& DamageInfo);
    void HandleDeath(AActor* Killer);
    void BroadcastHealthChanged(float Delta);
};