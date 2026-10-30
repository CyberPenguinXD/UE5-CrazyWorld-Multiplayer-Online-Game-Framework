// AGameplayFile/IdentityComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "IdentityTypes.h"
#include "IdentityComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSkillPointsChanged);

UCLASS(ClassGroup = (Identity), meta = (BlueprintSpawnableComponent))
class COOPADVENTURE_API UIdentityComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UIdentityComponent();

    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // ========== 查询接口 ==========

    UFUNCTION(BlueprintPure, Category = "Identity")
    EPlayerIdentityType GetCurrentIdentity() const { return CurrentIdentity; }

    UFUNCTION(BlueprintPure, Category = "Identity")
    bool IsImpostor() const { return CurrentIdentity == EPlayerIdentityType::Demon; }

    UFUNCTION(BlueprintPure, Category = "Identity")
    bool IsCrewmate() const { return CurrentIdentity == EPlayerIdentityType::Human || CurrentIdentity == EPlayerIdentityType::Holy; }

    UFUNCTION(BlueprintPure, Category = "Identity")
    bool CanSeeOtherIdentity(APlayerState* OtherPlayer) const;  // 声明（实现在cpp）

    UFUNCTION(BlueprintPure, Category = "SkillPoints")
    bool HasLearnedSkill(FName SkillID) const { return LearnedSkills.Contains(SkillID); }

    // ========== 技能点系统 ==========

    UFUNCTION(BlueprintPure, Category = "SkillPoints")
    int32 GetSkillPoints(EPlayerIdentityType ForIdentity) const;

    UFUNCTION(BlueprintCallable, Category = "SkillPoints")
    void AddSkillPoints(EPlayerIdentityType IdentityType, int32 Amount);

    UFUNCTION(BlueprintCallable, Category = "SkillPoints", Server, Reliable)
    void Server_LearnSkill(FName SkillID);
    void Server_LearnSkill_Implementation(FName SkillID);
    bool Server_LearnSkill_Validate(FName SkillID);

    UFUNCTION(BlueprintPure, Category = "SkillPoints")
    const TArray<FName>& GetLearnedSkills() const { return LearnedSkills; }

    // ========== 事件委托 ==========

    UPROPERTY(BlueprintAssignable, Category = "Identity|Events")
    FOnIdentityChanged OnIdentityChanged;

    UPROPERTY(BlueprintAssignable, Category = "Identity|Events")
    FOnIdentityRevealed OnIdentityRevealed;

    UPROPERTY(BlueprintAssignable, Category = "SkillPoints|Events")
    FOnSkillPointsChanged OnSkillPointsChanged;

    // ========== 服务器权限接口 ==========

    UFUNCTION(BlueprintCallable, Category = "Identity|ServerOnly")
    void AssignIdentity(EPlayerIdentityType NewIdentity);

    UFUNCTION(BlueprintCallable, Category = "Identity|ServerOnly")
    void RevealIdentity();

    UFUNCTION(BlueprintCallable, Category = "Identity|ServerOnly")
    void ResetIdentity();

    // ========== Public属性（方便其他组件访问） ==========

    UPROPERTY(ReplicatedUsing = OnRep_bIsRevealed, BlueprintReadOnly, Category = "Identity")
    bool bIsIdentityRevealed;

protected:
    UPROPERTY(ReplicatedUsing = OnRep_CurrentIdentity)
    EPlayerIdentityType CurrentIdentity;

    UPROPERTY(ReplicatedUsing = OnRep_SkillPoints)
    int32 HumanSkillPoints;

    UPROPERTY(ReplicatedUsing = OnRep_SkillPoints)
    int32 DemonSkillPoints;

    UPROPERTY(ReplicatedUsing = OnRep_SkillPoints)
    int32 SpecialSkillPoints;

    UPROPERTY(Replicated)
    TArray<FName> LearnedSkills;

    UFUNCTION()
    void OnRep_CurrentIdentity(EPlayerIdentityType OldIdentity);

    UFUNCTION()
    void OnRep_bIsRevealed();

    UFUNCTION()
    void OnRep_SkillPoints();

    bool ConsumeSkillPoints(FName SkillID);
};