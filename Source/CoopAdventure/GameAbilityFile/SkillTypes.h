// SkillTypes.h - 添加技能分类枚举
#pragma once

#include "CoreMinimal.h"
#include "SkillTypes.generated.h"

// 原有绑定类型枚举保持不变
UENUM(BlueprintType)
enum class ESkillBindingType : uint8
{
    Independent     UMETA(DisplayName = "独立技能"),
    ItemBound       UMETA(DisplayName = "物品绑定"),
    IdentityBound   UMETA(DisplayName = "身份绑定")
};

// 新增：技能分类枚举
UENUM(BlueprintType)
enum class ESkillCategory : uint8
{
    Offensive       UMETA(DisplayName = "攻击型"),      // 火球术等造成伤害的技能
    Defensive       UMETA(DisplayName = "防御型"),      // 护盾、减伤
    Support         UMETA(DisplayName = "辅助型"),      // 治疗、加速
    Control         UMETA(DisplayName = "控制型"),      // 眩晕、冰冻
    Movement        UMETA(DisplayName = "位移型"),      // 闪现、冲刺
    Passive         UMETA(DisplayName = "被动型")       // 自动生效，无需激活
};

UENUM(BlueprintType)
enum class ESkillInputSlot : uint8
{
    None            UMETA(DisplayName = "无"),
    Slot1           UMETA(DisplayName = "1键"),
    Slot2           UMETA(DisplayName = "2键"),
    Slot3           UMETA(DisplayName = "3键"),
    Slot4           UMETA(DisplayName = "4键"),
    Passive         UMETA(DisplayName = "被动槽")
};

// 新增：玩家身份类型（用于技能身份绑定）
UENUM(BlueprintType)
enum class EPlayerIdentity : uint8
{
    None    UMETA(DisplayName = "无"),
    Demon   UMETA(DisplayName = "魔"),      // 魔族/内鬼
    Human   UMETA(DisplayName = "人"),      // 普通人
    Holy    UMETA(DisplayName = "圣"),      // 神圣/特殊身份
};