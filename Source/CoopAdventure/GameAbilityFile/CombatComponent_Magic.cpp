#include "CombatComponent.h"
#include "GameFramework/Character.h"
#include "DrawDebugHelpers.h"
#include "GameAbilityFile/HealthComponent.h"

void UCombatComponent::PerformMagicAttack(float Damage, const FItemStaticData& WeaponData)
{
    if (GetOwnerRole() != ROLE_Authority) return;
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!Character) return;

    FVector StartLocation;
    FTransform MuzzleTransform;

    if (GetMuzzleTransform(MuzzleTransform))
    {
        StartLocation = MuzzleTransform.GetLocation();
    }
    else
    {
        FRotator UnusedRotation;
        Character->GetController()->GetPlayerViewPoint(StartLocation, UnusedRotation);
    }

    if (WeaponData.AttackRange <= 300.0f) // AOE
    {
        float Radius = WeaponData.AttackRange > 0 ? WeaponData.AttackRange : 200.0f;
        TArray<FHitResult> HitResults;
        FCollisionShape Shape;
        Shape.SetSphere(Radius);
        bool bHit = GetWorld()->SweepMultiByChannel(
            HitResults, StartLocation, StartLocation + FVector(0, 0, 0.1f),
            FQuat::Identity, ECC_Pawn, Shape);

        TSet<AActor*> DamagedActors;
        for (const FHitResult& Hit : HitResults)
        {
            AActor* Target = Hit.GetActor();
            if (Target && Target != GetOwner() && !DamagedActors.Contains(Target))
            {
                DamagedActors.Add(Target);
                if (UHealthComponent* HealthComp = Target->FindComponentByClass<UHealthComponent>())
                {
                    FDamageInfo DamageInfo;
                    DamageInfo.BaseDamage = Damage;
                    DamageInfo.Instigator = Character;
                    DamageInfo.DamageType = FGameplayTag::RequestGameplayTag(FName("Damage.Type.Magic"));
                    HealthComp->TakeDamage(DamageInfo);
                }
            }
        }
#if ENABLE_DRAW_DEBUG
        DrawDebugSphere(GetWorld(), StartLocation, Radius, 16, FColor::Purple, false, 2.0f);
#endif
    }
    else // Ô¶³ÌÉäÏß
    {
        FVector Direction = MuzzleTransform.GetRotation().GetForwardVector();
        float Range = WeaponData.AttackRange > 0 ? WeaponData.AttackRange : 1000.0f;
        FVector End = StartLocation + (Direction * Range);
        FHitResult Hit;
        FCollisionQueryParams Params;
        Params.AddIgnoredActor(GetOwner());
        if (GetWorld()->LineTraceSingleByChannel(Hit, StartLocation, End, ECC_Visibility, Params))
        {
            OnMagicHit.Broadcast(Hit.ImpactPoint, Hit.GetActor());
            if (UHealthComponent* HealthComp = Hit.GetActor()->FindComponentByClass<UHealthComponent>())
            {
                FDamageInfo DamageInfo;
                DamageInfo.BaseDamage = Damage;
                DamageInfo.Instigator = Character;
                static const FGameplayTag MagicTag = FGameplayTag::RequestGameplayTag(FName("Damage.Type.Magic"), false);
                if (MagicTag.IsValid())
                {
                    DamageInfo.DamageType = MagicTag;
                }
                else
                {
                    DamageInfo.DamageType = FGameplayTag::EmptyTag;
                }
                HealthComp->TakeDamage(DamageInfo);
            }
        }
        else OnMagicHit.Broadcast(End, nullptr);
    }
}

void UCombatComponent::SpawnMagicProjectile(float Damage, float ProjectileSpeed, float Range)
{
    UE_LOG(LogTemp, Warning, TEXT("SpawnMagicProjectile not implemented yet!"));
}