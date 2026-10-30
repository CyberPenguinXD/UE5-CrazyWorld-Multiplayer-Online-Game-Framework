#pragma once

#include "CoreMinimal.h"
#include "GameAbilityFile/SkillInstance.h"
#include "FireballSkill.generated.h"

UCLASS(ClassGroup = (Skill), meta = (BlueprintSpawnableComponent))
class COOPADVENTURE_API UFireballSkill : public USkillInstance
{
    GENERATED_BODY()

public:
    UFireballSkill();

protected:
    virtual void ExecuteEffect() override;

    // 施法特效广播（所有客户端执行）
    UFUNCTION(NetMulticast, Reliable)
    void Multicast_PlayCastEffects(FVector Location, FRotator Rotation);
    void Multicast_PlayCastEffects_Implementation(FVector Location, FRotator Rotation);

    // 立即播放特效（资源已加载时调用）
    void PlayCastEffectsImmediate(FVector Location, FRotator Rotation, const FSkillStaticData& Data);
};