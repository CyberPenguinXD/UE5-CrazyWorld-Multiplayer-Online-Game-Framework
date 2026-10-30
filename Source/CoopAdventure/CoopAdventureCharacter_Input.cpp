// CoopAdventureCharacter_Input.cpp - 输入系统
#include "CoopAdventureCharacter.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "AGameplayFile/TaskZone.h"
#include "InputActionValue.h"

void ACoopAdventureCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
        EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ACoopAdventureCharacter::Move);
        EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ACoopAdventureCharacter::Look);
        EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ACoopAdventureCharacter::Look);

        if (AimAction)
        {
            EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &ACoopAdventureCharacter::StartAiming);
            EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &ACoopAdventureCharacter::StopAiming);
        }

        // ===== 新增：技能输入绑定 =====
        if (Skill1Action)
            EnhancedInputComponent->BindAction(Skill1Action, ETriggerEvent::Started, this, &ACoopAdventureCharacter::ActivateSkillSlot, 0);
        if (Skill2Action)
            EnhancedInputComponent->BindAction(Skill2Action, ETriggerEvent::Started, this, &ACoopAdventureCharacter::ActivateSkillSlot, 1);
        if (Skill3Action)
            EnhancedInputComponent->BindAction(Skill3Action, ETriggerEvent::Started, this, &ACoopAdventureCharacter::ActivateSkillSlot, 2);
        if (Skill4Action)
            EnhancedInputComponent->BindAction(Skill4Action, ETriggerEvent::Started, this, &ACoopAdventureCharacter::ActivateSkillSlot, 3);


        if (InteractAction)
        {
            EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &ACoopAdventureCharacter::Interact);
        }
    }


}

void ACoopAdventureCharacter::Move(const FInputActionValue& Value)
{
    FVector2D MovementVector = Value.Get<FVector2D>();
    DoMove(MovementVector.X, MovementVector.Y);
}

void ACoopAdventureCharacter::Look(const FInputActionValue& Value)
{
    FVector2D LookAxisVector = Value.Get<FVector2D>();
    DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void ACoopAdventureCharacter::DoMove(float Right, float Forward)
{
    if (GetController() != nullptr)
    {
        const FRotator Rotation = GetController()->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);
        const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
        AddMovementInput(ForwardDirection, Forward);
        AddMovementInput(RightDirection, Right);
    }
}

void ACoopAdventureCharacter::DoLook(float Yaw, float Pitch)
{
    if (GetController() != nullptr)
    {
        AddControllerYawInput(Yaw);
        AddControllerPitchInput(Pitch);
    }
}

void ACoopAdventureCharacter::DoJumpStart()
{
    Jump();
}

void ACoopAdventureCharacter::DoJumpEnd()
{
    StopJumping();
}

void ACoopAdventureCharacter::Interact()
{
    if (CurrentTaskZone)
    {
        UE_LOG(LogTemp, Log, TEXT("[Character] %s 按下交互键，当前区域 %s"), *GetName(), *CurrentTaskZone->GetName());
        CurrentTaskZone->Interact(this);
    }
    else
    {
        UE_LOG(LogTemp, Verbose, TEXT("[Character] %s 按下交互键，但不在任何任务区域内"), *GetName());
    }
}