#include "CoopAdventurePlayerState.h"
#include "InventoryFile/InventoryComponent.h"
#include "GameAbilityFile/SkillComponent.h"  // 添加头文件
#include "Net/UnrealNetwork.h"  // 添加这个头文件

ACoopAdventurePlayerState::ACoopAdventurePlayerState()
{
    InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));

    // 新增：创建技能组件
    SkillComponent = CreateDefaultSubobject<USkillComponent>(TEXT("SkillComponent"));

    // 新增：创建身份组件
    IdentityComponent = CreateDefaultSubobject<UIdentityComponent>(TEXT("IdentityComponent"));

    //创建任务组件
    TaskComponent = CreateDefaultSubobject<UTaskComponent>(TEXT("TaskComponent"));
}


