// Fill out your copyright notice in the Description page of Project Settings.


#include "Main.h"

#include "Enemy.h"
#include "GameFramework/InputSettings.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetStringLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Weapon.h"
#include "Sound/SoundCue.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Enemy.h"
#include "MainPlayerController.h"
#include "SaveMyGame.h"
#include "ItemStorage.h"

// Sets default values
AMain::AMain()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	//Create Camera Boom (pull towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->TargetArmLength = 600.0f;	//Camera follows at this distance
	CameraBoom->bUsePawnControlRotation=true;	//Rotate arm based on controller

	//Set size for collision capsule
	GetCapsuleComponent()->SetCapsuleSize(43.f, 103.f);

	// Create Follow Camera
	FollowCamera=CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	//Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false;

	//Set our turn rates for input
	BaseTurnRate=65.f;
	BaseLookUpRate=65.f;

	 //Don't rotate when the controller rotates
	 //Let that just affect the camera
	 bUseControllerRotationYaw=false;
	 bUseControllerRotationPitch=false;
	 bUseControllerRotationRoll=false;

	 //Configure character movement
	 GetCharacterMovement()->bOrientRotationToMovement = true; //Character moves in the direction of input
	 GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.f, 0.0f); //...at this rotation rete
	 GetCharacterMovement()->JumpZVelocity=650.f;
	 GetCharacterMovement()->AirControl=0.2;

	//camera
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

 	MaxHealth=100.f;
	Health=65.f;
	MaxStamina=150.f;
	Stamina=120.f;
	Coins=0;

	RunningSpeed=650.f;
	SprintingSpeed=950.f;
	bMovingForward=false;
	bMovingRight=false;

	bShiftKyeDown=false;
	bLMBDown=false;

	//Initialize Enums
	MovementStatus=EMovementStatus::EMS_Normal;
	StaminaStatus=EStaminaStatus::ESS_Normal;

	StaminaDrainRate=25.f;
	MinSprintStamina=50.f;

	InterpSpeed=15.f;
	bInterpToEnemy=false;

	bHasCombatTarget=false;
}

// Called when the game starts or when spawned
void AMain::BeginPlay()
{
	Super::BeginPlay();

	MainPlayerController = Cast<AMainPlayerController>(GetController());
}

