// SkillInstance.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SkillData.h"
#include "SkillInstance.generated.h"

UENUM(BlueprintType)
enum class ESkillState : uint8
{
    Ready,
    Casting,
    Cooldown,
    Finished
};

class USkillComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSkillStateChanged, ESkillState, OldState, ESkillState, NewState);

UCLASS(Blueprintable, ClassGroup = (Skill), meta = (BlueprintSpawnableComponent))
class COOPADVENTURE_API USkillInstance : public UActorComponent
{
    GENERATED_BODY()

public:
    USkillInstance();

    UFUNCTION(BlueprintCallable, Category = "Skill")
    void InitializeSkill(USkillComponent* OwnerComp, FName InSkillID);

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable, Category = "Skill")
    virtual bool TryActivate();

    UFUNCTION(BlueprintCallable, Category = "Skill")
    virtual void Cancel();

    UFUNCTION(BlueprintPure, Category = "Skill")
    ESkillState GetState() const { return CurrentState; }

    UFUNCTION(BlueprintPure, Category = "Skill")
    float GetCooldownPercent() const;

    UFUNCTION(BlueprintPure, Category = "Skill")
    float GetCastPercent() const;

    UFUNCTION(BlueprintPure, Category = "Skill")
    int32 GetCurrentCharges() const { return CurrentCharges; }

    UFUNCTION(BlueprintPure, Category = "Skill")
    FName GetSkillID() const { return SkillID; }

    // 槽位管理
    UFUNCTION(BlueprintCallable, Category = "Skill")
    void SetSlotIndex(int32 InSlotIndex) { SlotIndex = InSlotIndex; }

    UFUNCTION(BlueprintPure, Category = "Skill")
    int32 GetSlotIndex() const { return SlotIndex; }

    UFUNCTION(BlueprintPure, Category = "Skill")
    FSkillStaticData GetSkillData() const;

    UPROPERTY(BlueprintAssignable, Category = "Skill|Events")
    FOnSkillStateChanged OnStateChanged;

protected:
    UPROPERTY()
    TObjectPtr<USkillComponent> OwnerComponent;

    UPROPERTY(Replicated)
    FName SkillID;

    UPROPERTY(ReplicatedUsing = OnRep_State)
    ESkillState CurrentState;

    UPROPERTY(Replicated)
    float CooldownRemaining;

    UPROPERTY(Replicated)
    float CastTimeRemaining;

    UPROPERTY(Replicated)
    int32 CurrentCharges;

    // 关键修复：SlotIndex 也需要网络复制，客户端才能识别实例属于哪个槽位
    UPROPERTY(Replicated)
    int32 SlotIndex = -1;

    UFUNCTION()
    void OnRep_State();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    virtual void OnActivate();
    virtual void OnCastFinished();
    virtual void OnCooldownEnd();
    virtual void ExecuteEffect();

    ACharacter* GetOwnerCharacter() const;
    void SetState(ESkillState NewState);
    void StartCooldown();
    void ConsumeCharge();

    void UpdateCooldown(float DeltaTime);
    void UpdateCasting(float DeltaTime);
};