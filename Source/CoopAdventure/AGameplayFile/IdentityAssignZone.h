// AGameplayFile/IdentityAssignZone.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IdentityAssignZone.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class APlayerState;

UCLASS()
class COOPADVENTURE_API AIdentityAssignZone : public AActor
{
	GENERATED_BODY()

public:
	AIdentityAssignZone();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void BeginDestroy() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> TriggerZone;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> ZoneMesh;

	// 预期玩家数量（0表示自动检测）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone Config")
	int32 ExpectedPlayerCount = 0;

	// 是否已分配
	UPROPERTY(ReplicatedUsing = OnRep_Distributed)
	bool bHasDistributed;

	// 已进入区域的玩家（使用弱引用防止悬空）
	UPROPERTY()
	TArray<TWeakObjectPtr<APlayerState>> EnteredPlayers;

	// 定时器句柄（用于清理）
	FTimerHandle CheckTimerHandle;

	// 蓝图事件
	UFUNCTION(BlueprintImplementableEvent, Category = "Identity Zone")
	void OnDistributionCompleted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Identity Zone")
	void OnPlayerEnteredZone(int32 CurrentCount, int32 ExpectedCount);

	// 手动强制开始
	UFUNCTION(BlueprintCallable, Category = "Identity Zone")
	void ForceStartDistribution();

protected:
	UFUNCTION()
	void OnTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	// 核心逻辑函数（替代Lambda）
	UFUNCTION()
	void CheckExistingOverlaps();

	void CheckAllPlayersEntered();
	void ExecuteDistribution();

	// 辅助函数
	bool IsPlayerAlreadyEntered(APlayerState* PS) const;
	int32 GetValidEnteredCount() const;
	int32 GetExpectedPlayerCount() const;

	UFUNCTION()
	void OnRep_Distributed();
};