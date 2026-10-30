// ItemData.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "NiagaraSystem.h"
#include "Sound/SoundCue.h"
#include "Sound/SoundAttenuation.h"
#include "GameplayTagContainer.h"
#include "ItemData.generated.h"

// 物品大类枚举
UENUM(BlueprintType)
enum class EItemType : uint8
{
    None        UMETA(DisplayName = "None"),
    Weapon      UMETA(DisplayName = "Weapon"),
    Consumable  UMETA(DisplayName = "Consumable"),
    Material    UMETA(DisplayName = "Material"),
    Quest       UMETA(DisplayName = "Quest"),
    Equipment   UMETA(DisplayName = "Equipment")
};

// 武器子类型枚举
UENUM(BlueprintType)
enum class EWeaponType : uint8
{
    None        UMETA(DisplayName = "None"),
    Melee       UMETA(DisplayName = "Melee"),
    Gun         UMETA(DisplayName = "Gun"),
    Magic       UMETA(DisplayName = "Magic"),
    Throwable   UMETA(DisplayName = "Throwable")
};

// 物品稀有度枚举
UENUM(BlueprintType)
enum class EItemRarity : uint8
{
    Common      UMETA(DisplayName = "Common"),
    Uncommon    UMETA(DisplayName = "Uncommon"),
    Rare        UMETA(DisplayName = "Rare"),
    Epic        UMETA(DisplayName = "Epic"),
    Legendary   UMETA(DisplayName = "Legendary")
};

// 物品静态数据（用于 Data Table）
USTRUCT(BlueprintType)
struct FItemStaticData : public FTableRowBase
{
    GENERATED_BODY()

    // ========================================
    // 基础信息
    // ========================================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    FText ItemName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    EItemType ItemType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    EWeaponType WeaponType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
    EItemRarity Rarity;

    // ========================================
    // 资源引用（软引用）
    // ========================================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    TSoftObjectPtr<UTexture2D> Icon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    TSoftObjectPtr<UStaticMesh> Mesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    TSoftObjectPtr<UStaticMesh> HeldMesh;

