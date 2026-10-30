#include "PickupItem.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "InventorySubsystem.h"
#include "InventoryComponent.h"
#include "PlayerStateFile/CoopAdventurePlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"

APickupItem::APickupItem()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    SetNetUpdateFrequency(60.0f);

    // 创建组件
    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    RootComponent = MeshComponent;

    // 【关键修复】只在非CDO时设置物理属性，避免GEngine未初始化错误
    if (!HasAnyFlags(RF_ClassDefaultObject))
    {
        MeshComponent->SetCollisionProfileName(TEXT("PhysicsActor"));
        MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        MeshComponent->SetSimulatePhysics(true);
        MeshComponent->SetMassOverrideInKg(NAME_None, 100.0f, true);
        MeshComponent->SetLinearDamping(0.3f);
        MeshComponent->SetAngularDamping(0.3f);
    }

    MeshComponent->SetIsReplicated(true);

    // 交互球体（无需物理，保持原样）
    InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
    InteractionSphere->SetupAttachment(MeshComponent);
    InteractionSphere->SetSphereRadius(30.0f);
    InteractionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    InteractionSphere->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

    bRotateInWorld = false;
    RotationSpeed = 50.0f;
    Quantity = 1;

    SetReplicateMovement(true);
}


void APickupItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(APickupItem, ItemID);
    DOREPLIFETIME(APickupItem, Quantity);
}

void APickupItem::BeginPlay()
{
    Super::BeginPlay();

    // 如果生成时已有ItemID但还没加载Mesh（例如编辑器中手动设置）
    if (!ItemID.IsNone() && MeshComponent && !MeshComponent->GetStaticMesh())
    {
        LoadItemDataSafe();
    }

    // 绑定物理休眠和唤醒事件
    if (MeshComponent)
    {
        MeshComponent->OnComponentSleep.AddDynamic(this, &APickupItem::OnPhysicsSleep);
        MeshComponent->OnComponentWake.AddDynamic(this, &APickupItem::OnPhysicsWake);
    }

    // 根据初始条件决定是否开始旋转（优化：不在Tick中检查）
    if (bRotateInWorld)
    {
        StartRotation();
    }
}

void APickupItem::InitializePickup(FName InItemID, int32 InQuantity)
{
    ItemID = InItemID;
    Quantity = InQuantity;

    // 只在服务器端立即加载，客户端通过OnRep_ItemID触发
    if (HasAuthority())
    {
        LoadItemData();
    }

    UE_LOG(LogTemp, Log, TEXT("InitializePickup: %s x%d"), *ItemID.ToString(), Quantity);
}

void APickupItem::OnRep_ItemID()
{
    if (!ItemID.IsNone())
    {
        UE_LOG(LogTemp, Log, TEXT("Client received ItemID: %s"), *ItemID.ToString());
        // 延迟加载，避免在Replication回调中直接修改组件导致Ensure
        LoadItemDataSafe();
    }
}

void APickupItem::LoadItemDataSafe()
{
    // 使用弱指针防止对象被销毁后访问
    TWeakObjectPtr<APickupItem> WeakThis(this);

    // 使用FTimerDelegate而不是lambda，避免this捕获问题
    FTimerDelegate TimerDelegate;
    TimerDelegate.BindLambda([WeakThis]()
        {
            if (WeakThis.IsValid())
            {
                WeakThis->LoadItemData();
            }
        });

    // 延迟到下一帧执行，确保Replication完成且组件状态稳定
    GetWorld()->GetTimerManager().SetTimerForNextTick(TimerDelegate);
}

