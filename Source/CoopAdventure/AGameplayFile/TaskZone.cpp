#include "TaskZone.h"
#include "CoopAdventureCharacter.h"
#include "PlayerStateFile/CoopAdventurePlayerState.h"
#include "AGameplayFile/TaskComponent.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

ATaskZone::ATaskZone()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;

    TriggerZone = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerZone"));
    RootComponent = TriggerZone;
    TriggerZone->SetCollisionProfileName(FName("Trigger"));
    TriggerZone->SetGenerateOverlapEvents(true);
    TriggerZone->OnComponentBeginOverlap.AddDynamic(this, &ATaskZone::OnOverlapBegin);
    TriggerZone->OnComponentEndOverlap.AddDynamic(this, &ATaskZone::OnOverlapEnd);
}

void ATaskZone::BeginPlay()
{
    Super::BeginPlay();
    UE_LOG(LogTemp, Log, TEXT("[TaskZone] %s 开始运行，TaskID=%s，AutoComplete=%d，Authority=%d"),
        *GetName(), *TaskID.ToString(), bAutoCompleteOnOverlap, HasAuthority());
}

void ATaskZone::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
    bool bFromSweep, const FHitResult& SweepResult)
{
  //  UE_LOG(LogTemp, Log, TEXT("[TaskZone] OnOverlapBegin 触发，OtherActor=%s"), OtherActor ? *OtherActor->GetName() : TEXT("nullptr"));
    ACoopAdventureCharacter* Character = Cast<ACoopAdventureCharacter>(OtherActor);
    if (Character)
    {
       // UE_LOG(LogTemp, Log, TEXT("[TaskZone] 角色 %s 进入区域 %s"), *Character->GetName(), *GetName());
        NotifyCharacterEnter(Character);
    }
}

void ATaskZone::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
   // UE_LOG(LogTemp, Log, TEXT("[TaskZone] OnOverlapEnd 触发，OtherActor=%s"), OtherActor ? *OtherActor->GetName() : TEXT("nullptr"));
    ACoopAdventureCharacter* Character = Cast<ACoopAdventureCharacter>(OtherActor);
    if (Character)
    {
       // UE_LOG(LogTemp, Log, TEXT("[TaskZone] 角色 %s 离开区域 %s"), *Character->GetName(), *GetName());
        NotifyCharacterLeave(Character);
    }
}

void ATaskZone::NotifyCharacterEnter(ACoopAdventureCharacter* Character)
{
    if (Character)
    {
        Character->SetCurrentTaskZone(this);
        OnCharacterEnter(Character);

        if (bAutoCompleteOnOverlap && HasAuthority())
        {
          //  UE_LOG(LogTemp, Log, TEXT("[TaskZone] 自动完成任务 %s for %s"), *TaskID.ToString(), *Character->GetName());
            Interact(Character);
        }
    }
}

void ATaskZone::NotifyCharacterLeave(ACoopAdventureCharacter* Character)
{
    if (Character && Character->GetCurrentTaskZone() == this)
    {
        Character->SetCurrentTaskZone(nullptr);
        OnCharacterLeave(Character);
    }
}

void ATaskZone::Interact(ACoopAdventureCharacter* Character)
{
    if (!Character)
    {
        UE_LOG(LogTemp, Warning, TEXT("[TaskZone] Interact 失败：Character为空"));
        return;
    }

   // UE_LOG(LogTemp, Log, TEXT("[TaskZone] %s 与区域 %s 交互 (TaskID=%s)，当前权限=%d"),
    //    *Character->GetName(), *GetName(), *TaskID.ToString(), HasAuthority());

    OnCharacterInteract(Character);
    OnInteract.Broadcast(Character);

    if (HasAuthority())
    {
        // 服务器直接处理（原有逻辑保持不变）
        ACoopAdventurePlayerState* PS = Character->GetPlayerState<ACoopAdventurePlayerState>();
        if (!PS) return;

        UTaskComponent* TaskComp = PS->GetTaskComponent();
        if (!TaskComp) return;

        if (bRequiresMinigame)
        {
            UE_LOG(LogTemp, Log, TEXT("[TaskZone] 任务 %s 需要小游戏"), *TaskID.ToString());
            // 此处可调用客户端打开小游戏UI
        }
        else
        {
            TaskComp->TryCompleteTask(TaskID);
            UE_LOG(LogTemp, Log, TEXT("[TaskZone] 已尝试完成任务 %s"), *TaskID.ToString());
        }
    }
    else
    {
        // 关键修改：客户端通过Character发送RPC，而不是直接调用TaskZone的RPC
        UE_LOG(LogTemp, Log, TEXT("[TaskZone] 客户端通过Character请求完成任务 %s"), *TaskID.ToString());
        Character->Server_CompleteTask(this);
    }
}
