// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"
#include "Main.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Sound/SoundCue.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "Enemy.h"

AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = true;

	SkeletalMesh=CreateDefaultSubobject<USkeletalMeshComponent>("SkeletalMesh");
	SkeletalMesh->SetupAttachment(GetRootComponent());

	CombatCollsion=CreateDefaultSubobject<UBoxComponent>("CombatCollsion");
	CombatCollsion->SetupAttachment(GetRootComponent());
	CombatCollsion->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	bWeaponParticles = false;
	WeaponState=EWeaponState::EWS_Pickup;
	
	Damage=25.f;
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	InitialLocation = GetActorLocation();

	CombatCollsion->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::CombatOnOverlapBegin);
	CombatCollsion->OnComponentEndOverlap.AddDynamic(this, &AWeapon::CombatOnOverlapEnd);

	CombatCollsion->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CombatCollsion->SetCollisionObjectType(ECC_WorldDynamic);
	CombatCollsion->SetCollisionResponseToAllChannels(ECR_Ignore);
	CombatCollsion->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (WeaponState == EWeaponState::EWS_Pickup)
	{
		RunningTime += DeltaTime;

		float DeltaZ = FMath::Sin(RunningTime * FloatingSpeed) * FloatingAmplitude;

		FVector NewLocation = InitialLocation;
		NewLocation.Z += DeltaZ;

		SetActorLocation(NewLocation);

		// если хочешь вращение:
		AddActorLocalRotation(FRotator(0.f, 60.f * DeltaTime, 0.f));
	}
}


void AWeapon::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnOverlapBegin(OverlappedComponent,  OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	if (WeaponState != EWeaponState::EWS_Pickup) return;
	
	if((WeaponState==EWeaponState::EWS_Pickup)&& OtherActor)
	{
		AMain* Main = Cast<AMain>(OtherActor);
		if(Main)
		{

			Main->SetActiveOverlappingItem(this);
		}
	}
}

void AWeapon::OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	Super::OnOverlapEnd(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex);
	if (WeaponState != EWeaponState::EWS_Pickup) return;
	
	if(OtherActor)
	{
		AMain* Main = Cast<AMain>(OtherActor);
		if(Main)
		{
			Main->SetActiveOverlappingItem(nullptr);
		}
	}
}

void AWeapon::Equip(AMain* Char)
{
	if(Char)
	{
		RunningTime = 0.f;

		SetInstigator(Char->GetController());

		WeaponState = EWeaponState::EWS_Equipped;

		SkeletalMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		SkeletalMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

		//исправлено
		//CollisionVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		SkeletalMesh->SetSimulatePhysics(false);

		const USkeletalMeshSocket* RightHandSocket = Char->GetMesh()->GetSocketByName("RightHandSocket");
		if(RightHandSocket)
		{
			RightHandSocket->AttachActor(this, Char->GetMesh());
			bRotate= false;
			
			Char->SetEquippedWeapon(this);
			Char->SetActiveOverlappingItem(nullptr);
		}
		if(OnEquipSound) UGameplayStatics::PlaySound2D(this, OnEquipSound);
		IdleParticleComponent->Deactivate();
		// if(!bWeaponParticles)
		// {
		// 	IdleParticleComponent->Deactivate();
		//
		// }
	}
}

void AWeapon::CombatOnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if(OtherActor)
	{
		AEnemy* Enemy=Cast<AEnemy>(OtherActor);
		if(Enemy)
		{
			if(Enemy->HitParticles)
			{
				const USkeletalMeshSocket* WeaponSocket=SkeletalMesh->GetSocketByName("WeaponSocket");
				if(WeaponSocket)
				{
					FVector SocketLocation = WeaponSocket->GetSocketLocation(SkeletalMesh);
					UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), Enemy->HitParticles, SocketLocation, FRotator::ZeroRotator, false);
				}
				//UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), Enemy->HitParticles, GetActorLocation(), FRotator(0.0f), false);
			}
			if(Enemy->HitSound)
			{
				UGameplayStatics::PlaySound2D(this, Enemy->HitSound);
			}
			if(DamageTypeClass)
			{
				UGameplayStatics::ApplyDamage(Enemy, Damage, WeaponInstigator, this, DamageTypeClass);
			}
		}
	}
}

void AWeapon::CombatOnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	
}

void AWeapon::EnableCollision()
{
	CombatCollsion->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

void AWeapon::DeactivateCollision()
{
	CombatCollsion->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}