// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/PlayerStart.h"
#include "GameAbilityFile/CombatComponent.h"
#include "GameAbilityFile/HealthComponent.h"
#include "CoopAdventureCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;
class UCombatComponent;

/**
 * 游戏主角色类
 * 支持第三人称视角、跨关卡外观同步、Ragdoll物理化、战斗系统
 */
UCLASS(abstract)
class ACoopAdventureCharacter : public ACharacter
{
	GENERATED_BODY()

	/** 摄像机弹簧臂组件（控制摄像机距离和角度） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** 跟随摄像机（实际渲染的相机） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

public:
	/** 构造函数：初始化默认组件和属性 */
	ACoopAdventureCharacter();

	/** 获取摄像机弹簧臂（供蓝图调用） */
	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** 获取跟随摄像机（供蓝图调用） */
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	// ========== 基础移动接口（蓝图可调用）==========

	/** 处理移动输入（WASD/摇杆），参数：左右、前后 */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoMove(float Right, float Forward);

	/** 处理视角输入（鼠标/右摇杆），参数：Yaw（水平）、Pitch（垂直） */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** 开始跳跃（按下跳跃键） */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoJumpStart();

	/** 结束跳跃（松开跳跃键） */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoJumpEnd();

	// ========== 跨关卡外观同步系统 ==========

	/**
	 * 保存当前外观到GameInstance
	 * 在传送/切换关卡前调用，确保复活后能恢复模型
	 */
	void SaveAppearanceToGameInstance();

	/**
	 * 从GameInstance加载外观
	 * 在新关卡BeginPlay后延迟调用，自动恢复之前的模型和动画
	 */
	void LoadAppearanceFromGameInstance();

protected:
	/**
	 * 手持物品网格体（武器显示）
	 * 动态附加到副Mesh的"Holding_r"插槽
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> HeldItemMesh;

	/**
	 * 副骨骼网格体（外观Mesh）
	 * 用于换装显示，主Mesh保留默认用于碰撞检测
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> SubordinateMesh;

	// ========== 输入配置（在蓝图中设置具体按键）==========

	/** 跳跃输入动作（IA_Jump） */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* JumpAction;

	/** 移动输入动作（IA_Move） */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* MoveAction;

	/** 手柄视角输入动作（IA_Look） */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* LookAction;

	/** 鼠标视角输入动作（IA_MouseLook） */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* MouseLookAction;


	/** 技能1输入（1键） */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* Skill1Action;

	/** 技能2输入（2键） */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* Skill2Action;

	/** 技能3输入（3键） */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* Skill3Action;

	/** 技能4输入（4键） */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* Skill4Action;

	/** 技能组件缓存（BlueprintReadOnly方便蓝图访问） */
	UPROPERTY(BlueprintReadOnly, Category = "Skill")
	TObjectPtr<class USkillComponent> SkillComp;

	// 技能输入回调（内部实现，蓝图可覆盖效果）
	UFUNCTION(BlueprintCallable, Category = "Input|Skill")
	void ActivateSkillSlot(int32 SlotIndex);

	/** 技能激活时触发（蓝图实现特效/音效） */
	UFUNCTION(BlueprintImplementableEvent, Category = "Skill|Events")
	void BP_OnSkillActivated(int32 SlotIndex, FName SkillID);

	// ========== 跨关卡外观网络同步（Replicated）==========

	/**
	 * 同步的Mesh资源（次Mesh）
	 * 用于在新关卡恢复玩家选择的角色外观
	 */
	UPROPERTY(ReplicatedUsing = OnRep_AppearanceData)
	USkeletalMesh* ReplicatedMesh;

	/**
	 * 同步的动画蓝图类
	 * 与ReplicatedMesh配套，确保动画一致
	 */
	UPROPERTY(ReplicatedUsing = OnRep_AppearanceData)
	TSubclassOf<UAnimInstance> ReplicatedAnim;

	/** 背包委托是否已绑定（防止重复绑定） */
	bool bInventoryDelegatesBound = false;

	/**
	 * 绑定背包系统的委托
	 * 在PossessedBy和OnRep_PlayerState中调用
	 */
	void BindInventoryDelegates();

	/**
	 * 装备变更回调
	 * @param SlotIndex - 装备的槽位（-1表示卸下）
	 * @param ItemID - 装备的物品ID
	 */
	UFUNCTION()
	void OnEquippedItemChanged(int32 SlotIndex, FName ItemID);

	/**
	 * 外观数据同步回调（RepNotify）
	 * 当ReplicatedMesh/ReplicatedAnim变化时自动触发，应用新外观
	 */
	UFUNCTION()
	void OnRep_AppearanceData();

	// ========== 重写引擎函数 ==========

