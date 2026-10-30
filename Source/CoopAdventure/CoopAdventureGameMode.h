// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CoopAdventureGameMode.generated.h"

/**
 *  Simple GameMode for a third person game
 */
UCLASS(abstract)
class ACoopAdventureGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	
	/** Constructor */
	ACoopAdventureGameMode();

	//virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
};