void APickupItem::LoadItemData()
{
    // 增强有效性检查
    if (!IsValid(this) || !IsValid(MeshComponent) || ItemID.IsNone())
    {
        return;
    }

    if (UInventorySubsystem* Subsystem = GetWorld()->GetGameInstance()->GetSubsystem<UInventorySubsystem>())
    {
        if (Subsystem->GetItemData(ItemID, CachedData))
        {
            if (CachedData.Mesh.IsValid())
            {
                // 同步加载（首次加载可能轻微卡顿，但比Ensure崩溃好）
                UStaticMesh* Mesh = CachedData.Mesh.LoadSynchronous();

                // 再次检查有效性，并确保Mesh确实不同才设置
                if (IsValid(MeshComponent) &&
                    IsValid(Mesh) &&
                    MeshComponent->GetStaticMesh() != Mesh)
                {
                    // 如果正在物理模拟，先暂停（避免物理引擎冲突）
                    bool bWasSimulating = MeshComponent->IsSimulatingPhysics();
                    if (bWasSimulating)
                    {
                        MeshComponent->SetSimulatePhysics(false);
                    }

                    // 设置新Mesh
                    MeshComponent->SetStaticMesh(Mesh);

                    // 恢复物理状态
                    if (bWasSimulating)
                    {
                        MeshComponent->SetSimulatePhysics(true);
                        MeshComponent->WakeRigidBody();
                    }

                    UE_LOG(LogTemp, Log, TEXT("Loaded mesh: %s"), *Mesh->GetName());
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Item %s has no mesh defined in DataTable"), *ItemID.ToString());
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to find item data for: %s"), *ItemID.ToString());
        }
    }
}

void APickupItem::StartRotation()
{
    // 只有需要旋转、且当前没有旋转定时器、且不在物理模拟时才启动
    if (!bRotateInWorld || !IsValid(this) || !IsValid(MeshComponent))
    {
        return;
    }

    // 如果已经在物理模拟（被扔出去了），不旋转
    if (MeshComponent->IsSimulatingPhysics())
    {
        return;
    }

    // 避免重复启动
    if (GetWorldTimerManager().IsTimerActive(RotationTimerHandle))
    {
        return;
    }

    // 每0.016秒（约60fps）执行一次旋转，而不是每帧都检查
    GetWorldTimerManager().SetTimer(
        RotationTimerHandle,
        this,
        &APickupItem::UpdateRotation,
        0.016f,  // 约60fps的间隔
        true     // 循环执行
    );

    UE_LOG(LogTemp, Verbose, TEXT("PickupItem %s started rotation"), *ItemID.ToString());
}

void APickupItem::StopRotation()
{
    if (GetWorldTimerManager().IsTimerActive(RotationTimerHandle))
    {
        GetWorldTimerManager().ClearTimer(RotationTimerHandle);
        UE_LOG(LogTemp, Verbose, TEXT("PickupItem %s stopped rotation"), *ItemID.ToString());
    }
}

void APickupItem::UpdateRotation()
{
    // 安全检查：对象是否还有效
    if (!IsValid(this) || !IsValid(MeshComponent))
    {
        StopRotation();
        return;
    }

    // 如果条件不满足，停止旋转
    if (!bRotateInWorld || MeshComponent->IsSimulatingPhysics())
    {
        StopRotation();
        return;
    }

    // 执行旋转（原Tick中的逻辑）
    FRotator NewRotation = GetActorRotation();
    NewRotation.Yaw += RotationSpeed * 0.016f; // 使用固定时间步长
    SetActorRotation(NewRotation);
}

void APickupItem::OnPhysicsSleep(UPrimitiveComponent* SleepingComponent, FName BoneName)
{
    UE_LOG(LogTemp, Verbose, TEXT("Item physics slept: %s"), *ItemID.ToString());

    // 物体静止后，如果设置需要旋转，重新开始旋转
    if (bRotateInWorld)
    {
        StartRotation();
    }
}

void APickupItem::OnPhysicsWake(UPrimitiveComponent* WakingComponent, FName BoneName)
{
    UE_LOG(LogTemp, Verbose, TEXT("Item physics woke: %s"), *ItemID.ToString());

    // 物体开始物理移动时，停止程序化的旋转（让物理接管）
    StopRotation();
}

void APickupItem::EnablePhysicsAndThrow(const FVector& ThrowDirection, float ImpulseStrength)
{
    if (!MeshComponent) return;

    // 启用物理前先停止程序化旋转
    StopRotation();

    // 启用物理模拟
    MeshComponent->SetSimulatePhysics(true);
    MeshComponent->WakeRigidBody();

    // 施加冲量（false = 作为冲量而非速度改变）
    MeshComponent->AddImpulse(ThrowDirection.GetSafeNormal() * ImpulseStrength, NAME_None, false);

    // 添加随机旋转（让投掷更自然）
    FVector RandomTorque = FMath::VRand() * ImpulseStrength * 0.5f;
    MeshComponent->AddTorqueInDegrees(RandomTorque, NAME_None, false);

    UE_LOG(LogTemp, Log, TEXT("Item thrown with impulse %.1f"), ImpulseStrength);
}

void APickupItem::SetRotateInWorld(bool bShouldRotate)
{
    bRotateInWorld = bShouldRotate;
    if (bRotateInWorld)
    {
        StartRotation();
    }
    else
    {
        StopRotation();
    }
}

FText APickupItem::GetItemName() const
{
    return CachedData.ItemName;
}

FText APickupItem::GetPickupText() const
{
    return FText::Format(
        NSLOCTEXT("Inventory", "PickupItem", "拾取 {0} x{1}"),
        CachedData.ItemName,
        FText::AsNumber(Quantity)
    );
}

void APickupItem::OnPickedUp(APlayerController* PlayerController)
{
    if (!PlayerController) return;

    ACoopAdventurePlayerState* PlayerState = PlayerController->GetPlayerState<ACoopAdventurePlayerState>();
    if (!PlayerState) return;

    UInventoryComponent* Inventory = PlayerState->GetInventoryComponent();
    if (!Inventory) return;

    if (Inventory->CanAddItem(ItemID, Quantity))
    {
        Inventory->AddItem(ItemID, Quantity);
        OnPickupSuccess();
        Destroy();
    }
}