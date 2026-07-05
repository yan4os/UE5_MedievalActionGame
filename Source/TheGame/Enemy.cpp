// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy.h"
#include "Components/SphereComponent.h"
#include "AIController.h"
#include "Main.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "NavigationSystem.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Sound/SoundCue.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Main.h"
#include "TimerManager.h"
#include "Chaos/DebugDrawCommand.h"
#include "MainPlayerController.h"
#include "Engine/EngineTypes.h"

// Sets default values
AEnemy::AEnemy()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	AgroSphere=CreateDefaultSubobject<USphereComponent>("AgroSphere");
	AgroSphere->SetupAttachment(GetRootComponent());
	AgroSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Ignore);
	AgroSphere->InitSphereRadius(600.f);

	CombatCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("CombatCollision"));
	CombatCollision->SetupAttachment(GetMesh(), FName("EnemySocket"));

	CombatSphere=CreateDefaultSubobject<USphereComponent>("CombatSphere");
	CombatSphere->SetupAttachment(GetRootComponent());
	CombatSphere->InitSphereRadius(75.f);

	bOverlappingCombatSphere=false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);

	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	EnemyMovementStatus = EEnemyMovementStatus::EMS_Idle;

	Health=100.f;
	MaxHealth=100.f;
	Damage=10.f;

	AttackMinTime = 0.5f;
	AttackMaxTime = 1.5f;

	InterpSpeed=15.f;
	bInterpToMain=false;

	EnemyMovementStatus = EEnemyMovementStatus::EMS_Idle;

	DeathDelay=3.0f;
	bHasValidTarget = false;
}

// Called when the game starts or when spawned
void AEnemy::BeginPlay()
{
	Super::BeginPlay();
	AIController = Cast<AAIController>(GetController());

	AgroSphere->OnComponentBeginOverlap.AddDynamic(this, &AEnemy::AgroSphereOnOverlapBegin);
	AgroSphere->OnComponentEndOverlap.AddDynamic(this, &AEnemy::AgroSphereOnOverlapEnd);

	CombatSphere->OnComponentBeginOverlap.AddDynamic(this, &AEnemy::CombatSphereOnOverlapBegin);
	CombatSphere->OnComponentEndOverlap.AddDynamic(this, &AEnemy::CombatSphereOnOverlapEnd);

	CombatCollision->OnComponentBeginOverlap.AddDynamic(this, &AEnemy::CombatOnOverlapBegin);
	CombatCollision->OnComponentEndOverlap.AddDynamic(this, &AEnemy::CombatOnOverlapEnd);

	CombatCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CombatCollision->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	CombatCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	CombatCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
}

// Called every frame
void AEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if(bOverlappingCombatSphere && EnemyMovementStatus != EEnemyMovementStatus::EMS_Attacking)
	{
		SetEnemyMovementStatus(EEnemyMovementStatus::EMS_Attacking);
	}

	if(bInterpToMain && CombatTarget)
	{
		FRotator LookAtYaw = GetLookAtRotationYaw(CombatTarget->GetActorLocation());
		FRotator InterpRotation = FMath::RInterpTo(GetActorRotation(), LookAtYaw, DeltaTime, InterpSpeed);

		SetActorRotation(InterpRotation);
	}

	// if(CombatTarget)
	// {
	// 	float Distance = FVector::Dist(CombatTarget->GetActorLocation(), GetActorLocation());
	//
	// 	if(Distance > 150.f) // если игрок снова убежал
	// 	{
	// 		MoveToTarget(Cast<AMain>(CombatTarget));
	// 	}
	// }
}

FRotator AEnemy::GetLookAtRotationYaw(FVector Target)
{
	FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), Target);
	FRotator LookAtRotationYaw = FRotator(0.f, LookAtRotation.Yaw, 0.f);
	return LookAtRotation;
}


// Called to bind functionality to input
void AEnemy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

 void AEnemy::AgroSphereOnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	if(OtherActor && Alive())
	{
		AMain* Main = Cast<AMain>(OtherActor);
		if(Main)
		{
			MoveToTarget(Main);
		}
	}
}

void AEnemy::AgroSphereOnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if(OtherActor)
	{
		AMain* Main = Cast<AMain>(OtherActor);
		{
			if(Main)
			{
				bHasValidTarget=false;
				if(Main->CombatTarget==this)
				{
					Main->SetCombatTarget(nullptr);
				}
				Main->SetHasCombatTarget(false);
				Main->UpdateCombatTarget();

				SetEnemyMovementStatus(EEnemyMovementStatus::EMS_Idle);
				if(AIController)
				{
					AIController->StopMovement();
				}
			}
		}
	}
}

void AEnemy::CombatSphereOnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	if(OtherActor && Alive())
	{
		AMain* Main = Cast<AMain>(OtherActor);
		{
			if(Main)
			{
				bHasValidTarget=true;
				Main->SetCombatTarget(this);
				Main->SetHasCombatTarget(true);

				Main->UpdateCombatTarget();

				CombatTarget=Main;
				bOverlappingCombatSphere=true;

				float AttackTime = FMath::FRandRange(AttackMinTime, AttackMaxTime);
				GetWorldTimerManager().SetTimer(AttackTimer, this, &AEnemy::Attack, AttackTime);
			}
		}
	}
}

