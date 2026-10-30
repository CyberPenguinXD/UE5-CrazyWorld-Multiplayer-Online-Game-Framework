// Copyright Epic Games, Inc. All Rights Reserved.

#include "CoopAdventureGameMode.h"
#include "PlayerStateFile/CoopAdventurePlayerState.h"

ACoopAdventureGameMode::ACoopAdventureGameMode()
{
	// stub
	PlayerStateClass = ACoopAdventurePlayerState::StaticClass();
}
