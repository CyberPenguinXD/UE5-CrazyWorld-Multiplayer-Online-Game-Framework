// GameAbilityFile/IdentityTypes.h
#pragma once

#include "CoreMinimal.h"
#include "IdentityTypes.generated.h"


// 玩家身份类型（与技能系统解耦，独立定义）
UENUM(BlueprintType)
enum class EPlayerIdentityType : uint8
{
    None        UMETA(DisplayName = "未分配"),
    Human       UMETA(DisplayName = "人类/船员"),      // 普通好人
    Demon       UMETA(DisplayName = "魔族/内鬼"),      // 坏人，可以杀人
    Holy        UMETA(DisplayName = "圣职者"),         // 特殊好人，可能有额外能力
    Neutral     UMETA(DisplayName = "中立"),           // 独立胜利条件（如小丑、纵火犯）
    Spectator   UMETA(DisplayName = "观察者")          // 死亡后或旁观者
};

// 身份可见性规则
UENUM(BlueprintType)
enum class EIdentityVisibility : uint8
{
    Hidden,         // 完全隐藏（默认）
    VisibleToTeam,  // 仅队友可见（内鬼之间互相认识）
    VisibleToAll,   // 所有人可见（如死亡后暴露身份）
    VisibleToSelf   // 仅自己可见
};

// 身份数据（可用于DataTable配置身份属性）
USTRUCT(BlueprintType)
struct FIdentityData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EPlayerIdentityType IdentityType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText DisplayName;          // 显示名称（如"伪装者"）

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FLinearColor IdentityColor; // 身份颜色（UI显示用）

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxCount;             // 每局游戏最大数量（-1=无限制）

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bCanKill;              // 是否可以击杀其他玩家

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bCanUseVent;           // 是否可以使用通风口（Among Us特色）

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bHasTasks;             // 是否有任务要做（人类true，内鬼false但伪装成有）

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EIdentityVisibility DefaultVisibility;

    FIdentityData()
        : IdentityType(EPlayerIdentityType::None)
        , DisplayName(FText::FromString("未知"))
        , IdentityColor(FLinearColor::White)
        , MaxCount(-1)
        , bCanKill(false)
        , bCanUseVent(false)
        , bHasTasks(true)
        , DefaultVisibility(EIdentityVisibility::Hidden)
    {
    }
};

// 委托类型声明（放在类型文件里避免循环引用）
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnIdentityChanged, EPlayerIdentityType, NewIdentity, EPlayerIdentityType, OldIdentity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnIdentityRevealed);  // 身份被暴露时（如被投票出局）