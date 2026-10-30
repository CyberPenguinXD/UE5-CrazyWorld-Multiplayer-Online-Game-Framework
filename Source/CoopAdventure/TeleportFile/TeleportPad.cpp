#include "TeleportFile/TeleportPad.h"
#include "GameFramework/PlayerController.h"
#include "CoopAdventureCharacter.h"

ATeleportPad::ATeleportPad()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
}

void ATeleportPad::ExecuteTeleport()
{
    if (!HasAuthority()) return;

    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC || !PC->IsLocalController()) return;

    // 保存所有玩家外观
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (APlayerController* PlayerC = It->Get())
        {
            if (APawn* Pawn = PlayerC->GetPawn())
            {
                if (ACoopAdventureCharacter* Char = Cast<ACoopAdventureCharacter>(Pawn))
                {
                    Char->SaveAppearanceToGameInstance();
                }
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("Saved appearances, traveling..."));

    FTimerHandle Handle;
    GetWorldTimerManager().SetTimer(Handle, this,
        &ATeleportPad::DoServerTravel, 1.0f, false);
}

void ATeleportPad::DoServerTravel()
{
    if (!HasAuthority()) return;

    FString MapName = TargetMapName.ToString();

    // 清理 World' 前缀（如果你复制的是World引用）
    if (MapName.Contains(TEXT("World'")))
    {
        MapName = MapName.Replace(TEXT("World'"), TEXT(""));
        MapName = MapName.Replace(TEXT("'"), TEXT(""));
    }

    // 清理 .MapName 后缀（如 .Map_PrisonIsland）
    if (MapName.Contains(TEXT(".")))
    {
        int32 DotIndex = MapName.Find(TEXT("."), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
        if (DotIndex != INDEX_NONE)
        {
            MapName = MapName.Left(DotIndex);
        }
    }

    // 关键修复：如果没有 /Game/ 前缀，自动补全你的实际路径
    // 支持三种填法：
    // 1. Map_PrisonIsland → 自动变成 /Game/MainFile/Map/Map_PrisonIsland
    // 2. MainFile/Map/Map_PrisonIsland → 自动变成 /Game/MainFile/Map/Map_PrisonIsland  
    // 3. /Game/MainFile/Map/Map_PrisonIsland → 保持原样
    if (!MapName.StartsWith(TEXT("/Game/")))
    {
        if (MapName.StartsWith(TEXT("MainFile/")))
        {
            MapName = FString::Printf(TEXT("/Game/%s"), *MapName);
        }
        else
        {
            MapName = FString::Printf(TEXT("/Game/MainFile/Map/%s"), *MapName);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("ServerTravel to: %s"), *MapName);

    // false, false = 客户端不中断，跟随传送
    GetWorld()->ServerTravel(MapName, false, false);
}