// AGameplayFile/IdentityAssignZone.cpp
#include "IdentityAssignZone.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "../CoopAdventureGameState.h"
#include "DemonManagerComponent.h"

AIdentityAssignZone::AIdentityAssignZone()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	TriggerZone = CreateDefaultSubobject<USphereComponent>(TEXT("TriggerZone"));
	RootComponent = TriggerZone;
	TriggerZone->InitSphereRadius(500.0f);
	TriggerZone->SetCollisionProfileName(FName("Trigger"));
	TriggerZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerZone->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerZone->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	ZoneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ZoneMesh"));
	ZoneMesh->SetupAttachment(RootComponent);
	ZoneMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ZoneMesh->SetVisibility(true);

	bHasDistributed = false;
}

void AIdentityAssignZone::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		// 绑定碰撞事件
		TriggerZone->OnComponentBeginOverlap.AddDynamic(this, &AIdentityAssignZone::OnTriggerOverlap);

		// 使用 Member 函数绑定，避免 Lambda 捕获 this 导致悬空
		FTimerDelegate TimerDelegate;
		TimerDelegate.BindUObject(this, &AIdentityAssignZone::CheckExistingOverlaps);

		GetWorld()->GetTimerManager().SetTimer(CheckTimerHandle, TimerDelegate, 2.0f, false);

		UE_LOG(LogTemp, Log, TEXT("[IdentityZone] 等待玩家进入... 预期人数: %d"),
			ExpectedPlayerCount > 0 ? ExpectedPlayerCount : GetExpectedPlayerCount());
	}
}

void AIdentityAssignZone::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 清理定时器，防止回调时对象已销毁
	GetWorldTimerManager().ClearTimer(CheckTimerHandle);

	// 清空数组，释放弱引用
	EnteredPlayers.Empty();

	Super::EndPlay(EndPlayReason);
}

void AIdentityAssignZone::BeginDestroy()
{
	// 额外保险：再次清理定时器
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(CheckTimerHandle);
	}

	EnteredPlayers.Empty();
	Super::BeginDestroy();
}

void AIdentityAssignZone::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AIdentityAssignZone, bHasDistributed);
}

void AIdentityAssignZone::OnTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || bHasDistributed) return;
	if (!OtherActor) return;

	ACharacter* Character = Cast<ACharacter>(OtherActor);
	if (!Character) return;

	APlayerState* PS = Character->GetPlayerState();

	// 有效性检查：确保 PlayerState 有效且未被销毁
	if (!PS || !IsValid(PS)) return;

	// 检查是否已存在
	if (IsPlayerAlreadyEntered(PS)) return;

	// 存储为弱引用（自动处理 PlayerState 销毁的情况）
	EnteredPlayers.Add(TWeakObjectPtr<APlayerState>(PS));

	int32 Expected = GetExpectedPlayerCount();
	int32 Current = GetValidEnteredCount();

	UE_LOG(LogTemp, Log, TEXT("[IdentityZone] 玩家 %s 进入区域，当前: %d/%d"),
		*PS->GetPlayerName(), Current, Expected);

	OnPlayerEnteredZone(Current, Expected);
	CheckAllPlayersEntered();
}

void AIdentityAssignZone::CheckExistingOverlaps()
{
	if (!HasAuthority() || bHasDistributed) return;

	TArray<AActor*> OverlappingActors;
	TriggerZone->GetOverlappingActors(OverlappingActors, APawn::StaticClass());

	for (AActor* Actor : OverlappingActors)
	{
		ACharacter* Character = Cast<ACharacter>(Actor);
		if (!Character) continue;

		APlayerState* PS = Character->GetPlayerState();
		if (!PS || !IsValid(PS)) continue;

		if (IsPlayerAlreadyEntered(PS)) continue;

		EnteredPlayers.Add(TWeakObjectPtr<APlayerState>(PS));
		UE_LOG(LogTemp, Log, TEXT("[IdentityZone] 发现已存在玩家: %s"), *PS->GetPlayerName());
	}

	// 检查是否全部进入
	CheckAllPlayersEntered();
}