    // ========================================
    // 手持变换
    // ========================================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Held Transform")
    FVector HeldLocationOffset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Held Transform",
        meta = (ClampMin = "-360", ClampMax = "360"))
    FRotator HeldRotation;

    // ========================================
    // 战斗属性
    // ========================================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Stats",
        meta = (EditCondition = "ItemType == EItemType::Weapon", ClampMin = "0"))
    float BaseDamage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Stats",
        meta = (EditCondition = "ItemType == EItemType::Weapon", ClampMin = "0.1"))
    float AttackCooldown;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Stats",
        meta = (EditCondition = "ItemType == EItemType::Weapon", ClampMin = "0"))
    float AttackRange;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Gun",
        meta = (EditCondition = "WeaponType == EWeaponType::Gun"))
    FName AmmoType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Gun",
        meta = (EditCondition = "WeaponType == EWeaponType::Gun", ClampMin = "1"))
    int32 MagazineSize;

    // ========================================
    // 枪械特效配置（改为软引用）
    // ========================================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Gun|Effects",
        meta = (EditCondition = "WeaponType == EWeaponType::Gun"))
    TSoftObjectPtr<UNiagaraSystem> MuzzleFlashEffect;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Gun|Effects",
        meta = (EditCondition = "WeaponType == EWeaponType::Gun"))
    TSoftObjectPtr<UNiagaraSystem> TracerEffect;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Gun|Effects",
        meta = (EditCondition = "WeaponType == EWeaponType::Gun", ClampMin = "1000", ClampMax = "100000"))
    float TracerSpeed;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Gun|Effects",
        meta = (EditCondition = "WeaponType == EWeaponType::Gun"))
    TSoftObjectPtr<UNiagaraSystem> ImpactEffect_Default;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Gun|Effects",
        meta = (EditCondition = "WeaponType == EWeaponType::Gun"))
    TSoftObjectPtr<UMaterialInterface> ImpactDecalMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Gun|Effects",
        meta = (EditCondition = "WeaponType == EWeaponType::Gun", ClampMin = "0.1", ClampMax = "60"))
    float ImpactDecalLifeSpan;

    // ========================================
    // 枪械音效配置（改为软引用）
    // ========================================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Gun|Audio",
        meta = (EditCondition = "WeaponType == EWeaponType::Gun"))
    TArray<TSoftObjectPtr<USoundCue>> MuzzleSounds;  // 数组元素改为软引用

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Gun|Audio",
        meta = (EditCondition = "WeaponType == EWeaponType::Gun"))
    TSoftObjectPtr<USoundCue> ImpactSound_Hit;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Gun|Audio",
        meta = (EditCondition = "WeaponType == EWeaponType::Gun"))
    TSoftObjectPtr<USoundCue> ImpactSound_Miss;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Gun|Audio",
        meta = (EditCondition = "WeaponType == EWeaponType::Gun"))
    TSoftObjectPtr<USoundAttenuation> GunshotAttenuation;

    // ========================================
    // 魔法消耗
    // ========================================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Magic",
        meta = (EditCondition = "WeaponType == EWeaponType::Magic", ClampMin = "0"))
    float ManaCost;

    // ========================================
    // 近战特效配置（改为软引用）
    // ========================================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Melee|Effects",
        meta = (EditCondition = "WeaponType == EWeaponType::Melee"))
    TSoftObjectPtr<UNiagaraSystem> MeleeTrailEffect;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Melee|Effects",
        meta = (EditCondition = "WeaponType == EWeaponType::Melee"))
    TSoftObjectPtr<UNiagaraSystem> MeleeImpactEffect;

    // ========================================
    // 近战音效配置（改为软引用）
    // ========================================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Melee|Audio",
        meta = (EditCondition = "WeaponType == EWeaponType::Melee"))
    TArray<TSoftObjectPtr<USoundCue>> SwingSounds;  // 数组元素改为软引用

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Melee|Audio",
        meta = (EditCondition = "WeaponType == EWeaponType::Melee"))
    TSoftObjectPtr<USoundCue> MeleeImpactSound_Hit;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Melee|Audio",
        meta = (EditCondition = "WeaponType == EWeaponType::Melee"))
    TSoftObjectPtr<USoundCue> MeleeImpactSound_Miss;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Melee|Audio",
        meta = (EditCondition = "WeaponType == EWeaponType::Melee"))
    TSoftObjectPtr<USoundAttenuation> MeleeSwingAttenuation;

    // ========================================
    // 背包属性
    // ========================================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    int32 MaxStackSize;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    float Weight;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    int32 BaseValue;

    // ========================================
    // 使用属性
    // ========================================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Usage")
    bool bCanUse;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Usage")
    bool bCanDrop;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Usage")
    bool bCanEquip;

    // ========================================
    // 构造函数 - 软引用自动初始化为空
    // ========================================
    FItemStaticData()
        : ItemType(EItemType::None)
        , WeaponType(EWeaponType::None)
        , Rarity(EItemRarity::Common)
        , HeldLocationOffset(FVector::ZeroVector)
        , HeldRotation(FRotator::ZeroRotator)
        , BaseDamage(0.0f)
        , AttackCooldown(1.0f)
        , AttackRange(100.0f)
        , MagazineSize(30)
        , TracerSpeed(15000.0f)
        , ImpactDecalLifeSpan(5.0f)
        , ManaCost(0.0f)
        , MaxStackSize(1)
        , Weight(1.0f)
        , BaseValue(0)
        , bCanUse(false)
        , bCanDrop(true)
        , bCanEquip(false)
    {
        // 软引用（TSoftObjectPtr）会自动初始化为空，无需手动设为nullptr
        // TArray 也会自动为空
    }

    // ========================================
    // 辅助函数 - 安全加载资源
    // ========================================

    /** 安全获取图标 */
    UTexture2D* GetIconSafe() const
    {
        if (Icon.IsValid())
        {
            return Icon.LoadSynchronous();
        }
        return nullptr;
    }

    /** 安全获取Mesh */
    UStaticMesh* GetMeshSafe() const
    {
        if (Mesh.IsValid())
        {
            return Mesh.LoadSynchronous();
        }
        return nullptr;
    }

    /** 安全获取手持Mesh */
    UStaticMesh* GetHeldMeshSafe() const
    {
        if (HeldMesh.IsValid())
        {
            return HeldMesh.LoadSynchronous();
        }
        return nullptr;
    }

    /** 安全获取枪口特效 */
    UNiagaraSystem* GetMuzzleFlashEffectSafe() const
    {
        if (MuzzleFlashEffect.IsValid())
        {
            return MuzzleFlashEffect.LoadSynchronous();
        }
        return nullptr;
    }

    /** 安全获取弹道特效 */
    UNiagaraSystem* GetTracerEffectSafe() const
    {
        if (TracerEffect.IsValid())
        {
            return TracerEffect.LoadSynchronous();
        }
        return nullptr;
    }

    /** 安全获取命中特效 */
    UNiagaraSystem* GetImpactEffectSafe() const
    {
        if (ImpactEffect_Default.IsValid())
        {
            return ImpactEffect_Default.LoadSynchronous();
        }
        return nullptr;
    }

    /** 安全获取弹孔材质 */
    UMaterialInterface* GetImpactDecalMaterialSafe() const
    {
        if (ImpactDecalMaterial.IsValid())
        {
            return ImpactDecalMaterial.LoadSynchronous();
        }
        return nullptr;
    }

    /** 安全获取第一个有效射击音效 */
    USoundCue* GetFirstMuzzleSoundSafe() const
    {
        for (const auto& SoftSound : MuzzleSounds)
        {
            if (SoftSound.IsValid())
            {
                return SoftSound.LoadSynchronous();
            }
        }
        return nullptr;
    }

    /** 安全获取命中音效 */
    USoundCue* GetImpactSoundHitSafe() const
    {
        if (ImpactSound_Hit.IsValid())
        {
            return ImpactSound_Hit.LoadSynchronous();
        }
        return nullptr;
    }

    /** 安全获取未命中音效 */
    USoundCue* GetImpactSoundMissSafe() const
    {
        if (ImpactSound_Miss.IsValid())
        {
            return ImpactSound_Miss.LoadSynchronous();
        }
        return nullptr;
    }

    /** 安全获取衰减设置 */
    USoundAttenuation* GetGunshotAttenuationSafe() const
    {
        if (GunshotAttenuation.IsValid())
        {
            return GunshotAttenuation.LoadSynchronous();
        }
        return nullptr;
    }

    /** 安全获取近战 trail 特效 */
    UNiagaraSystem* GetMeleeTrailEffectSafe() const
    {
        if (MeleeTrailEffect.IsValid())
        {
            return MeleeTrailEffect.LoadSynchronous();
        }
        return nullptr;
    }

    /** 安全获取近战命中特效 */
    UNiagaraSystem* GetMeleeImpactEffectSafe() const
    {
        if (MeleeImpactEffect.IsValid())
        {
            return MeleeImpactEffect.LoadSynchronous();
        }
        return nullptr;
    }

    /** 安全获取第一个挥砍音效 */
    USoundCue* GetFirstSwingSoundSafe() const
    {
        for (const auto& SoftSound : SwingSounds)
        {
            if (SoftSound.IsValid())
            {
                return SoftSound.LoadSynchronous();
            }
        }
        return nullptr;
    }

    /** 安全获取近战命中音效 */
    USoundCue* GetMeleeImpactSoundHitSafe() const
    {
        if (MeleeImpactSound_Hit.IsValid())
        {
            return MeleeImpactSound_Hit.LoadSynchronous();
        }
        return nullptr;
    }

    /** 安全获取近战未命中音效 */
    USoundCue* GetMeleeImpactSoundMissSafe() const
    {
        if (MeleeImpactSound_Miss.IsValid())
        {
            return MeleeImpactSound_Miss.LoadSynchronous();
        }
        return nullptr;
    }

    /** 安全获取近战音效衰减 */
    USoundAttenuation* GetMeleeSwingAttenuationSafe() const
    {
        if (MeleeSwingAttenuation.IsValid())
        {
            return MeleeSwingAttenuation.LoadSynchronous();
        }
        return nullptr;
    }
};

// 背包条目
USTRUCT(BlueprintType)
struct FInventoryEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ItemID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Quantity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 SlotIndex;

    FInventoryEntry()
        : ItemID(NAME_None)
        , Quantity(0)
        , SlotIndex(-1)
    {
    }

    bool IsValid() const
    {
        return !ItemID.IsNone() && Quantity > 0;
    }
};