// Called every frame
void AMain::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if(MovementStatus == EMovementStatus::EMS_Dead) return;
	
	float MoveForwardValue = GetInputAxisValue("MoveForward");

	if (bShiftKyeDown && MoveForwardValue <= 0.1f)
	{
		SetMovementStatus(EMovementStatus::EMS_Normal);

		float DeltaStamina = StaminaDrainRate * DeltaTime;
		if (Stamina + DeltaStamina >= MaxStamina)
		{
			Stamina = MaxStamina;
		}
		else
		{
			Stamina += DeltaStamina;
		}
		return;
	}
	
	float DeltaStamina=StaminaDrainRate * DeltaTime;
	switch (StaminaStatus)
	{
		case EStaminaStatus::ESS_Normal:
			if(bShiftKyeDown && MoveForwardValue > 0.1f)
			{
				if(Stamina-DeltaStamina<=MinSprintStamina)
				{
					SetStaminaStatus(EStaminaStatus::ESS_BelowMinimum);
					Stamina-=DeltaStamina;
				}
				else
				{
					Stamina-=DeltaStamina;
				}
				if(bMovingForward || bMovingRight)
				{
					SetMovementStatus(EMovementStatus::EMS_Sprinting);
				}
				else
				{
					SetMovementStatus(EMovementStatus::EMS_Normal);
				}

			}
			else //Shift key is up
			{
				if(Stamina+DeltaStamina>=MaxStamina)
				{
					Stamina=MaxStamina;
				}
				else
				{
					Stamina+=DeltaStamina;
				}
				SetMovementStatus(EMovementStatus::EMS_Normal);
			}
			break;
		case EStaminaStatus::ESS_BelowMinimum:
			if(bShiftKyeDown && MoveForwardValue > 0.1f)
			{
				if(Stamina-DeltaStamina<=0.0f)
				{
					SetStaminaStatus(EStaminaStatus::ESS_Exausted);
					Stamina=0;
					SetMovementStatus(EMovementStatus::EMS_Normal);
				}
				else
				{
					Stamina-=DeltaStamina;
					if(bMovingForward || bMovingRight)
					{
						SetMovementStatus(EMovementStatus::EMS_Sprinting);
					}
					else
					{
						SetMovementStatus(EMovementStatus::EMS_Normal);
					}
				}
			}
			else //Shift key is up
			{
				if(Stamina+DeltaStamina>=MinSprintStamina)
				{
					SetStaminaStatus(EStaminaStatus::ESS_Normal);
					Stamina+=DeltaStamina;
				}
				else
				{
					Stamina+=DeltaStamina;
				}
				SetMovementStatus(EMovementStatus::EMS_Normal);
			}
			break;
		case EStaminaStatus::ESS_Exausted:
			if(bShiftKyeDown && MoveForwardValue > 0.1f)
			{
				Stamina=0.f;
			}
			else //Shift key is up
			{
				SetStaminaStatus(EStaminaStatus::ESS_ExaustedRecovering);
				Stamina+=DeltaStamina;
			}
			SetMovementStatus(EMovementStatus::EMS_Normal);
			break;
		case EStaminaStatus::ESS_ExaustedRecovering:
			if(Stamina+DeltaStamina>=MinSprintStamina)
			{
				SetStaminaStatus(EStaminaStatus::ESS_Normal);
				Stamina+=DeltaStamina;
			}else
			{
				Stamina+=DeltaStamina;
			}
			SetMovementStatus(EMovementStatus::EMS_Normal);
			break;
		default:
			break;
	}

	if(bInterpToEnemy && CombatTarget)
	{
		FRotator LookAtYaw = GetLookAtRotationYaw(CombatTarget->GetActorLocation());
		FRotator InterpRotation = FMath::RInterpTo(GetActorRotation(), LookAtYaw, DeltaTime, InterpSpeed);

		SetActorRotation(InterpRotation);
	}

	if(CombatTarget)
	{
		CombatTargetLocation=CombatTarget->GetActorLocation();
		if(MainPlayerController)
		{
			MainPlayerController->EnemyLocation = CombatTargetLocation;
		}
	}
}

FRotator AMain::GetLookAtRotationYaw(FVector Target)
{
	FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), Target);
	FRotator LookAtRotationYaw = FRotator(0.f, LookAtRotation.Yaw, 0.f);
	return LookAtRotation;
}

// Called to bind functionality to input
void AMain::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	check(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AMain::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &AMain::ShiftKeyDown);
	PlayerInputComponent->BindAction("Sprint", IE_Released, this, &AMain::ShiftKeyUp);

	PlayerInputComponent->BindAction("LMB", IE_Pressed, this, &AMain::LMBDown);
	PlayerInputComponent->BindAction("LMB", IE_Released, this, &AMain::LMBUp);

	PlayerInputComponent->BindAxis("MoveForward", this, &AMain::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AMain::MoveRight);

	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);

	PlayerInputComponent->BindAxis("TurnRate", this, &AMain::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AMain::LookUpAtRate);
}

void AMain::MoveForward(float Value)
{
	bMovingForward=false;
	if((Controller!=nullptr) && (Value!=0.0f)&&(!bAttacking) && !(MovementStatus==EMovementStatus::EMS_Dead))
	{
		//find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0.f, Rotation.Yaw, 0.0f);

		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);

		bMovingForward=true;
	}
}

void AMain::MoveRight(float Value)
{
	bMovingRight=false;
	if((Controller!=nullptr) && (Value!=0.0f)&&(!bAttacking) && !(MovementStatus==EMovementStatus::EMS_Dead))
	{
		//find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0.f, Rotation.Yaw, 0.0f);

		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		AddMovementInput(Direction, Value);
		bMovingRight=true;
	}
}

void AMain::TurnAtRate(float Rate)
{
	AddControllerYawInput(Rate*BaseLookUpRate*GetWorld()->GetDeltaSeconds());
}

void AMain::LookUpAtRate(float Rate)
{
	AddControllerPitchInput(Rate*BaseLookUpRate*GetWorld()->GetDeltaSeconds());
}

void AMain::DecrementHealth(float Amount)
{
	if(Health-Amount<=0.f)
	{
		Health-=Amount;
		Die();
	}
	else
	{
		Health-=Amount;
	}
}