void AEnemy::CombatSphereOnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if(OtherActor && OtherComp)
	{
		AMain* Main = Cast<AMain>(OtherActor);
		{
			if(Main)
			{
				bOverlappingCombatSphere=false;
				MoveToTarget(Main);
				CombatTarget=nullptr;

				if(Main->CombatTarget==this)
				{
					Main->SetCombatTarget(nullptr);
					Main->bHasCombatTarget=false;
					Main->UpdateCombatTarget();
				}
				if(Main->MainPlayerController)
				{
					USkeletalMeshComponent* MainMesh = Cast<USkeletalMeshComponent>(OtherComp);
					if(MainMesh) Main->MainPlayerController->RemoveEnemyHealthBar();
				}

				GetWorldTimerManager().ClearTimer(AttackTimer);
			}
		}
	}
}

void AEnemy::MoveToTarget(class AMain* Target)
{
	SetEnemyMovementStatus(EEnemyMovementStatus::EMS_MoveToTarget);
	if(AIController)
	{
		FAIMoveRequest MoveRequest;
		MoveRequest.SetGoalActor(Target);
		//чт!!
		MoveRequest.SetAcceptanceRadius(80.0f);

		FNavPathSharedPtr NavPath;
		if(!AIController)
		{
			UE_LOG(LogTemp, Warning, TEXT("AIController is NULL"));
		}
		AIController->MoveTo(MoveRequest, &NavPath);

		// auto PathPoints = NavPath->GetPathPoints();
		// for(auto Point :PathPoints)
		// {
		// 	FVector Location = Point.Location;
		// 	UKismetSystemLibrary::DrawDebugSphere(this,Location,25.0f,8,FColor::Green, 10.f, 1.8f);
		// }
	}
}

void AEnemy::CombatOnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if(OtherActor)
	{
		AMain* Main=Cast<AMain>(OtherActor);
		if(Main)
		{
			if(Main->HitParticles)
			{
				const USkeletalMeshSocket* TipSocket=GetMesh()->GetSocketByName("TipSocket");
				if(TipSocket)
				{
					FVector SocketLocation = TipSocket->GetSocketLocation(GetMesh());
					UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), Main->HitParticles, SocketLocation, FRotator::ZeroRotator, false);
				}
			}
			if(Main->HitSound)
			{
				UGameplayStatics::PlaySound2D(this, Main->HitSound);
			}
			if(DamageTypeClass)
			{
				UGameplayStatics::ApplyDamage(Main, Damage, AIController, this, DamageTypeClass);
			}
		}
	}
}

void AEnemy::CombatOnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{

}

void AEnemy::EnableCollision()
{
	CombatCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	if(SwingSound)
	{
		UGameplayStatics::PlaySound2D(this, SwingSound);
	}
}

void AEnemy::DeactivateCollision()
{
	CombatCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AEnemy::Attack()
{
	if(Alive() && bHasValidTarget)
	{
		if(AIController)
		{
			AIController->StopMovement();
			SetEnemyMovementStatus(EEnemyMovementStatus::EMS_Attacking);
		}
		if(!bAttacking)
		{
			UE_LOG(LogTemp, Warning, TEXT("Attack"));
			bAttacking=true;
			SetInterpToMain(true);
			UAnimInstance* AnimInstance=GetMesh()->GetAnimInstance();
			if(AnimInstance)
			{
				AnimInstance->Montage_Play(CombatMontage, 1.0f);
				if(!CombatMontage)
				{
					UE_LOG(LogTemp, Error, TEXT("CombatMontage is NULL"));
				}
				AnimInstance->Montage_JumpToSection(FName("Attack"), CombatMontage);
			}
		}
	}
}


void AEnemy::AttackEnd()
{
	bAttacking=false;
	SetInterpToMain(false);
	if(bOverlappingCombatSphere)
	{
		float AttackTime = FMath::FRandRange(AttackMinTime, AttackMaxTime);
		GetWorldTimerManager().SetTimer(AttackTimer, this, &AEnemy::Attack, AttackTime);
	}
}

void AEnemy::SetInterpToMain(bool Interp)
{
	bInterpToMain=Interp;
}

float AEnemy::TakeDamage(float DamageAmount, struct FDamageEvent const & DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
	if(Health - DamageAmount <= 0.0f)
	{
		Health = 0.f;
		Die(DamageCauser);
	}
	else
	{
		Health-=DamageAmount;
	}
	return DamageAmount;
}

void AEnemy::Die(AActor* Causer)
{
	SetEnemyMovementStatus(EEnemyMovementStatus::EMS_Dead);
	UAnimInstance* AnimInstance=GetMesh()->GetAnimInstance();
	if(AnimInstance)
	{
		AnimInstance->Montage_Play(CombatMontage, 1.0f);
		AnimInstance->Montage_JumpToSection(FName("Death"), CombatMontage);
	}

	CombatCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AgroSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CombatSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	bAttacking=false;

	AMain* Main = Cast<AMain>(Causer);
	if(Main)
	{
		Main->UpdateCombatTarget();
	}
}

void AEnemy::DeathEnd()
{
	GetMesh()->bPauseAnims=true;
	GetMesh()->bNoSkeletonUpdate=true;

	GetWorldTimerManager().SetTimer(DeathTimer, this, &AEnemy::Disappear, DeathDelay);
}

bool AEnemy::Alive()
{
	return GetEnemyMovementStatus()!=EEnemyMovementStatus::EMS_Dead;
}

void AEnemy::Disappear()
{
	Destroy();
}
