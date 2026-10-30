#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SkillTypes.h"
#include "SkillData.h"
#include "SkillComponent.generated.h"

class USkillInstance;
class USkillSubsystem;

// 技能槽位结构体（只存储实例指针）
USTRUCT()
struct FSkillSlot
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<USkillInstance> SkillInstance;

	bool IsValid() const { return SkillInstance != nullptr; }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSkillCastFinished, int32, SlotIndex, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSkillSlotChanged, int32, SlotIndex, FName, SkillID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCooldownUpdated, int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSkillActivated, int32, SlotIndex, FName, SkillID);

UCLASS(ClassGroup = (Skill), meta = (BlueprintSpawnableComponent))
class COOPADVENTURE_API USkillComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USkillComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	static constexpr int32 MAX_SKILL_SLOTS = 4;

	// ========== 查询接口 ==========

	UFUNCTION(BlueprintPure, Category = "Skill")
	FName GetSkillInSlot(int32 SlotIndex) const;

	UFUNCTION(BlueprintPure, Category = "Skill")
	bool IsSlotReady(int32 SlotIndex) const;

	UFUNCTION(BlueprintPure, Category = "Skill")
	ESkillState GetSlotState(int32 SlotIndex) const;

	UFUNCTION(BlueprintPure, Category = "Skill")
	float GetCooldownPercent(int32 SlotIndex) const;

	UFUNCTION(BlueprintPure, Category = "Skill")
	float GetCooldownRemaining(int32 SlotIndex) const;

	UFUNCTION(BlueprintPure, Category = "Skill")
	int32 GetCurrentCharges(int32 SlotIndex) const;

	UFUNCTION(BlueprintPure, Category = "Skill")
	bool IsCasting(int32 SlotIndex) const;

	UFUNCTION(BlueprintPure, Category = "Skill")
	USkillInstance* GetSkillInstance(int32 SlotIndex) const;

	// ========== 技能装备 ==========

	UFUNCTION(BlueprintCallable, Category = "Skill")
	bool EquipSkill(FName SkillID, int32 SlotIndex = -1);

	UFUNCTION(BlueprintCallable, Category = "Skill")
	void UnequipSkill(int32 SlotIndex);

	// ========== 技能使用 ==========

	UFUNCTION(BlueprintCallable, Category = "Skill")
	void ActivateSkillBySlot(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "Skill|Input")
	void ActivateSlot1() { ActivateSkillBySlot(0); }
	UFUNCTION(BlueprintCallable, Category = "Skill|Input")
	void ActivateSlot2() { ActivateSkillBySlot(1); }
	UFUNCTION(BlueprintCallable, Category = "Skill|Input")
	void ActivateSlot3() { ActivateSkillBySlot(2); }
	UFUNCTION(BlueprintCallable, Category = "Skill|Input")
	void ActivateSlot4() { ActivateSkillBySlot(3); }

	// ========== 新增：冷却更新通知接口 ==========
	UFUNCTION()
	void NotifyCooldownUpdated(int32 SlotIndex);

	// ========== 事件委托 ==========

	UPROPERTY(BlueprintAssignable, Category = "Skill|Events")
	FOnSkillCastFinished OnSkillCastFinished;

	UPROPERTY(BlueprintAssignable, Category = "Skill|Events")
	FOnSkillSlotChanged OnSkillSlotChanged;

	UPROPERTY(BlueprintAssignable, Category = "Skill|Events")
	FOnCooldownUpdated OnCooldownUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Skill|Events")
	FOnSkillActivated OnSkillActivated;

protected:
	// 本地技能槽（存储实例指针）
	UPROPERTY()
	TArray<FSkillSlot> SkillSlots;

	// 关键：只复制技能ID数组（轻量级，可同步）
	UPROPERTY(ReplicatedUsing = OnRep_SkillIDs)
	TArray<FName> ReplicatedSkillIDs;

	UFUNCTION()
	void OnRep_SkillIDs();

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_ActivateSkill(int32 SlotIndex);
	void Server_ActivateSkill_Implementation(int32 SlotIndex);
	bool Server_ActivateSkill_Validate(int32 SlotIndex) { return IsValidSlot(SlotIndex); }

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_EquipSkill(FName SkillID, int32 SlotIndex);
	void Server_EquipSkill_Implementation(FName SkillID, int32 SlotIndex);
	bool Server_EquipSkill_Validate(FName SkillID, int32 SlotIndex) { return IsValidSlot(SlotIndex); }

	bool IsValidSlot(int32 SlotIndex) const { return SlotIndex >= 0 && SlotIndex < MAX_SKILL_SLOTS; }
	USkillSubsystem* GetSkillSubsystem() const;

	// 本地创建/销毁技能实例
	void LocalCreateSkillInstance(int32 SlotIndex, FName SkillID);
	void LocalDestroySkillInstance(int32 SlotIndex);
};