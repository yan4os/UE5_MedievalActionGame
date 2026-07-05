// Fill out your copyright notice in the Description page of Project Settings.


#include "Floater.h"

// Sets default values
AFloater::AFloater()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	StaticMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CustomStaticMesh"));

	InitialLocation = FVector(0,0,0);
	PlacedLocation = FVector(0,0,0);
	WorldOrigin = FVector(0,0,0);

	InitialDirection = FVector(0,0,0);

	bInitializeFloaterLocations = false;
	bShouldFloat=false;

	RunningTime=0.0f;

	Amplitude=1.f;
	TimeStretch=1.f;
	C=1.f;
	
	// ADD FORCE
	// InitialForce = FVector(200000,0,0);
	// InitialTorque = FVector(200000,0,0);
}

// Called when the game starts or when spawned
void AFloater::BeginPlay()
{
	Super::BeginPlay();

	float InitialX=FMath::FRandRange(-500.0,500.0);
	float InitialY=FMath::FRandRange(-500.0,500.0);
	float InitialZ=FMath::FRandRange(0.0,500.0);

	InitialLocation.X=InitialX;
	InitialLocation.Y=InitialY;
	InitialLocation.Z=InitialZ;
	
	PlacedLocation=GetActorLocation();

	if(bInitializeFloaterLocations)
	{
		SetActorLocation(InitialLocation);
	}

	//ADD FORCE
	// StaticMesh->AddForce(InitialForce);
	// StaticMesh->AddTorqueInRadians(InitialTorque);

	//LOCAL OFFSET
	// FHitResult HitResult;
	// FVector LocalOffset = FVector(200.f, 0.0f, 0.0f);
	//AddActorWorldOffset(LocalOffset, true, &HitResult);
}

// Called every frame
void AFloater::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if(bShouldFloat)
	{
		FVector NewLocation = GetActorLocation();

		NewLocation.X=NewLocation.X+Amplitude*FMath::Sin(RunningTime*(TimeStretch))+C;
		NewLocation.Y=NewLocation.Y+Amplitude*FMath::Cos(RunningTime*(TimeStretch))+C;

		SetActorLocation(NewLocation);
		RunningTime+=DeltaTime;
	}
	//WORLD ROTATION
	// FRotator Rotation = FRotator(1.0f, 0.0f, 0.0f);
	// AddActorWorldRotation(Rotation);

}

