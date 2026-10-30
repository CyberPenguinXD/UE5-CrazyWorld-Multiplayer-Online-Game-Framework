// CoopAdventureCharacter_Combat.cpp - 网络同步瞄准系统（完整版）
#include "CoopAdventureCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

// ========== 开始瞄准（网络同步版）==========
void ACoopAdventureCharacter::StartAiming()
{
	if (!GetCharacterMovement() || !HeldItemMesh) return;
	if (bIsPrecisionAiming) return; // 防止重复进入

	GetCharacterMovement()->bOrientRotationToMovement = false;
	bIsPrecisionAiming = true;

	// 计算初始角度（0-360 转 -180-180）
	float RawPitch = GetControlRotation().Pitch;
	if (RawPitch > 180.0f) RawPitch -= 360.0f;
	CurrentAimPitch = FMath::Clamp(RawPitch, -45.0f, 60.0f);

	// 关键修复：不再立即设置身体旋转！
	// 只设置枪械角度，身体保持当前朝向
	// 让 UpdateAimingRotation 在 Tick 中平滑转向摄像机
	if (HeldItemMesh)
	{
		FRotator FinalRotation = CurrentItemBaseRotation;
		FinalRotation.Yaw -= CurrentAimPitch; // 使用你确认过的符号
		HeldItemMesh->SetRelativeRotation(FinalRotation);
	}

	// 网络同步部分保持不变...
	if (HasAuthority())
	{
		ReplicatedAimPitch = CurrentAimPitch;
		bReplicatedIsAiming = true;
	}
	else
	{
		Server_StartAiming(CurrentAimPitch);
	}

	// 重置发送节流记录
	LastSentAimPitch = CurrentAimPitch;
	LastSentAimTime = GetWorld()->GetTimeSeconds();
}

void ACoopAdventureCharacter::Server_StartAiming_Implementation(float InitialPitch)
{
	// 服务器执行（Listen Server的Host也会走这里）
	if (bIsPrecisionAiming) return; // 防止重复

	bIsPrecisionAiming = true;
	bReplicatedIsAiming = true;
	ReplicatedAimPitch = InitialPitch;
	CurrentAimPitch = InitialPitch;
	GetCharacterMovement()->bOrientRotationToMovement = false;

	// 服务器立即应用（权威）
	ApplyAimingRotation(CurrentAimPitch, GetActorRotation());
}

// ========== 停止瞄准（网络同步版）==========
void ACoopAdventureCharacter::StopAiming()
{
	if (!bIsPrecisionAiming) return;

	bIsPrecisionAiming = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	// 恢复基准姿态
	if (HeldItemMesh)
	{
		HeldItemMesh->SetRelativeRotation(CurrentItemBaseRotation);
	}
	CurrentAimPitch = 0.0f;

	if (HasAuthority())
	{
		bReplicatedIsAiming = false;
		ReplicatedAimPitch = 0.0f;
	}
	else
	{
		Server_StopAiming();
	}
}

void ACoopAdventureCharacter::Server_StopAiming_Implementation()
{
	bIsPrecisionAiming = false;
	bReplicatedIsAiming = false;
	ReplicatedAimPitch = 0.0f;
	CurrentAimPitch = 0.0f;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	if (HeldItemMesh)
	{
		HeldItemMesh->SetRelativeRotation(CurrentItemBaseRotation);
	}
}

