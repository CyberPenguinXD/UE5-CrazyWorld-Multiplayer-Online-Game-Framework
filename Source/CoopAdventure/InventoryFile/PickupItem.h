// PickupItem.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "ItemData.h"
#include "PickupItem.generated.h"

UCLASS()
class COOPADVENTURE_API APickupItem : public AActor
{
    GENERATED_BODY()

public:
    APickupItem();

    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // 初始化拾取物
    UFUNCTION(BlueprintCallable, Category = "Pickup")
    void InitializePickup(FName InItemID, int32 InQuantity = 1);

    // 获取物品ID
    UFUNCTION(BlueprintPure, Category = "Pickup")
    FName GetItemID() const { return ItemID; }

    // 获取数量
    UFUNCTION(BlueprintPure, Category = "Pickup")
    int32 GetQuantity() const { return Quantity; }

    // 获取物品名称
    UFUNCTION(BlueprintPure, Category = "Pickup")
    FText GetItemName() const;

    // 获取拾取提示文本
    UFUNCTION(BlueprintPure, Category = "Pickup")
    FText GetPickupText() const;

    // 执行拾取
    UFUNCTION(BlueprintCallable, Category = "Pickup")
    void OnPickedUp(class APlayerController* PlayerController);

    // 蓝图可重写的拾取成功事件
    UFUNCTION(BlueprintImplementableEvent, Category = "Pickup")
    void OnPickupSuccess();

    // 启用物理并投掷（丢出物品用）
    UFUNCTION(BlueprintCallable, Category = "Pickup")
    void EnablePhysicsAndThrow(const FVector& ThrowDirection, float ImpulseStrength);

    // 运行时设置是否旋转（蓝图可调）
    UFUNCTION(BlueprintCallable, Category = "Pickup")
    void SetRotateInWorld(bool bShouldRotate);

protected:
    // 同步回调
    UFUNCTION()
    void OnRep_ItemID();

    // 物理休眠回调（物体静止时触发）
    UFUNCTION()
    void OnPhysicsSleep(UPrimitiveComponent* SleepingComponent, FName BoneName);

    // 物理唤醒回调（物体开始移动时触发）
    UFUNCTION()
    void OnPhysicsWake(UPrimitiveComponent* WakingComponent, FName BoneName);

    // 根据ID加载物品数据（延迟执行版本，避免在OnRep中直接修改组件）
    void LoadItemDataSafe();

    // 加载物品数据（实际执行）
    void LoadItemData();

    // 旋转控制函数（替代Tick）
    void StartRotation();
    void StopRotation();
    void UpdateRotation();

    // 延迟加载定时器句柄
    FTimerHandle DelayedLoadTimerHandle;

    // 旋转定时器句柄（替代Tick）
    FTimerHandle RotationTimerHandle;

    // 物品ID（服务器设置，同步到客户端）
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup", ReplicatedUsing = OnRep_ItemID)
    FName ItemID;

    // 数量（同步到客户端）
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup", Replicated)
    int32 Quantity;

    // 网格体组件（根组件）
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> MeshComponent;

    // 交互检测球体
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USphereComponent> InteractionSphere;

    // 旋转展示设置
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
    bool bRotateInWorld;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
    float RotationSpeed;

    // 缓存的静态数据（无需同步，客户端本地缓存）
    FItemStaticData CachedData;
};