	/** 游戏开始时初始化（查找组件、延迟恢复外观等） */
	virtual void BeginPlay() override;

	/** 每帧更新（处理瞄准旋转平滑过渡） */
	virtual void Tick(float DeltaTime) override;

	/** 绑定输入映射（Enhanced Input） */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/**
	 * 被控制器控制时调用（仅Server）
	 * 用于Server端立即绑定背包委托
	 */
	virtual void PossessedBy(AController* NewController) override;

	/**
	 * PlayerState在Client复制到达时调用
	 * 用于Client端延迟绑定背包委托
	 */
	virtual void OnRep_PlayerState() override;

	/** 注册网络复制属性 */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Enhanced Input移动回调（内部实现） */
	void Move(const FInputActionValue& Value);

	/** Enhanced Input视角回调（内部实现） */
	void Look(const FInputActionValue& Value);

public:
	/** 外观是否已恢复（防止重复加载） */
	bool bAppearanceRestored = false;

	// ========== 战斗/生存组件接口 ==========

	/** 获取生命值组件（供蓝图调用） */
	UFUNCTION(BlueprintPure, Category = "Combat")
	UHealthComponent* GetHealthComponent() const { return HealthComponent; }

protected:
	/** 生命值组件（处理伤害、死亡、复活） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UHealthComponent> HealthComponent;

public:
	/** 获取战斗组件（供蓝图调用） */
	UFUNCTION(BlueprintPure, Category = "Combat")
	class UCombatComponent* GetCombatComponent() const { return CombatComponent; }

protected:
	/** 战斗组件（处理攻击、瞄准、武器切换） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCombatComponent> CombatComponent;

public:
	/**
	 * 重新初始化战斗组件
	 * 在装备变更或组件重建时调用
	 */
	void ReinitializeCombatComponent();

	/**
	 * 请求装备指定槽位（Client->Server）
	 * 由背包UI按钮触发，通过Character转发给PlayerState的InventoryComponent
	 * @param SlotIndex - 背包槽位索引（-1表示卸下）
	 */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestEquipSlot(int32 SlotIndex);
	void Server_RequestEquipSlot_Implementation(int32 SlotIndex);
	bool Server_RequestEquipSlot_Validate(int32 SlotIndex) { return SlotIndex >= -1; }

	/** 获取手持物品网格体（供CombatComponent使用） */
	UFUNCTION(BlueprintPure, Category = "Equipment")
	UStaticMeshComponent* GetHeldItemMesh() const { return HeldItemMesh; }

	// ========== 精确瞄准系统 ==========

	/** 瞄准输入动作（IA_Aim，鼠标右键） */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* AimAction;

	/** 当前是否处于精确瞄准模式（按住右键） */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bIsPrecisionAiming = false;

	/** 瞄准时的旋转过渡速度（防止瞬间跳变） */
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float AimingRotationSpeed = 10.0f;

	/** 退出瞄准时的旋转回归速度 */
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float RotationTransitionSpeed = 360.0f;

	/** 是否正在旋转过渡中（用于平滑退出瞄准） */
	bool bIsInRotationTransition = false;

	/** 过渡目标Yaw角度 */
	float TargetTransitionYaw = 0.0f;

	/** 开始瞄准（按下右键） */
	UFUNCTION(BlueprintCallable, Category = "Input")
	void StartAiming();

	/** 停止瞄准（松开右键） */
	UFUNCTION(BlueprintCallable, Category = "Input")
	void StopAiming();

	/**
	 * 更新瞄准旋转（每帧调用）
	 * 处理身体Yaw跟随鼠标、武器Pitch跟随鼠标
	 */
	void UpdateAimingRotation(float DeltaTime);

	/** 当前手持物品的基准旋转（用于瞄准偏移计算） */
	FRotator CurrentItemBaseRotation;

	/** 当前瞄准Pitch角度（平滑插值后的值） */
	float CurrentAimPitch = 0.0f;

	/** 瞄准插值速度（越大越灵敏，推荐15-25） */
	UPROPERTY(EditDefaultsOnly, Category = "Aiming")
	float AimInterpSpeed = 20.0f;

	/** 网络同步的瞄准Pitch角度（其他玩家看到你） */
	UPROPERTY(ReplicatedUsing = OnRep_AimPitch)
	float ReplicatedAimPitch = 0.0f;

	/** 网络同步的瞄准状态 */
	UPROPERTY(Replicated)
	bool bReplicatedIsAiming = false;

	/** 上次发送的Pitch角度（Client节流用） */
	float LastSentAimPitch = 0.0f;

	/** 上次发送时间（Client节流用） */
	float LastSentAimTime = 0.0f;

	/** Server RPC：通知Server开始瞄准 */
	UFUNCTION(Server, Reliable)
	void Server_StartAiming(float InitialPitch);
	void Server_StartAiming_Implementation(float InitialPitch);

