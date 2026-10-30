#include "SkillComponent.h"
#include "Net/UnrealNetwork.h"
#include "SkillInstance.h"
#include "AllSkills/FireballSkill.h"
#include "SkillSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"  // 添加这个头文件！
#include "Engine/GameInstance.h"

USkillComponent::USkillComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bTickInEditor = false;
	SetIsReplicatedByDefault(true);

	SkillSlots.SetNum(MAX_SKILL_SLOTS);
	ReplicatedSkillIDs.SetNum(MAX_SKILL_SLOTS);
}

void USkillComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AActor* Owner = GetOwner())
	{
		Owner->SetNetUpdateFrequency(100.0f);
		Owner->SetMinNetUpdateFrequency(60.0f);
	}
}

void USkillComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void USkillComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(USkillComponent, ReplicatedSkillIDs);
}

void USkillComponent::NotifyCooldownUpdated(int32 SlotIndex)
{
	if (IsValidSlot(SlotIndex))
	{
		OnCooldownUpdated.Broadcast(SlotIndex);
	}
}

void USkillComponent::OnRep_SkillIDs()
{
	UE_LOG(LogTemp, Log, TEXT("SkillComponent: OnRep_SkillIDs 被调用"));

	// 延迟一帧执行，确保网络复制的 SkillInstance 已经到达并初始化
	FTimerHandle TempHandle;
	GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
		{
			if (!IsValid(this)) return;

			// 关键修复：通过 GetOwner() 获取 Actor，然后查找所有 SkillInstance
			// 因为 SkillInstance 虽然 Outer 是 SkillComponent，但组件会附加到 Owner Actor 上
			AActor* OwnerActor = GetOwner();
			if (!OwnerActor) return;

			TArray<USkillInstance*> AllInstances;
			OwnerActor->GetComponents<USkillInstance>(AllInstances);

			UE_LOG(LogTemp, Log, TEXT("Client: 找到 %d 个 SkillInstance"), AllInstances.Num());

			for (int32 i = 0; i < MAX_SKILL_SLOTS; ++i)
			{
				FName ServerID = ReplicatedSkillIDs[i];
				FName LocalID = SkillSlots[i].IsValid() ? SkillSlots[i].SkillInstance->GetSkillID() : NAME_None;

				// 只有发生变化才处理
				if (ServerID != LocalID)
				{
					if (ServerID.IsNone())
					{
						// 卸载
						SkillSlots[i].SkillInstance = nullptr;
						UE_LOG(LogTemp, Log, TEXT("Client: 卸载槽位 %d"), i);
					}
					else
					{
						// 装备：查找网络复制过来的实例（通过 SlotIndex 匹配）
						USkillInstance* FoundInstance = nullptr;
						for (USkillInstance* Inst : AllInstances)
						{
							if (Inst && Inst->GetSlotIndex() == i && Inst->GetSkillID() == ServerID)
							{
								FoundInstance = Inst;
								break;
							}
						}

						if (FoundInstance)
						{
							SkillSlots[i].SkillInstance = FoundInstance;
							UE_LOG(LogTemp, Log, TEXT("Client: 绑定实例到槽位 %d, 技能 %s"), i, *ServerID.ToString());
						}
						else
						{
							UE_LOG(LogTemp, Warning, TEXT("Client: 未找到槽位 %d 的实例"), i);
						}
					}

					// 广播 UI 更新
					OnSkillSlotChanged.Broadcast(i, ServerID);
				}
			}
		});
}

// 服务器端创建实例（客户端通过复制自动获得）
void USkillComponent::LocalCreateSkillInstance(int32 SlotIndex, FName SkillID)
{
	if (!IsValidSlot(SlotIndex)) return;
	LocalDestroySkillInstance(SlotIndex);

	if (SkillID.IsNone()) return;

	TSubclassOf<USkillInstance> SkillClass = USkillInstance::StaticClass();
	if (SkillID.ToString().StartsWith("Fireball"))
	{
		SkillClass = UFireballSkill::StaticClass();
	}

	USkillInstance* Instance = NewObject<USkillInstance>(this, SkillClass);
	if (Instance)
	{
		Instance->RegisterComponent();
		Instance->InitializeSkill(this, SkillID);
		Instance->SetSlotIndex(SlotIndex);  // 关键：设置槽位索引，会同步到客户端
		SkillSlots[SlotIndex].SkillInstance = Instance;

		UE_LOG(LogTemp, Log, TEXT("Server: 创建技能实例 槽位%d, 技能%s"), SlotIndex, *SkillID.ToString());
	}
}

void USkillComponent::LocalDestroySkillInstance(int32 SlotIndex)
{
	if (!IsValidSlot(SlotIndex)) return;

	if (SkillSlots[SlotIndex].IsValid())
	{
		// 只在权威端（服务器）销毁组件，客户端的组件是网络复制过来的
		if (GetOwnerRole() == ROLE_Authority)
		{
			SkillSlots[SlotIndex].SkillInstance->DestroyComponent();
		}
		SkillSlots[SlotIndex].SkillInstance = nullptr;

		UE_LOG(LogTemp, Log, TEXT("销毁技能实例: 槽位%d"), SlotIndex);
	}
}

FName USkillComponent::GetSkillInSlot(int32 SlotIndex) const
{
	if (!IsValidSlot(SlotIndex)) return NAME_None;
	return ReplicatedSkillIDs.IsValidIndex(SlotIndex) ? ReplicatedSkillIDs[SlotIndex] : NAME_None;
}

