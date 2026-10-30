// CoopAdventureCharacter.cpp - 基础生命周期与核心组件管理
// 包含：构造函数、BeginPlay、Tick、网络同步基础

#include "CoopAdventureCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "GameAbilityFile/CombatComponent.h"
#include "GameAbilityFile/HealthComponent.h"
#include "InventoryFile/InventoryComponent.h"
#include "InventoryFile/InventorySubsystem.h"
#include "GameFramework/PlayerState.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"  // 如果用了 GetAllActorsOfClass
#include "AGameplayFile/TaskZone.h"  // 添加这行
#include "PlayerStateFile/CoopAdventurePlayerState.h"
#include "GameAbilityFile/SkillComponent.h"
#include "TimerManager.h"
#include "CoopAdventure.h"

// ========== 构造函数：初始化默认组件与属性 ==========
ACoopAdventureCharacter::ACoopAdventureCharacter()
{
    // 设置胶囊体大小（半径, 高度半长）
    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

    // 禁用控制器旋转影响（默认由移动方向决定角色朝向）
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    // 配置角色移动组件
    GetCharacterMovement()->bOrientRotationToMovement = true;        // 角色朝向移动方向
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // 转身速度（度/秒）
    GetCharacterMovement()->JumpZVelocity = 500.f;                   // 跳跃初速度
    GetCharacterMovement()->AirControl = 0.35f;                      // 空中控制系数（0-1）
    GetCharacterMovement()->MaxWalkSpeed = 500.f;                    // 最大行走速度
    GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;               // 模拟输入最小有效速度
    GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;     // 行走制动减速度
    GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;    // 下落制动减速度

    // 创建弹簧臂组件（摄像机吊臂）
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 400.0f;                            // 摄像机距离
    CameraBoom->bUsePawnControlRotation = true;                      // 跟随控制器旋转（鼠标控制）

    // 创建跟随摄像机
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;                   // 相对弹簧臂固定，不额外旋转

    // 创建战斗与生存组件
    HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
    CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));

    // 启用Tick更新（用于处理瞄准旋转平滑过渡）
    PrimaryActorTick.bCanEverTick = true;

    // 初始化瞄准系统状态
    bIsPrecisionAiming = false;           // 是否处于精确瞄准模式
    bIsInRotationTransition = false;      // 是否正在旋转过渡中（防止瞬间跳变）
    RotationTransitionSpeed = 360.0f;     // 旋转过渡速度（度/秒）
}

// ========== BeginPlay：游戏开始时初始化（延迟恢复外观等） ==========
void ACoopAdventureCharacter::BeginPlay()
{
    Super::BeginPlay();

    // ----- 查找次网格体（双骨骼网格体系统）-----
    // 项目使用主网格体（GetMesh）+ 次网格体（SubordinateMesh）结构
    // 次网格体用于显示角色外观（如换装后的模型），主网格体保留默认
    if (!SubordinateMesh)
    {
        TArray<USkeletalMeshComponent*> SkeletalMeshes;
        GetComponents<USkeletalMeshComponent>(SkeletalMeshes);
        for (USkeletalMeshComponent* MeshComp : SkeletalMeshes)
        {
            if (MeshComp != GetMesh())  // 排除主网格体
            {
                SubordinateMesh = MeshComp;
                break;
            }
        }
    }

    // ===== 新增：保存子网格体的初始相对位置和旋转 =====
    if (SubordinateMesh)
    {
        InitialSubMeshRelativeLocation = SubordinateMesh->GetRelativeLocation();
        InitialSubMeshRelativeRotation = SubordinateMesh->GetRelativeRotation();

        UE_LOG(LogTemp, Log, TEXT("Saved SubMesh initial relative location: %s"), *InitialSubMeshRelativeLocation.ToString());
    }


    // ----- 初始化手持物品网格体（武器显示）-----
    // 动态创建或查找现有的手持物品组件，用于显示装备中的武器
    if (!HeldItemMesh)
    {
        // 尝试查找已存在的组件（防止重复创建）
        TArray<UStaticMeshComponent*> StaticMeshes;
        GetComponents<UStaticMeshComponent>(StaticMeshes);
        for (UStaticMeshComponent* MeshComp : StaticMeshes)
        {
            if (MeshComp->GetName().Contains(TEXT("HeldItemMesh")))
            {
                HeldItemMesh = MeshComp;
                break;
            }
        }

        // 如果不存在则动态创建（运行时生成）
        if (!HeldItemMesh)
        {
            HeldItemMesh = NewObject<UStaticMeshComponent>(this, TEXT("HeldItemMesh"));
            HeldItemMesh->RegisterComponent();  // 注册到世界
            HeldItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);  // 无碰撞
            HeldItemMesh->SetVisibility(false);  // 初始隐藏（未装备时）

            // 附加到次网格体的根，后续会重新附加到"Holding_r"插槽
            if (SubordinateMesh)
            {
                HeldItemMesh->AttachToComponent(SubordinateMesh, FAttachmentTransformRules::KeepRelativeTransform);
            }
        }
    }

    // ----- 绑定背包委托（Listen Server立即执行，Client等待PlayerState复制）-----
    // 必须在BeginPlay中检查，因为PlayerState可能在不同时间点可用
    if (!bInventoryDelegatesBound && GetPlayerState())
    {
        BindInventoryDelegates();
    }

    // ----- 跨关卡外观恢复机制（多次尝试确保成功）-----
    // 使用多个定时器延迟执行，确保PlayerState、GameInstance等已完全初始化
    // 0.1秒：第一次尝试（快速恢复）
    // 0.5秒：第二次尝试（网络延迟补偿）
    // 2.0秒：最终保险（极端情况）
    FTimerHandle Handle1, Handle2, Handle3;
    GetWorldTimerManager().SetTimer(Handle1, this, &ACoopAdventureCharacter::LoadAppearanceFromGameInstance, 0.1f, false);
    GetWorldTimerManager().SetTimer(Handle2, this, &ACoopAdventureCharacter::LoadAppearanceFromGameInstance, 0.5f, false);
    GetWorldTimerManager().SetTimer(Handle3, this, &ACoopAdventureCharacter::LoadAppearanceFromGameInstance, 2.0f, false);

    // 绑定死亡事件（如果 HealthComponent 存在）
    if (HealthComponent)
    {
        HealthComponent->OnDeath.AddDynamic(this, &ACoopAdventureCharacter::OnDeath);
    }
}

void ACoopAdventureCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsPrecisionAiming)
    {
        // 调用新的瞄准旋转更新（包含Yaw平滑+Pitch跟随）
        UpdateAimingRotation(DeltaTime);
    }
    else if (bIsInRotationTransition) // 保留退出时的过渡（可选）
    {
        // 退出瞄准时的平滑回归（这段可以删除，让引擎自动处理）
        FRotator CurrentRot = GetActorRotation();
        float NewYaw = FMath::FInterpConstantTo(CurrentRot.Yaw, TargetTransitionYaw, DeltaTime, RotationTransitionSpeed);
        SetActorRotation(FRotator(CurrentRot.Pitch, NewYaw, CurrentRot.Roll));

        if (FMath::Abs(FMath::FindDeltaAngleDegrees(NewYaw, TargetTransitionYaw)) < 1.0f)
        {
            bIsInRotationTransition = false;
        }
    }
}

// ========== PossessedBy：被控制器控制时（仅Server执行） ==========
void ACoopAdventureCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);

    // Server端立即绑定背包委托（Client端在OnRep_PlayerState中处理）
    if (GetLocalRole() == ROLE_Authority)
    {
        BindInventoryDelegates();
    }
}

// ========== OnRep_PlayerState：PlayerState在Client复制到达时 ==========
void ACoopAdventureCharacter::OnRep_PlayerState()
{
    Super::OnRep_PlayerState();

    // Client端PlayerState到达时绑定背包委托（延迟绑定）
    BindInventoryDelegates();
}

// ========== GetLifetimeReplicatedProps：定义网络复制属性 ==========
void ACoopAdventureCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // 注册跨关卡外观同步属性
    // ReplicatedMesh：次网格体资源（SkeletalMesh）
    // ReplicatedAnim：动画蓝图类（AnimInstance Class）
    // 使用RepNotify（OnRep_AppearanceData）确保Client自动应用
    DOREPLIFETIME(ACoopAdventureCharacter, ReplicatedMesh);
    DOREPLIFETIME(ACoopAdventureCharacter, ReplicatedAnim);

    // 新增：瞄准系统网络同步
    DOREPLIFETIME(ACoopAdventureCharacter, ReplicatedAimPitch);
    DOREPLIFETIME(ACoopAdventureCharacter, bReplicatedIsAiming);
}


