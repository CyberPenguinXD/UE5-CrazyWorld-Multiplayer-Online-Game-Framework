#include "FireballProjectile.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "GameAbilityFile/HealthComponent.h"
#include "GameAbilityFile/SkillSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/AssetManager.h"
#include "Net/UnrealNetwork.h"

void AFireballProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AFireballProjectile, SkillID);
}

AFireballProjectile::AFireballProjectile()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicatingMovement(true);

    Damage = 50.0f;
    ExplosionRadius = 150.0f;
    LifeTime = 5.0f;
    bExploded = false;
    SkillID = NAME_None;

    // 碰撞球体设置
    CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
    RootComponent = CollisionSphere;
    CollisionSphere->InitSphereRadius(40.0f);
    CollisionSphere->SetCollisionProfileName(FName("BlockAllDynamic"));
    CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
    CollisionSphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
    CollisionSphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
    CollisionSphere->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);

    // 投射物移动组件
    MovementComp = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("MovementComp"));
    MovementComp->UpdatedComponent = CollisionSphere;
    MovementComp->InitialSpeed = 2000.0f;
    MovementComp->MaxSpeed = 2000.0f;
    MovementComp->bRotationFollowsVelocity = true;
    MovementComp->bShouldBounce = false;
    MovementComp->ProjectileGravityScale = 0.0f;

    // 轨迹特效组件（Niagara）
    TrailSystem = CreateDefaultSubobject<UNiagaraComponent>(TEXT("TrailSystem"));
    TrailSystem->SetupAttachment(CollisionSphere);
    TrailSystem->bAutoActivate = false;

    // 绑定碰撞事件
    CollisionSphere->OnComponentHit.AddDynamic(this, &AFireballProjectile::OnHit);
}

void AFireballProjectile::BeginPlay()
{
    Super::BeginPlay();

    // 【客户端备用】如果复制早于BeginPlay（极少情况），或客户端直接生成
    // 注意：此时SkillID可能还未设置（InitProjectile在BeginPlay之后调用）
    if (!HasAuthority() && !SkillID.IsNone())
    {
       // UE_LOG(LogTemp, Log, TEXT("[FireballProjectile] BeginPlay: 客户端加载轨迹特效 [SkillID=%s]"), *SkillID.ToString());
        LoadEffectsFromSkillData();
    }

    // 服务器端设置生命周期定时器
    if (HasAuthority())
    {
        GetWorldTimerManager().SetTimer(LifeTimerHandle, this, &AFireballProjectile::OnLifeTimeEnd, LifeTime, false);
    }
}

void AFireballProjectile::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // 清理轨迹特效
    if (TrailSystem)
    {
        TrailSystem->Deactivate();
    }
    Super::EndPlay(EndPlayReason);
}

void AFireballProjectile::InitProjectile(float InDamage, float InRadius, float InSpeed, float InLifeTime, FName InSkillID)
{
    // 设置基础参数
    Damage = InDamage;
    ExplosionRadius = InRadius;
    LifeTime = InLifeTime;
    SkillID = InSkillID;

    if (MovementComp)
    {
        MovementComp->InitialSpeed = InSpeed;
        MovementComp->MaxSpeed = InSpeed;
    }

    // 【关键修复】服务器端（监听服务器/Standalone）也需要加载轨迹特效
    // 因为 InitProjectile 在 BeginPlay 之后执行，此时 SkillID 才有效
    // 排除专用服务器（不需要视觉表现）
    if (!SkillID.IsNone() && GetNetMode() != NM_DedicatedServer)
    {
        //UE_LOG(LogTemp, Log, TEXT("[FireballProjectile] InitProjectile: 服务器加载轨迹特效 [SkillID=%s, NetMode=%d]"),
         //   *SkillID.ToString(), (int32)GetNetMode());
        LoadEffectsFromSkillData();
    }

   // UE_LOG(LogTemp, Log, TEXT("[FireballProjectile] InitProjectile: 伤害=%.1f, 半径=%.1f, 速度=%.1f, 技能=%s"),
   //     InDamage, ExplosionRadius, InSpeed, *SkillID.ToString());
}

void AFireballProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    if (bExploded) return;

    // 删除以下行：if (OtherActor && OtherActor == GetInstigator()) return;

    if (HasAuthority())
    {
        DoExplode(Hit.ImpactPoint);
    }
}

void AFireballProjectile::OnLifeTimeEnd()
{
    if (bExploded) return;

    // 只在服务器处理超时爆炸
    if (HasAuthority())
    {
        DoExplode(GetActorLocation());
    }
}

void AFireballProjectile::DoExplode(const FVector& Location)
{
    if (bExploded) return;
    bExploded = true;

    // 停止移动
    if (MovementComp)
    {
        MovementComp->StopMovementImmediately();
    }

    // 广播爆炸特效（所有客户端播放）
    Multicast_PlayExplosion(Location);

    // 服务器处理伤害和销毁
    if (HasAuthority())
    {
        ApplyDamage(Location);
        SetLifeSpan(1.0f); // 1秒后销毁，给特效播放留时间
    }
}

void AFireballProjectile::Multicast_PlayExplosion_Implementation(FVector Location)
{
    // 如果 ImpactEffect/ImpactSound 尚未加载，尝试从技能数据获取
    if ((!ImpactEffect.IsValid() || !ImpactSound.IsValid()) && !SkillID.IsNone())
    {
        UGameInstance* GI = GetWorld()->GetGameInstance();
        if (GI)
        {
            USkillSubsystem* Subsystem = GI->GetSubsystem<USkillSubsystem>();
            if (Subsystem)
            {
                FSkillStaticData Data;
                if (Subsystem->GetSkillData(SkillID, Data))
                {
                    ImpactEffect = Data.ImpactEffect;
                    ImpactSound = Data.ImpactSound;
                }
            }
        }

        // 异步加载资源
        TArray<FSoftObjectPath> AssetsToLoad;
        if (!ImpactEffect.IsNull())
            AssetsToLoad.Add(ImpactEffect.ToSoftObjectPath());
        if (!ImpactSound.IsNull())
            AssetsToLoad.Add(ImpactSound.ToSoftObjectPath());

        if (AssetsToLoad.Num() > 0)
        {
            FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
            TSharedPtr<FStreamableHandle> Handle = Streamable.RequestAsyncLoad(AssetsToLoad,
                [this, Location]()
                {
                    if (!IsValid(this) || !IsValid(GetWorld())) return;
                    PlayExplosionImmediate(Location);
                }
            );
            return;
        }
    }

    // 资源已存在，直接播放
    PlayExplosionImmediate(Location);
}

void AFireballProjectile::PlayExplosionImmediate(FVector Location)
{
    // 播放爆炸特效
    if (ImpactEffect.IsValid())
    {
        UNiagaraSystem* LoadedEffect = ImpactEffect.Get();
        if (LoadedEffect != nullptr)
        {
            UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                GetWorld(),
                LoadedEffect,
                Location,
                FRotator::ZeroRotator,
                FVector(1.0f),
                true,
                true
            );
        }
    }

    // 播放爆炸音效
    if (ImpactSound.IsValid())
    {
        USoundCue* LoadedSound = ImpactSound.Get();
        if (LoadedSound != nullptr)
        {
            UGameplayStatics::PlaySoundAtLocation(GetWorld(), LoadedSound, Location);
        }
    }

    // 隐藏轨迹特效
    if (IsValid(TrailSystem))
    {
        TrailSystem->Deactivate();
    }
}