	/** Server RPC：通知Server停止瞄准 */
	UFUNCTION(Server, Reliable)
	void Server_StopAiming();
	void Server_StopAiming_Implementation();

	/** Server RPC：更新瞄准角度（Client节流发送） */
	UFUNCTION(Server, Reliable)
	void Server_UpdateAiming(float NewPitch, FRotator NewRotation);
	void Server_UpdateAiming_Implementation(float NewPitch, FRotator NewRotation);

	/** 瞄准角度同步回调（其他玩家看到你抬头/低头） */
	UFUNCTION()
	void OnRep_AimPitch();

	/**
	 * 应用瞄准旋转
	 * @param Pitch - 武器上下角度
	 * @param BodyRotation - 身体Yaw旋转
	 */
	void ApplyAimingRotation(float Pitch, const FRotator& BodyRotation);

	/** 完成瞄准过渡（平滑进入） */
	void FinishAimingTransition();

	/** 完成正常过渡（平滑退出） */
	void FinishNormalTransition();

	// ========== Ragdoll/死亡系统 ==========

	/**
	 * 死亡时启用Ragdoll（Multicast，所有客户端执行）
	 * 使身体瘫软、开启物理模拟、禁用输入
	 */
	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category = "Ragdoll")
	void Multicast_EnableRagdoll();

	/**
	 * 复活时关闭Ragdoll（Multicast，所有客户端执行）
	 * 恢复动画、关闭物理、恢复输入
	 */
	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category = "Ragdoll")
	void Multicast_DisableRagdoll();

	/** 传送到复活点（Server调用） */
	UFUNCTION(BlueprintCallable, Category = "Respawn")
	void ResetToSpawnPoint();

	/**
	 * 启用/禁用玩家输入（死亡/复活时调用）
	 * @param bEnabled - true=恢复输入，false=禁用输入
	 */
	UFUNCTION(BlueprintCallable, Category = "Input")
	void SetInputEnabled(bool bEnabled);

	/**
	 * 死亡输入禁用事件（蓝图实现）
	 * 用于显示死亡UI、显示鼠标、切换到UI输入模式
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Input")
	void BP_OnDeathInputDisable();

	/**
	 * 复活输入启用事件（蓝图实现）
	 * 用于关闭死亡UI、隐藏鼠标、恢复游戏输入模式
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Input")
	void BP_OnRespawnInputEnable();

protected:
	// ========== Ragdoll状态保存（死亡时保存，复活时恢复）==========

	/** 保存进入Ragdoll前的组件指针（用于恢复碰撞配置） */
	TArray<UPrimitiveComponent*> RagdollSavedComponents;

	/** 保存对应的碰撞配置名（与上面数组一一对应） */
	TArray<FName> RagdollSavedProfiles;

	/** 保存主Mesh的动画类（复活时恢复） */
	UPROPERTY()
	TSubclassOf<UAnimInstance> SavedAnimClass;

	/** 保存副Mesh的动画类（复活时恢复） */
	UPROPERTY()
	TSubclassOf<UAnimInstance> SavedSubordinateAnimClass;

protected:
//	FVector SavedSubMeshRelativeLocation;
//	FRotator SavedSubMeshRelativeRotation;

	UPROPERTY()
	FVector InitialSubMeshRelativeLocation;

	UPROPERTY()
	FRotator InitialSubMeshRelativeRotation;

public:
	UFUNCTION(BlueprintPure, Category = "Task")
	ATaskZone* GetCurrentTaskZone() const { return CurrentTaskZone; }

	UFUNCTION(BlueprintCallable, Category = "Task")
	void SetCurrentTaskZone(ATaskZone* NewZone) { CurrentTaskZone = NewZone; }

	UFUNCTION(BlueprintCallable, Category = "Input")
	void Interact();

protected:
	UPROPERTY()
	TObjectPtr<ATaskZone> CurrentTaskZone;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* InteractAction;

	// 当前正在进行的武器Mesh异步加载句柄
	TSharedPtr<FStreamableHandle> CurrentWeaponLoadHandle;

	// 当前正在加载的武器ID（用于验证）
	FName LoadingWeaponID;

public:
	/** 客户端请求服务器完成任务（解决TaskZone无所有权问题） */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_CompleteTask(ATaskZone* TaskZone);
	void Server_CompleteTask_Implementation(ATaskZone* TaskZone);
	bool Server_CompleteTask_Validate(ATaskZone* TaskZone) { return TaskZone != nullptr; }

protected:
	// 防止重复装备
	UPROPERTY()
	FName LastEquippedItemID = NAME_None;

protected:
	UFUNCTION()
	void OnDeath(AActor* Victim, AActor* Killer);
};