// ========== Ragdoll系统实现 ==========
void ACoopAdventureCharacter::Multicast_EnableRagdoll_Implementation()
{

    // ===== 新增：本地客户端也触发死亡输入禁用 =====
    if (IsLocallyControlled())
    {
        SetInputEnabled(false);  // 这会触发你的 BP_OnDeathInputDisable 事件
    }
    // ============================================

    UE_LOG(LogTemp, Warning, TEXT(">>> Multicast_EnableRagdoll called on %s"), *GetName());

    if (GetCharacterMovement())
    {
        GetCharacterMovement()->DisableMovement();
        GetCharacterMovement()->StopMovementImmediately();
    }

    USkeletalMeshComponent* MainMesh = GetMesh();
    if (!MainMesh) return;

    // 保存动画类
    SavedAnimClass = MainMesh->GetAnimClass();
    if (SubordinateMesh)
    {
        SavedSubordinateAnimClass = SubordinateMesh->GetAnimClass();
    }

    UE_LOG(LogTemp, Log, TEXT("Saved anim classes"));

    // 清空保存的组件数组
    RagdollSavedComponents.Empty();
    RagdollSavedProfiles.Empty();

    // 保存主Mesh配置
    RagdollSavedComponents.Add(MainMesh);
    RagdollSavedProfiles.Add(MainMesh->GetCollisionProfileName());

    // 停止动画更新并开启物理
    MainMesh->SetAnimationMode(EAnimationMode::AnimationCustomMode);
    MainMesh->SetAllBodiesSimulatePhysics(true);
    MainMesh->SetCollisionProfileName(FName("Ragdoll"));
    MainMesh->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);

    FVector RandomImpulse = FVector(
        FMath::FRandRange(-200.0f, 200.0f),
        FMath::FRandRange(-200.0f, 200.0f),
        FMath::FRandRange(100.0f, 300.0f)
    );
    MainMesh->AddImpulse(RandomImpulse, NAME_None, true);





    // 处理副Mesh
    if (SubordinateMesh)
    {
        RagdollSavedComponents.Add(SubordinateMesh);
        RagdollSavedProfiles.Add(SubordinateMesh->GetCollisionProfileName());

        if (SubordinateMesh->GetPhysicsAsset())
        {
            SubordinateMesh->SetAnimationMode(EAnimationMode::AnimationCustomMode);
            SubordinateMesh->SetAllBodiesSimulatePhysics(true);
            SubordinateMesh->SetCollisionProfileName(FName("Ragdoll"));
            SubordinateMesh->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
            SubordinateMesh->AddImpulse(RandomImpulse, NAME_None, true);
        }
        else
        {
            SubordinateMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        }
    }

    // 处理胶囊体
    UCapsuleComponent* Capsule = GetCapsuleComponent();
    if (Capsule)
    {
        RagdollSavedComponents.Add(Capsule);
        RagdollSavedProfiles.Add(Capsule->GetCollisionProfileName());
        Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    // 隐藏武器
    if (HeldItemMesh)
    {
        HeldItemMesh->SetVisibility(false);
    }

    /*
    // 调整相机
    if (IsLocallyControlled() && CameraBoom)
    {
        CameraBoom->TargetArmLength = 600.0f;
        CameraBoom->SocketOffset = FVector(0, 0, 200.0f);
    }
    */
    UE_LOG(LogTemp, Log, TEXT("Ragdoll enabled"));
}

// 7. 隐藏手持武器
/*
if (HeldItemMesh)
{
    HeldItemMesh->SetVisibility(false);
    HeldItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
*/

/*
// 8. 相机调整
if (IsLocallyControlled() && CameraBoom)
{
    CameraBoom->TargetArmLength = 600.0f;
    CameraBoom->SocketOffset = FVector(0, 0, 200.0f);
}

UE_LOG(LogTemp, Warning, TEXT(">>> Ragdoll setup complete"));
*/