void AFireballProjectile::ApplyDamage(const FVector& Center)
{
    if (!HasAuthority()) return;

    TArray<FHitResult> HitResults;
    FCollisionShape Sphere = FCollisionShape::MakeSphere(ExplosionRadius);

    bool bHit = GetWorld()->SweepMultiByChannel(
        HitResults,
        Center, Center,
        FQuat::Identity,
        ECC_Pawn,
        Sphere
    );

    TSet<AActor*> DamagedActors;
    int32 HitCount = 0;

    for (const FHitResult& Hit : HitResults)
    {
        AActor* Target = Hit.GetActor();
        // 删除：if (!Target || Target == GetInstigator() || DamagedActors.Contains(Target)) continue;
        if (!Target || DamagedActors.Contains(Target)) continue;  // 现在只检查无效或已伤害过的目标

        DamagedActors.Add(Target);

        if (UHealthComponent* HealthComp = Target->FindComponentByClass<UHealthComponent>())
        {
            FDamageInfo DamageInfo;
            DamageInfo.BaseDamage = Damage;
            DamageInfo.Instigator = GetInstigator();
            DamageInfo.DamageType = FGameplayTag::EmptyTag;
            DamageInfo.ImpulseDirection = (Target->GetActorLocation() - Center).GetSafeNormal();

            HealthComp->TakeDamage(DamageInfo);
            HitCount++;
        }
    }

    UE_LOG(LogTemp, Log, TEXT("[FireballProjectile] 爆炸击中 %d 个目标"), HitCount);
}
void AFireballProjectile::LoadEffectsFromSkillData()
{
    // 防止重复加载
    if (bTrailLoadingStarted)
    {
        return;
    }
    bTrailLoadingStarted = true;

    UWorld* World = GetWorld();
    if (!World) return;

    UGameInstance* GI = World->GetGameInstance();
    if (!GI) return;

    USkillSubsystem* Subsystem = GI->GetSubsystem<USkillSubsystem>();
    if (!Subsystem)
    {
        UE_LOG(LogTemp, Error, TEXT("[FireballProjectile] SkillSubsystem 未找到！"));
        return;
    }

    // 从Subsystem获取技能静态数据
    FSkillStaticData Data;
    if (!Subsystem->GetSkillData(SkillID, Data))
    {
        UE_LOG(LogTemp, Error, TEXT("[FireballProjectile] 无法获取技能数据: %s"), *SkillID.ToString());
        return;
    }

   // UE_LOG(LogTemp, Log, TEXT("[FireballProjectile] 成功获取技能数据 [%s]，准备加载轨迹特效"), *SkillID.ToString());

    // 保存软引用
    TrailEffect = Data.ProjectileEffect;
    ImpactEffect = Data.ImpactEffect;
    ImpactSound = Data.ImpactSound;

    // 异步加载轨迹特效
    if (!TrailEffect.IsNull())
    {
        TArray<FSoftObjectPath> AssetsToLoad;
        AssetsToLoad.Add(TrailEffect.ToSoftObjectPath());

        FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
        TSharedPtr<FStreamableHandle> Handle = Streamable.RequestAsyncLoad(AssetsToLoad,
            [this]()
            {
                if (!IsValid(this) || !IsValid(TrailSystem)) return;

                UNiagaraSystem* LoadedEffect = TrailEffect.Get();
                if (LoadedEffect != nullptr)
                {
                    TrailSystem->SetAsset(LoadedEffect);
                    TrailSystem->Activate(true);
                   // UE_LOG(LogTemp, Log, TEXT("[FireballProjectile] 轨迹特效加载成功并激活"));
                }
                else
                {
                    //UE_LOG(LogTemp, Error, TEXT("[FireballProjectile] 轨迹特效加载失败，资源为空"));
                }
            }
        );
    }
    else
    {
       // UE_LOG(LogTemp, Warning, TEXT("[FireballProjectile] TrailEffect为空，无轨迹特效"));
        // 无轨迹特效，但还是要激活组件（可能为空）
        if (IsValid(TrailSystem))
            TrailSystem->Activate(true);
    }
}

void AFireballProjectile::OnRep_SkillID()
{
    // 当SkillID从服务器复制到客户端时触发（客户端备用路径）
    if (!HasAuthority() && !SkillID.IsNone())
    {
        UE_LOG(LogTemp, Log, TEXT("[FireballProjectile] OnRep_SkillID: 客户端通过复制接收SkillID [%s]"), *SkillID.ToString());
        LoadEffectsFromSkillData();
    }
}