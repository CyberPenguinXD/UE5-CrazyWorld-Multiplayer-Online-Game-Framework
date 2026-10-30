#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "SkillFactory.generated.h"

class USkillInstance;

UCLASS()
class COOPADVENTURE_API USkillFactory : public UObject
{
    GENERATED_BODY()

public:
    // 根据SkillID创建对应的技能实例
    UFUNCTION(BlueprintCallable, Category = "Skill|Factory")
    static USkillInstance* CreateSkill(UObject* Outer, FName SkillID);
};