// CoopAdventureCharacter_Inventory.cpp - 背包与装备系统
#include "CoopAdventureCharacter.h"
#include "GameFramework/PlayerState.h"
#include "InventoryFile/InventoryComponent.h"
#include "InventoryFile/InventorySubsystem.h"
#include "GameAbilityFile/CombatComponent.h"

void ACoopAdventureCharacter::BindInventoryDelegates()
{
    if (bInventoryDelegatesBound) return;

    APlayerState* PS = GetPlayerState();
    if (!PS) return;

    UInventoryComponent* Inventory = PS->FindComponentByClass<UInventoryComponent>();
    if (!Inventory) return;

    Inventory->OnEquippedSlotChanged.AddDynamic(this, &ACoopAdventureCharacter::OnEquippedItemChanged);
    bInventoryDelegatesBound = true;

    int32 CurrentSlot = Inventory->GetEquippedSlotIndex();
    FName CurrentItem = Inventory->GetEquippedItemID();
    OnEquippedItemChanged(CurrentSlot, CurrentItem);

    ReinitializeCombatComponent();
}

void ACoopAdventureCharacter::ReinitializeCombatComponent()
{
    if (UCombatComponent* CombatComp = GetCombatComponent())
    {
        UE_LOG(LogTemp, Log, TEXT("Character通知CombatComponent重新初始化"));
        CombatComp->TryInitialize();
    }
}

void ACoopAdventureCharacter::OnEquippedItemChanged(int32 SlotIndex, FName ItemID)
{
    if (!HeldItemMesh || !SubordinateMesh) return;
    if (!SubordinateMesh->DoesSocketExist(FName("Holding_r"))) return;

    if (SlotIndex == -1 || ItemID.IsNone())
    {
        HeldItemMesh->SetVisibility(false);
        HeldItemMesh->SetStaticMesh(nullptr);
        CurrentAimPitch = 0.0f;
        return;
    }

    FAttachmentTransformRules AttachRules(
        EAttachmentRule::SnapToTarget,
        EAttachmentRule::SnapToTarget,
        EAttachmentRule::SnapToTarget,
        true
    );
    HeldItemMesh->AttachToComponent(SubordinateMesh, AttachRules, FName("Holding_r"));

    if (UInventorySubsystem* Subsystem = GetGameInstance()->GetSubsystem<UInventorySubsystem>())
    {
        FItemStaticData ItemData;
        if (Subsystem->GetItemData(ItemID, ItemData))
        {
            TSoftObjectPtr<UStaticMesh> MeshToLoad = ItemData.HeldMesh.IsValid() ? ItemData.HeldMesh : ItemData.Mesh;
            if (MeshToLoad.IsValid())
            {
                if (UStaticMesh* LoadedMesh = MeshToLoad.LoadSynchronous())
                {
                    HeldItemMesh->SetStaticMesh(LoadedMesh);

                    // 设置基准位置和旋转
                    HeldItemMesh->SetRelativeLocation(ItemData.HeldLocationOffset);
                    HeldItemMesh->SetRelativeRotation(ItemData.HeldRotation);

                    // 关键：保存基准旋转供瞄准系统使用
                    CurrentItemBaseRotation = ItemData.HeldRotation;
                    CurrentAimPitch = 0.0f; // 重置瞄准角度

                    HeldItemMesh->SetVisibility(true);
                }
            }
        }
    }
}

void ACoopAdventureCharacter::Server_RequestEquipSlot_Implementation(int32 SlotIndex)
{
    if (APlayerState* PS = GetPlayerState())
    {
        if (UInventoryComponent* InvComp = PS->FindComponentByClass<UInventoryComponent>())
        {
            InvComp->ExecuteEquipSlot(SlotIndex);
        }
    }
}