void AMain::IncrementCoins(int32 Amount)
{
	Coins+=Amount;
}

void AMain::IncrementHealth(float Amount)
{
	if(Health+Amount>=MaxHealth)
	{
		Health=MaxHealth;
	}
	else
	{
		Health+=Amount;
	}
}

void AMain::Die()
{
	UAnimInstance* AnimInstance=GetMesh()->GetAnimInstance();
	if(AnimInstance && CombatMontage)
	{
		AnimInstance->Montage_Play(CombatMontage, 1.0f);
		AnimInstance->Montage_JumpToSection(FName("Death"));
	}
	SetMovementStatus(EMovementStatus::EMS_Dead);
}

void AMain::Jump()
{
	if(MovementStatus != EMovementStatus::EMS_Dead)
	{
		Super::Jump();
	}
}


void AMain::DeathEnd()
{
	GetMesh()->bPauseAnims=true;
	GetMesh()->bNoSkeletonUpdate=true;
}


void AMain::SetMovementStatus(EMovementStatus Status)
{
	MovementStatus = Status;
	if(MovementStatus==EMovementStatus::EMS_Sprinting)
	{
		GetCharacterMovement()->MaxWalkSpeed = SprintingSpeed;
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = RunningSpeed;
	}
}

void AMain::ShiftKeyDown()
{
	bShiftKyeDown=true;
}

void AMain::ShiftKeyUp()
{
	bShiftKyeDown=false;
}

void AMain::ShowPickupLocations()
{
	for(int32 i=0;i<PickupLocations.Num();i++)
	{
		UKismetSystemLibrary::DrawDebugSphere(this, GetActorLocation()+ FVector(0,0,75.f), 25.f, 12, FLinearColor::Yellow, 5.f, 2.f);
	}
}

void AMain::LMBDown()
{
	bLMBDown = true;

	if(MovementStatus ==EMovementStatus::EMS_Dead) return;

	if (ActiveOverlappingItem)
	{
		AWeapon* Weapon = Cast<AWeapon>(ActiveOverlappingItem);
		if (Weapon && Weapon != EquippedWeapon)
		{
			Weapon->Equip(this);
			SetActiveOverlappingItem(nullptr);
			return;
		}
	}
	if (EquippedWeapon)
	{
		Attack();
	}
}

void AMain::LMBUp()
{
	bLMBDown=false;
}

void AMain::SetEquippedWeapon(AWeapon* WeaponToSet)
{
	if(EquippedWeapon && EquippedWeapon != WeaponToSet)
	{
		EquippedWeapon->Destroy();
	}
	EquippedWeapon=WeaponToSet;
	SetMovementStatus(MovementStatus);
}

void AMain::Attack()
{
	if(!bAttacking && !(MovementStatus==EMovementStatus::EMS_Dead))
	{
		bAttacking=true;
		SetInterpToEnemy(true);

		UAnimInstance* AnimInstance=GetMesh()->GetAnimInstance();
		if(AnimInstance && CombatMontage)
		{
			int32 Section = FMath::RandRange(0,1);
			switch(Section)
			{
				case 0:
					AnimInstance->Montage_Play(CombatMontage, 1.8f);
					AnimInstance->Montage_JumpToSection(FName("Attack1"), CombatMontage);
					break;
				case 1:
					AnimInstance->Montage_Play(CombatMontage, 1.3f);
					AnimInstance->Montage_JumpToSection(FName("Attack2"), CombatMontage);
					break;
				
				default:
					break;
			}
		}
	}
}

void AMain::AttackEnd()
{
	bAttacking=false;
	SetInterpToEnemy(false);
	if(bLMBDown)
	{
		Attack();
	}
}

void AMain::PlaySwingSound()
{
	if(EquippedWeapon->SwingSound)
	{
		UGameplayStatics::PlaySound2D(this, EquippedWeapon->SwingSound);
	}
}

void AMain::SetInterpToEnemy(bool Interp)
{
	bInterpToEnemy=Interp;
}

float AMain::TakeDamage(float DamageAmount, struct FDamageEvent const & DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
	if(Health-DamageAmount<=0.f)
	{
		Health-=DamageAmount;
		Die();
		if(DamageCauser)
		{
			AEnemy* Enemy = Cast<AEnemy>(DamageCauser);
			if(Enemy)
			{
				Enemy->bHasValidTarget=false;
			}
		}
	}
	else
	{
		Health-=DamageAmount;
	}
	return DamageAmount;
}