void AIdentityAssignZone::CheckAllPlayersEntered()
{
	if (bHasDistributed || !HasAuthority()) return;

	int32 Expected = GetExpectedPlayerCount();
	int32 Current = GetValidEnteredCount();

	// 条件：已进入数 >= 预期人数 且 预期人数 > 0
	if (Expected > 0 && Current >= Expected)
	{
		UE_LOG(LogTemp, Log, TEXT("[IdentityZone] 所有玩家已进入 (%d/%d)，准备分配!"), Current, Expected);
		ExecuteDistribution();
	}
	else if (Expected == 0 && Current > 0)
	{
		// 自动检测模式
		if (AGameStateBase* GS = GetWorld()->GetGameState())
		{
			if (Current >= GS->PlayerArray.Num() && GS->PlayerArray.Num() > 0)
			{
				UE_LOG(LogTemp, Log, TEXT("[IdentityZone] 所有在线玩家已进入 (%d/%d)，准备分配!"),
					Current, GS->PlayerArray.Num());
				ExecuteDistribution();
			}
		}
	}
}

void AIdentityAssignZone::ExecuteDistribution()
{
	if (bHasDistributed) return;
	bHasDistributed = true;

	// 获取 DemonManager
	ACoopAdventureGameState* GameState = Cast<ACoopAdventureGameState>(GetWorld()->GetGameState());
	if (!GameState)
	{
		UE_LOG(LogTemp, Error, TEXT("[IdentityZone] 找不到 GameState!"));
		return;
	}

	UDemonManagerComponent* DemonManager = GameState->GetDemonManager();
	if (!DemonManager)
	{
		UE_LOG(LogTemp, Error, TEXT("[IdentityZone] 找不到 DemonManager!"));
		return;
	}

	// 调用 DemonManager 触发游戏开始
	bool bSuccess = DemonManager->TriggerGameStart(nullptr);

	if (bSuccess)
	{
		UE_LOG(LogTemp, Log, TEXT("[IdentityZone] 身份和任务分配完成!"));

		// 禁用碰撞，防止重复触发
		TriggerZone->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		OnDistributionCompleted();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[IdentityZone] DemonManager 报告分配失败"));
	}
}

void AIdentityAssignZone::ForceStartDistribution()
{
	if (!HasAuthority() || bHasDistributed) return;

	UE_LOG(LogTemp, Warning, TEXT("[IdentityZone] 管理员强制开始分配!"));
	ExecuteDistribution();
}

int32 AIdentityAssignZone::GetExpectedPlayerCount() const
{
	if (ExpectedPlayerCount > 0)
	{
		return ExpectedPlayerCount;
	}

	if (AGameStateBase* GS = GetWorld()->GetGameState())
	{
		return GS->PlayerArray.Num();
	}

	return 0;
}

bool AIdentityAssignZone::IsPlayerAlreadyEntered(APlayerState* PS) const
{
	if (!PS) return false;

	for (const TWeakObjectPtr<APlayerState>& WeakPS : EnteredPlayers)
	{
		// 使用 IsValid 检查弱引用是否仍然有效且指向同一对象
		if (WeakPS.IsValid() && WeakPS.Get() == PS) return true;
	}
	return false;
}

int32 AIdentityAssignZone::GetValidEnteredCount() const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<APlayerState>& WeakPS : EnteredPlayers)
	{
		if (WeakPS.IsValid()) Count++;
	}
	return Count;
}

void AIdentityAssignZone::OnRep_Distributed()
{
	if (bHasDistributed)
	{
		UE_LOG(LogTemp, Log, TEXT("[IdentityZone] 客户端收到：分配已完成"));
		OnDistributionCompleted();
	}
}