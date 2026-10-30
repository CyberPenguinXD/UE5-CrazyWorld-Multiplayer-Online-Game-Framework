#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AGameplayFile/IdentityComponent.h"
#include "AGameplayFile/TaskComponent.h"
#include "CoopAdventurePlayerState.generated.h"

UCLASS()
class COOPADVENTURE_API ACoopAdventurePlayerState : public APlayerState
{
    GENERATED_BODY()

public:
    ACoopAdventurePlayerState();

	// 添加这行声明
	//virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

   // virtual void BeginPlay() override;

    UFUNCTION(BlueprintPure, Category = "Inventory")
    class UInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

    // 新增：获取技能组件
    UFUNCTION(BlueprintPure, Category = "Skill")
    class USkillComponent* GetSkillComponent() const { return SkillComponent; }

    // 新增：身份系统接口
    UFUNCTION(BlueprintPure, Category = "Identity")
    UIdentityComponent* GetIdentityComponent() const { return IdentityComponent; }

    // 添加访问函数
    UFUNCTION(BlueprintPure, Category = "Task")
    UTaskComponent* GetTaskComponent() const { return TaskComponent; }

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UInventoryComponent> InventoryComponent;

    // 新增技能组件
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USkillComponent> SkillComponent;

    // 新增：身份组件（管理技能点、身份、已学技能）
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UIdentityComponent> IdentityComponent;

    // 在类中添加
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UTaskComponent> TaskComponent;

   

};