void AMain::UpdateCombatTarget()
{
	TArray<AActor*> OverlappingActors;
	GetOverlappingActors(OverlappingActors, EnemyFilter);

	if(OverlappingActors.Num()==0)
	{
		if(MainPlayerController)
		{
			MainPlayerController->RemoveEnemyHealthBar();
		}
		return;
	}

	AEnemy* ClosestEnemy = Cast<AEnemy>(OverlappingActors[0]);
	if(ClosestEnemy)
	{
		FVector Location = GetActorLocation();
		float MinDistance = (ClosestEnemy->GetActorLocation()-Location).Size();

		for(auto Actor : OverlappingActors)
		{
			AEnemy* Enemy = Cast<AEnemy>(Actor);
			if(Enemy)
			{
				float DistanceToActor = (Enemy->GetActorLocation()-Location).Size();
				if(DistanceToActor < MinDistance)
				{
					MinDistance = DistanceToActor;
					ClosestEnemy = Enemy;
				}
			}
		}
		if(MainPlayerController)
		{
			MainPlayerController->DisplayEnemyHealthBar();
		}
		SetCombatTarget(ClosestEnemy);
		bHasCombatTarget=true;
	}
}

void AMain::SwitchLevel(FName LevelName)
{
	UWorld* World = GetWorld();
	if(World)
	{
		FString CurrentLevel = World->GetMapName();

		FName CurrentLevelName(*CurrentLevel);
		if(CurrentLevel!=LevelName)
		{
			UGameplayStatics::OpenLevel(World, LevelName);
		}
	}
}

void AMain::SaveGame()
{
	USaveMyGame* SaveGameInstance = Cast<USaveMyGame>(UGameplayStatics::CreateSaveGameObject(USaveMyGame::StaticClass()));

	SaveGameInstance->CharacterStats.Health=Health;
	SaveGameInstance->CharacterStats.Coins=Coins;
	SaveGameInstance->CharacterStats.MaxHealth=MaxHealth;
	SaveGameInstance->CharacterStats.MaxStamina=MaxStamina;
	SaveGameInstance->CharacterStats.Stamina=Stamina;

	if(EquippedWeapon)
	{
		SaveGameInstance->CharacterStats.WeaponName=EquippedWeapon->Name;
	}

	SaveGameInstance->CharacterStats.Location=GetActorLocation();
	SaveGameInstance->CharacterStats.Rotation=GetActorRotation();

	UGameplayStatics::SaveGameToSlot(SaveGameInstance, SaveGameInstance->PlayerName, SaveGameInstance->UserIndex);
}

void AMain::LoadGame(bool SetPosition)
{
	USaveMyGame* LoadGameInstance = Cast<USaveMyGame>(UGameplayStatics::CreateSaveGameObject(USaveMyGame::StaticClass()));

	LoadGameInstance = Cast<USaveMyGame> (UGameplayStatics::LoadGameFromSlot(LoadGameInstance->PlayerName, LoadGameInstance->UserIndex));

	Health = LoadGameInstance->CharacterStats.Health;
	Stamina = LoadGameInstance->CharacterStats.Stamina;
	MaxHealth = LoadGameInstance->CharacterStats.MaxHealth;
	MaxStamina = LoadGameInstance->CharacterStats.MaxStamina;
	Coins = LoadGameInstance->CharacterStats.Coins;

	if(WeaponStorage)
	{
		AItemStorage* Weapons = GetWorld()->SpawnActor<AItemStorage>(WeaponStorage);
		if(Weapons)
		{
			FString WeaponName = LoadGameInstance->CharacterStats.WeaponName;
			if(TSubclassOf<AWeapon>* WeaponClass = Weapons->WeaponMap.Find(WeaponName))
			{
				AWeapon* WeaponToEquip = GetWorld()->SpawnActor<AWeapon>(*WeaponClass);
				WeaponToEquip->Equip(this);
			}
		}
	}


	if(SetPosition)
	{
		SetActorLocation(LoadGameInstance->CharacterStats.Location);
		SetActorRotation(LoadGameInstance->CharacterStats.Rotation);
	}
}