bool USkillComponent::IsSlotReady(int32 SlotIndex) const
{
	if (!IsValidSlot(SlotIndex)) return false;
	if (!SkillSlots[SlotIndex].IsValid()) return false;
	return SkillSlots[SlotIndex].SkillInstance->GetState() == ESkillState::Ready;
}

ESkillState USkillComponent::GetSlotState(int32 SlotIndex) const
{
	if (!IsValidSlot(SlotIndex)) return ESkillState::Ready;
	if (!SkillSlots[SlotIndex].IsValid()) return ESkillState::Ready;
	return SkillSlots[SlotIndex].SkillInstance->GetState();
}

float USkillComponent::GetCooldownPercent(int32 SlotIndex) const
{
	if (!IsValidSlot(SlotIndex)) return 0.0f;
	if (!SkillSlots[SlotIndex].IsValid()) return 0.0f;
	return SkillSlots[SlotIndex].SkillInstance->GetCooldownPercent();
}

float USkillComponent::GetCooldownRemaining(int32 SlotIndex) const
{
	if (!IsValidSlot(SlotIndex)) return 0.0f;
	if (!SkillSlots[SlotIndex].IsValid()) return 0.0f;
	FSkillStaticData Data = SkillSlots[SlotIndex].SkillInstance->GetSkillData();
	if (Data.Cooldown <= 0.0f) return 0.0f;
	return SkillSlots[SlotIndex].SkillInstance->GetCooldownPercent() * Data.Cooldown;
}

int32 USkillComponent::GetCurrentCharges(int32 SlotIndex) const
{
	if (!IsValidSlot(SlotIndex)) return 0;
	if (!SkillSlots[SlotIndex].IsValid()) return 0;
	return SkillSlots[SlotIndex].SkillInstance->GetCurrentCharges();
}

bool USkillComponent::IsCasting(int32 SlotIndex) const
{
	if (!IsValidSlot(SlotIndex)) return false;
	if (!SkillSlots[SlotIndex].IsValid()) return false;
	return SkillSlots[SlotIndex].SkillInstance->GetState() == ESkillState::Casting;
}

USkillInstance* USkillComponent::GetSkillInstance(int32 SlotIndex) const
{
	if (!IsValidSlot(SlotIndex)) return nullptr;
	return SkillSlots[SlotIndex].SkillInstance;
}

bool USkillComponent::EquipSkill(FName SkillID, int32 SlotIndex)
{
	if (SkillID.IsNone())
	{
		UnequipSkill(SlotIndex);
		return true;
	}

	if (!GetSkillSubsystem()->HasSkillData(SkillID)) return false;

	if (SlotIndex == -1)
	{
		for (int32 i = 0; i < MAX_SKILL_SLOTS; ++i)
		{
			if (ReplicatedSkillIDs[i].IsNone())
			{
				SlotIndex = i;
				break;
			}
		}
	}

	if (!IsValidSlot(SlotIndex)) return false;

	if (GetOwnerRole() == ROLE_Authority)
	{
		Server_EquipSkill_Implementation(SkillID, SlotIndex);
		return true;
	}
	else
	{
		Server_EquipSkill(SkillID, SlotIndex);
		return true;
	}
}

void USkillComponent::UnequipSkill(int32 SlotIndex)
{
	if (!IsValidSlot(SlotIndex)) return;

	if (GetOwnerRole() == ROLE_Authority)
	{
		Server_EquipSkill_Implementation(NAME_None, SlotIndex);
	}
	else
	{
		Server_EquipSkill(NAME_None, SlotIndex);
	}
}

void USkillComponent::Server_EquipSkill_Implementation(FName SkillID, int32 SlotIndex)
{
	if (!IsValidSlot(SlotIndex)) return;

	// 更新复制数组（这会触发客户端的 OnRep_SkillIDs）
	ReplicatedSkillIDs[SlotIndex] = SkillID;

	// 服务器本地创建/销毁实例（客户端通过复制自动获得）
	if (SkillID.IsNone())
	{
		LocalDestroySkillInstance(SlotIndex);
	}
	else
	{
		LocalCreateSkillInstance(SlotIndex, SkillID);
	}

	OnSkillSlotChanged.Broadcast(SlotIndex, SkillID);
}

void USkillComponent::Server_ActivateSkill_Implementation(int32 SlotIndex)
{
	if (!IsValidSlot(SlotIndex)) return;
	if (!SkillSlots[SlotIndex].IsValid()) return;

	if (SkillSlots[SlotIndex].SkillInstance->TryActivate())
	{
		OnSkillActivated.Broadcast(SlotIndex, SkillSlots[SlotIndex].SkillInstance->GetSkillID());
	}
}

USkillSubsystem* USkillComponent::GetSkillSubsystem() const
{
	if (UGameInstance* GI = GetWorld()->GetGameInstance())
	{
		return GI->GetSubsystem<USkillSubsystem>();
	}
	return nullptr;
}

void USkillComponent::ActivateSkillBySlot(int32 SlotIndex)
{
	if (!IsValidSlot(SlotIndex)) return;

	if (!SkillSlots[SlotIndex].IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("ActivateSkillBySlot: 槽位%d 没有技能"), SlotIndex);
		return;
	}

	if (GetOwnerRole() == ROLE_Authority)
	{
		Server_ActivateSkill_Implementation(SlotIndex);
	}
	else
	{
		Server_ActivateSkill(SlotIndex);
	}
}