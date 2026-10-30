// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "CoopAdventure.generated.h"

/** Main log category used across the project */
DECLARE_LOG_CATEGORY_EXTERN(LogCoopAdventure, Log, All);

UCLASS()
class COOPADVENTURE_API UCoopAdventureGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	// 按玩家ID存储外观（Key是玩家唯一ID，Value是模型）
	UPROPERTY()
	TMap<FString, TObjectPtr<USkeletalMesh>> PlayerSavedMeshes;

	UPROPERTY()
	TMap<FString, TSubclassOf<UAnimInstance>> PlayerSavedAnims;

	// 重写 Init 函数，在游戏启动时自动调用
	virtual void Init() override;

	// 强制加载所有关键资源（非静态成员函数）
	void ForceLoadAllAssets();

	// 新增：存储昵称
	//UPROPERTY()
	TMap<FString, FString> PlayerSavedNicknames;

};