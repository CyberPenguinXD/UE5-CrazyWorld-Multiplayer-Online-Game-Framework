#include "CharacterModelSwitcher.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "CoopAdventureCharacter.h"
#include "CoopAdventure.h"

UCharacterModelSwitcher::UCharacterModelSwitcher()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UCharacterModelSwitcher::BeginPlay()
{
    Super::BeginPlay();
}

void UCharacterModelSwitcher::SwitchModel(ACharacter* TargetCharacter, const FString& NewNickname)
{
    if (!TargetCharacter) return;

    if (GetOwner()->HasAuthority())
    {
        // 服务器直接执行完整逻辑（包括模型加载和昵称修改）
        Server_SwitchModel_Implementation(TargetCharacter, NewNickname);
    }
    else
    {
        // 客户端通过 RPC 发送到服务器
        Server_SwitchModel(TargetCharacter, NewNickname);
    }
}

void UCharacterModelSwitcher::Server_SwitchModel_Implementation(ACharacter* TargetCharacter, const FString& NewNickname)
{
    if (!TargetCharacter) return;

    // 1. 加载模型和动画资源（同步加载，因为此函数在服务器执行，且是异步？其实同步加载可能卡顿，但资源通常已预加载）
    USkeletalMesh* LoadedMesh = TargetSkeletalMesh.LoadSynchronous();
    UClass* LoadedAnimClass = TargetAnimBlueprint.LoadSynchronous();

    if (!LoadedMesh || !LoadedAnimClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("ModelSwitcher: Failed to load target assets!"));
        return;
    }

    TSubclassOf<UAnimInstance> AnimClass = TSubclassOf<UAnimInstance>(LoadedAnimClass);

    // 2. 修改昵称（仅在服务器执行，昵称会自动复制到客户端）
    if (!NewNickname.IsEmpty())
    {
        APlayerState* PS = TargetCharacter->GetPlayerState();
        if (PS)
        {
            PS->SetPlayerName(NewNickname);
            UE_LOG(LogTemp, Log, TEXT("ModelSwitcher: Player %s nickname changed to %s"), *TargetCharacter->GetName(), *NewNickname);
        }
    }

    // 3. 广播模型更新到所有客户端
    Multicast_UpdateAppearance(TargetCharacter, LoadedMesh, AnimClass);
}

void UCharacterModelSwitcher::Multicast_UpdateAppearance_Implementation(
    ACharacter* TargetCharacter, USkeletalMesh* NewMesh, TSubclassOf<UAnimInstance> NewAnimClass)
{
    ApplyModelChange(TargetCharacter, NewMesh, NewAnimClass);
}

void UCharacterModelSwitcher::ApplyModelChange(
    ACharacter* TargetCharacter, USkeletalMesh* NewMesh, TSubclassOf<UAnimInstance> NewAnimClass)
{
    USkeletalMeshComponent* TargetMesh = GetTargetMeshComponent(TargetCharacter);
    if (!TargetMesh) return;

    // 应用新模型和动画
    TargetMesh->SetSkeletalMesh(NewMesh);
    TargetMesh->SetAnimInstanceClass(NewAnimClass);
    TargetMesh->UpdateLODStatus();

    // 如果是本地控制的角色，保存外观到 GameInstance（跨关卡恢复用）
    if (TargetCharacter && TargetCharacter->IsLocallyControlled())
    {
        if (ACoopAdventureCharacter* CoopChar = Cast<ACoopAdventureCharacter>(TargetCharacter))
        {
            CoopChar->SaveAppearanceToGameInstance();
            UE_LOG(LogTemp, Log, TEXT("Model saved on %s for %s"),
                TargetCharacter->HasAuthority() ? TEXT("Server") : TEXT("Client"),
                *TargetCharacter->GetName());
        }
    }
}

USkeletalMeshComponent* UCharacterModelSwitcher::GetTargetMeshComponent(ACharacter* TargetCharacter) const
{
    if (!TargetCharacter) return nullptr;

    USkeletalMeshComponent* CharacterMesh = TargetCharacter->GetMesh();
    if (!CharacterMesh) return nullptr;

    // 查找子骨骼网格组件
    TArray<USceneComponent*> Children;
    CharacterMesh->GetChildrenComponents(true, Children);

    for (USceneComponent* Child : Children)
    {
        USkeletalMeshComponent* ChildMesh = Cast<USkeletalMeshComponent>(Child);
        if (ChildMesh && ChildMesh->GetName().Contains(TargetMeshComponentName.ToString()))
        {
            return ChildMesh;
        }
    }

    // 未找到则返回主网格体
    return CharacterMesh;
}