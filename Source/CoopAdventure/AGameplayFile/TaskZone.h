#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "TaskZone.generated.h"

class ACoopAdventureCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTaskZoneInteract, ACoopAdventureCharacter*, InteractingCharacter);

UCLASS()
class COOPADVENTURE_API ATaskZone : public AActor
{
    GENERATED_BODY()

public:
    ATaskZone();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Task")
    FName TaskID;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
    FText InteractionPrompt;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UBoxComponent> TriggerZone;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
    bool bAutoCompleteOnOverlap = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
    bool bRequiresMinigame = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
    TSoftClassPtr<UUserWidget> MinigameClass;

    UPROPERTY(BlueprintAssignable)
    FOnTaskZoneInteract OnInteract;

    UFUNCTION()
    virtual void NotifyCharacterEnter(ACoopAdventureCharacter* Character);

    UFUNCTION()
    virtual void NotifyCharacterLeave(ACoopAdventureCharacter* Character);

    // 公开的交互函数（客户端可调用）
    UFUNCTION(BlueprintCallable, Category = "Task")
    virtual void Interact(ACoopAdventureCharacter* Character);

protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

    // 客户端请求服务器完成任务
    //UFUNCTION(Server, Reliable)
   // void Server_RequestCompleteTask(ACoopAdventureCharacter* Character);

    // 蓝图可重写的事件
    UFUNCTION(BlueprintImplementableEvent, Category = "Task")
    void OnCharacterEnter(ACoopAdventureCharacter* Character);

    UFUNCTION(BlueprintImplementableEvent, Category = "Task")
    void OnCharacterLeave(ACoopAdventureCharacter* Character);

    UFUNCTION(BlueprintImplementableEvent, Category = "Task")
    void OnCharacterInteract(ACoopAdventureCharacter* Character);
};