void ACoopAdventureCharacter::Multicast_DisableRagdoll_Implementation()
{
    UE_LOG(LogTemp, Log, TEXT(">>> Multicast_DisableRagdoll called"));

    USkeletalMeshComponent* MainMesh = GetMesh();
    UCapsuleComponent* Capsule = GetCapsuleComponent();

    if (!MainMesh || !Capsule) return;

    // 1. 恢复胶囊体
    Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    for (int32 i = 0; i < RagdollSavedComponents.Num(); i++)
    {
        if (RagdollSavedComponents[i] == Capsule)
        {
            Capsule->SetCollisionProfileName(RagdollSavedProfiles[i]);
            break;
        }
    }

    // 2. 恢复主Mesh
    MainMesh->SetAllBodiesSimulatePhysics(false);
    MainMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

    for (int32 i = 0; i < RagdollSavedComponents.Num(); i++)
    {
        if (RagdollSavedComponents[i] == MainMesh)
        {
            MainMesh->SetCollisionProfileName(RagdollSavedProfiles[i]);
            break;
        }
    }

    // 重新附加到胶囊体
    MainMesh->AttachToComponent(Capsule, FAttachmentTransformRules::SnapToTargetIncludingScale);
    MainMesh->SetRelativeLocation(FVector(0, 0, -90.0f));
    MainMesh->SetRelativeRotation(FRotator(0, -90.0f, 0));

    // 恢复动画
    if (SavedAnimClass)
    {
        MainMesh->SetAnimInstanceClass(SavedAnimClass);
    }
    MainMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);

    // 3. 恢复副Mesh
    if (SubordinateMesh)
    {
        SubordinateMesh->SetAllBodiesSimulatePhysics(false);
        SubordinateMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

        for (int32 i = 0; i < RagdollSavedComponents.Num(); i++)
        {
            if (RagdollSavedComponents[i] == SubordinateMesh)
            {
                SubordinateMesh->SetCollisionProfileName(RagdollSavedProfiles[i]);
                break;
            }
        }

        // **关键：恢复保存的相对位置和旋转**
        UE_LOG(LogTemp, Log, TEXT("Before restore: SubMesh relative loc = %s"), *SubordinateMesh->GetRelativeLocation().ToString());
        SubordinateMesh->SetRelativeLocation(InitialSubMeshRelativeLocation);
        SubordinateMesh->SetRelativeRotation(InitialSubMeshRelativeRotation);
        UE_LOG(LogTemp, Log, TEXT("After restore: SubMesh relative loc = %s"), *SubordinateMesh->GetRelativeLocation().ToString());



        if (SavedSubordinateAnimClass)
        {
            SubordinateMesh->SetAnimInstanceClass(SavedSubordinateAnimClass);
        }
        SubordinateMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);

        
    }

    // 4. 清理
    RagdollSavedComponents.Empty();
    RagdollSavedProfiles.Empty();

    /*
    // 5. 恢复相机
    if (IsLocallyControlled() && CameraBoom)
    {
        CameraBoom->TargetArmLength = 400.0f;
        CameraBoom->SocketOffset = FVector::ZeroVector;
    }*/

    // 6. 恢复移动
    if (GetCharacterMovement())
    {
        GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    }

    UE_LOG(LogTemp, Log, TEXT("Ragdoll disabled, animation restored"));

    // ===== 新增：本地客户端恢复输入 =====
    if (IsLocallyControlled())
    {
        SetInputEnabled(true);  // 触发 BP_OnRespawnInputEnable
        UE_LOG(LogTemp, Log, TEXT("本地输入已恢复"));
    }
    // ===================================

}

void ACoopAdventureCharacter::ResetToSpawnPoint()
{
    // 只在Server执行
    if (GetLocalRole() != ROLE_Authority) return;

    // 查找PlayerStart
    AActor* PlayerStart = nullptr;
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), FoundActors);

    if (FoundActors.Num() > 0)
    {
        // 简单选择第一个，或者你可以实现随机/最近逻辑
        PlayerStart = FoundActors[0];
    }

    if (PlayerStart)
    {
        SetActorLocation(PlayerStart->GetActorLocation());
        SetActorRotation(PlayerStart->GetActorRotation());
        UE_LOG(LogTemp, Log, TEXT("Respawned at %s"), *PlayerStart->GetName());
    }
    else
    {
        // 备选：传送到原点上方
        SetActorLocation(FVector(0, 0, 200.0f));
    }


}

void ACoopAdventureCharacter::SetInputEnabled(bool bEnabled)
{
    if (AController* PC = GetController())
    {
        // 禁用/启用移动输入（WASD）
        PC->SetIgnoreMoveInput(!bEnabled);

        // 禁用/启用视角输入（鼠标）
        PC->SetIgnoreLookInput(!bEnabled);

        UE_LOG(LogTemp, Log, TEXT("Input %s for %s"),
            bEnabled ? TEXT("Enabled") : TEXT("Disabled"),
            *GetName());
    }

    // 触发蓝图事件
    if (!bEnabled)
    {
        BP_OnDeathInputDisable();  // 死亡时调用
    }
    else
    {
        BP_OnRespawnInputEnable(); // 复活时调用
    }
}



void ACoopAdventureCharacter::ActivateSkillSlot(int32 SlotIndex)
{
    if (!SkillComp)
    {
        if (APlayerState* PS = GetPlayerState())
        {
            SkillComp = PS->FindComponentByClass<USkillComponent>();
        }
    }

    if (SkillComp)
    {
        SkillComp->ActivateSkillBySlot(SlotIndex);
    }
    else
    {
    }
}

void ACoopAdventureCharacter::Server_CompleteTask_Implementation(ATaskZone* TaskZone)
{
    if (TaskZone && TaskZone->HasAuthority())
    {
        UE_LOG(LogTemp, Log, TEXT("[Character] 服务器执行完成任务，Zone: %s"), *TaskZone->GetName());
        TaskZone->Interact(this);
    }
}

void ACoopAdventureCharacter::OnDeath(AActor* Victim, AActor* Killer)
{
    // 只在服务器执行物品掉落
    if (!HasAuthority()) return;

    // 获取 PlayerState 和 InventoryComponent
    ACoopAdventurePlayerState* PS = GetPlayerState<ACoopAdventurePlayerState>();
    if (!PS) return;

    UInventoryComponent* Inventory = PS->GetInventoryComponent();
    if (Inventory)
    {
        Inventory->DropAllItems();
    }

    // 可选：在此处添加其他死亡逻辑（如分数统计、任务失败等）
}



