#include "SkillFactory.h"
#include "SkillInstance.h"
#include "AllSkills/FireballSkill.h"
#include "SkillSubsystem.h"
#include "Engine/GameInstance.h"

USkillInstance* USkillFactory::CreateSkill(UObject* Outer, FName SkillID)
{
    // 注意：现在 Outer 必须是 Actor 或 ActorComponent（SkillComponent）
    UGameInstance* GI = Outer->GetWorld()->GetGameInstance();
    USkillSubsystem* Subsystem = GI->GetSubsystem<USkillSubsystem>();

    FSkillStaticData Data;
    if (!Subsystem->GetSkillData(SkillID, Data))
    {
        return nullptr;
    }

    // 确定类
    TSubclassOf<USkillInstance> SkillClass = USkillInstance::StaticClass();
    if (SkillID.ToString().StartsWith("Fireball"))
    {
        SkillClass = UFireballSkill::StaticClass();
    }
    // else if...

    // 创建组件（但不注册，由调用者注册）
    USkillInstance* Instance = NewObject<USkillInstance>(Outer, SkillClass);
    return Instance;
}