// ========== 更新瞄准（带节流的网络同步）==========
void ACoopAdventureCharacter::UpdateAimingRotation(float DeltaTime)
{
	if (!bIsPrecisionAiming || !HeldItemMesh) return;

	// 计算鼠标目标角度（标准化到 -180~180）
	float RawPitch = GetControlRotation().Pitch;
	if (RawPitch > 180.0f) RawPitch -= 360.0f;
	float TargetPitch = FMath::Clamp(RawPitch, -45.0f, 60.0f);

	// ========== 武器类型检测 ==========
	bool bShouldFollowPitch = false;

	// 检查当前武器是否为枪械
	if (CombatComponent)
	{
		EAttackMode CurrentMode = CombatComponent->GetCurrentAttackMode();
		// 只有枪械才跟随鼠标上下，近战和魔法保持水平
		bShouldFollowPitch = (CurrentMode == EAttackMode::Gun);
	}

	// 只有枪械才平滑插值Pitch角度，其他武器保持0（基准姿态）
	if (bShouldFollowPitch)
	{
		CurrentAimPitch = FMath::FInterpTo(CurrentAimPitch, TargetPitch, DeltaTime, AimInterpSpeed);
	}
	else
	{
		// 近战/魔法：保持基准旋转（Pitch = 0，不跟随鼠标上下）
		// 可以选择瞬间归零或平滑归零，这里用平滑过渡更自然
		CurrentAimPitch = FMath::FInterpTo(CurrentAimPitch, 0.0f, DeltaTime, AimInterpSpeed * 2.0f);
	}

	// 身体旋转计算（水平Yaw跟随鼠标）- **所有武器都执行**
	FRotator CurrentRot = GetActorRotation();
	FRotator TargetRot(0.0f, GetControlRotation().Yaw, 0.0f);
	FRotator NewBodyRot = FMath::RInterpTo(CurrentRot, TargetRot, DeltaTime, 6.0f);

	// 本地立即应用
	ApplyAimingRotation(CurrentAimPitch, NewBodyRot);

	// ========== 网络同步（节流策略）==========
	if (!HasAuthority())
	{
		float CurrentTime = GetWorld()->GetTimeSeconds();
		bool bSignificantChange = FMath::Abs(CurrentAimPitch - LastSentAimPitch) > 2.0f;
		bool bTimeElapsed = CurrentTime - LastSentAimTime > 0.066f; // 约15fps的同步频率

		if (bSignificantChange || bTimeElapsed)
		{
			Server_UpdateAiming(CurrentAimPitch, NewBodyRot);
			LastSentAimPitch = CurrentAimPitch;
			LastSentAimTime = CurrentTime;
		}
	}
	else
	{
		// Host更新复制值，让其他客户端同步
		ReplicatedAimPitch = CurrentAimPitch;
	}
}

void ACoopAdventureCharacter::Server_UpdateAiming_Implementation(float NewPitch, FRotator NewRotation)
{
	// 服务器权威更新
	ReplicatedAimPitch = NewPitch;
	CurrentAimPitch = NewPitch;

	// 服务器执行旋转（这会复制给所有客户端）
	ApplyAimingRotation(NewPitch, NewRotation);
}

// ========== 复制回调（其他玩家看到你）==========
void ACoopAdventureCharacter::OnRep_AimPitch()
{
	// 非控制端客户端收到服务器更新的角度
	if (!IsLocallyControlled())
	{
		CurrentAimPitch = ReplicatedAimPitch;

		// 应用收到的角度（身体旋转由CharacterMovement自动同步）
		if (HeldItemMesh)
		{
			FRotator FinalRotation = CurrentItemBaseRotation;
			FinalRotation.Yaw -= CurrentAimPitch; // 注意：使用你确认的轴向和符号
			HeldItemMesh->SetRelativeRotation(FinalRotation);
		}
	}
}

// ========== 辅助函数（统一应用旋转）==========
void ACoopAdventureCharacter::ApplyAimingRotation(float Pitch, const FRotator& BodyRotation)
{
	// 设置身体朝向（权威旋转，会自动复制）
	SetActorRotation(BodyRotation);

	// 设置枪械角度
	if (HeldItemMesh)
	{
		FRotator FinalRotation = CurrentItemBaseRotation;
		FinalRotation.Yaw -= Pitch; // 注意符号：之前确认过的正确方向
		HeldItemMesh->SetRelativeRotation(FinalRotation);
	}
}