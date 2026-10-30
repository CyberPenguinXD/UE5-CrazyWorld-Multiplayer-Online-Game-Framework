#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CharacterModelSwitcher.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class COOPADVENTURE_API UCharacterModelSwitcher : public UActorComponent
{
    GENERATED_BODY()

public:
    UCharacterModelSwitcher();

    // 要切换的子骨骼网格体组件名称（默认为 "SkeletalMesh"）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Model Switch")
    FName TargetMeshComponentName = FName("SkeletalMesh");

    // 目标骨骼网格体（模型）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Model Switch")
    TSoftObjectPtr<USkeletalMesh> TargetSkeletalMesh;

    // 目标动画蓝图类
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Model Switch")
    TSoftClassPtr<UAnimInstance> TargetAnimBlueprint;

    // ===== 新增：模型对应的昵称 =====
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Model Switch")
    FString TargetNickname;

    // 切换模型（可传入新昵称，为空则不修改昵称）
    UFUNCTION(BlueprintCallable, Category = "Model Switch")
    void SwitchModel(ACharacter* TargetCharacter, const FString& NewNickname = TEXT(""));

protected:
    virtual void BeginPlay() override;

    // 服务器 RPC（增加昵称参数）
    UFUNCTION(Server, Reliable)
    void Server_SwitchModel(ACharacter* TargetCharacter, const FString& NewNickname);

    // 广播更新外观（仅模型和动画，昵称单独修改）
    UFUNCTION(NetMulticast, Reliable)
    void Multicast_UpdateAppearance(ACharacter* TargetCharacter, USkeletalMesh* NewMesh, TSubclassOf<UAnimInstance> NewAnimClass);

    // 应用模型变化到本地
    void ApplyModelChange(ACharacter* TargetCharacter, USkeletalMesh* NewMesh, TSubclassOf<UAnimInstance> NewAnimClass);

    // 辅助函数：获取目标网格体组件
    USkeletalMeshComponent* GetTargetMeshComponent(ACharacter* TargetCharacter) const;
};