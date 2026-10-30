// SkillData.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "NiagaraSystem.h"
#include "Sound/SoundCue.h"
#include "Sound/SoundAttenuation.h"
#include "SkillTypes.h"
#include "AGameplayFile/IdentityTypes.h"
#include "SkillData.generated.h"

USTRUCT(BlueprintType)
struct FSkillStaticData : public FTableRowBase
{
    GENERATED_BODY()

    FSkillStaticData()
        : SkillID(NAME_None)
        , Icon(nullptr)
        , Category(ESkillCategory::Offensive)
        , MaxLevel(1)
        , bIsUltimate(false)
        , BindingType(ESkillBindingType::Independent)
        , IdentityType(EPlayerIdentityType::None)
        , RequiredItemID(NAME_None)
        , SkillPointCost(1)
        , Cooldown(5.0f)
        , MaxCharges(-1)
        , CastTime(0.0f)
        , bCanMoveWhileCasting(true)
        , ManaCost(0.0f)
        , BaseDamage(0.0f)
        , DamagePerLevel(0.0f)
        , Range(1000.0f)
        , EffectRadius(0.0f)
        , ProjectileSpeed(2000.0f)
    {
        PrerequisiteSkills.Empty();
    }

    // ========================================
    // 基础信息
    // ========================================
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    FName SkillID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    FText SkillName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    FText Description;

    // ========================================
    // 视觉资源 - 改回软引用
    // ========================================
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    TSoftObjectPtr<UTexture2D> Icon;  // 改为软引用

    // ========================================
    // 技能分类与等级
    // ========================================
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Classification")
    ESkillCategory Category;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Classification", meta = (ClampMin = "1", ClampMax = "5"))
    int32 MaxLevel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Classification")
    bool bIsUltimate;

    // ========================================
    // 绑定配置（含身份系统）
    // ========================================
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Binding")
    ESkillBindingType BindingType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Binding|Identity")
    EPlayerIdentityType IdentityType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Binding|Item", meta = (EditCondition = "BindingType == ESkillBindingType::ItemBound"))
    FName RequiredItemID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillTree")
    int32 SkillPointCost;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillTree")
    TArray<FName> PrerequisiteSkills;

    // ========================================
    // 使用配置
    // ========================================
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Usage", meta = (ClampMin = "0"))
    float Cooldown;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Usage", meta = (ClampMin = "-1"))
    int32 MaxCharges;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Usage", meta = (ClampMin = "0"))
    float CastTime;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Usage")
    bool bCanMoveWhileCasting;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Usage", meta = (ClampMin = "0"))
    float ManaCost;

    // ========================================
    // 战斗属性
    // ========================================
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (ClampMin = "0"))
    float BaseDamage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (ClampMin = "0"))
    float DamagePerLevel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (ClampMin = "0"))
    float Range;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (ClampMin = "0"))
    float EffectRadius;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (ClampMin = "0"))
    float ProjectileSpeed;

    // ========================================
    // 视觉表现 - 全部改为软引用
    // ========================================
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    TSoftObjectPtr<UNiagaraSystem> CastEffect;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    TSoftObjectPtr<UNiagaraSystem> ProjectileEffect;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    TSoftObjectPtr<UNiagaraSystem> ImpactEffect;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    TSoftObjectPtr<UNiagaraSystem> BuffEffect;

    // ========================================
    // 音效 - 全部改为软引用
    // ========================================
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TSoftObjectPtr<USoundCue> CastSound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TSoftObjectPtr<USoundCue> ProjectileSound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TSoftObjectPtr<USoundCue> ImpactSound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TSoftObjectPtr<USoundCue> ActivationSound;

    // ========================================
    // 辅助函数 - 安全加载
    // ========================================

    /** 获取指定等级伤害 */
    float GetDamageAtLevel(int32 Level) const
    {
        Level = FMath::Clamp(Level, 1, MaxLevel);
        return BaseDamage + DamagePerLevel * (Level - 1);
    }

    /** 安全获取Icon，无效返回nullptr */
    UTexture2D* GetIconSafe() const
    {
        if (Icon.IsValid())
        {
            return Icon.LoadSynchronous();
        }
        return nullptr;
    }

    /** 检查是否有有效施法音效 */
    bool HasValidCastSound() const
    {
        return CastSound.IsValid();
    }

    /** 获取有效施法音效，空则返回nullptr */
    USoundCue* GetCastSoundSafe() const
    {
        if (CastSound.IsValid())
        {
            return CastSound.LoadSynchronous();
        }
        return nullptr;
    }

    /** 获取有效碰撞音效，空则返回nullptr */
    USoundCue* GetImpactSoundSafe() const
    {
        if (ImpactSound.IsValid())
        {
            return ImpactSound.LoadSynchronous();
        }
        return nullptr;
    }

    /** 安全获取特效 */
    UNiagaraSystem* GetCastEffectSafe() const
    {
        if (CastEffect.IsValid())
        {
            return CastEffect.LoadSynchronous();
        }
        return nullptr;
    }

    UNiagaraSystem* GetProjectileEffectSafe() const
    {
        if (ProjectileEffect.IsValid())
        {
            return ProjectileEffect.LoadSynchronous();
        }
        return nullptr;
    }

    UNiagaraSystem* GetImpactEffectSafe() const
    {
        if (ImpactEffect.IsValid())
        {
            return ImpactEffect.LoadSynchronous();
        }
        return nullptr;
    }
};