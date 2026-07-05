// Fill out your copyright notice in the Description page of Project Settings.


#include "FloatinngPlatform.h"

// Sets default values
AFloatinngPlatform::AFloatinngPlatform()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;

	StartPoint = FVector::ZeroVector;
	EndPoint = FVector::ZeroVector;

	bInterping=false;

	InterpTime=1.1f;
	InterpSpeed = 2.0f;
}

// Called when the game starts or when spawned
void AFloatinngPlatform::BeginPlay()
{
	Super::BeginPlay();

	StartPoint = GetActorLocation();
	EndPoint+=StartPoint;

	bInterping=false;

	GetWorldTimerManager().SetTimer(InterpTimer, this, &AFloatinngPlatform::ToggleInterping, InterpTime);
	Distance=(EndPoint-StartPoint).Size();
}

// Called every frame
void AFloatinngPlatform::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if(bInterping)
	{
		FVector CurrentLocation = GetActorLocation();
		FVector Interp = FMath::VInterpTo(CurrentLocation, EndPoint, DeltaTime, InterpSpeed);
		SetActorLocation(Interp);

		float DistanceTraveled = (GetActorLocation()-StartPoint).Size();
		if(Distance-DistanceTraveled<=1.0f)
		{
			ToggleInterping();
			
			GetWorldTimerManager().SetTimer(InterpTimer, this, &AFloatinngPlatform::ToggleInterping, InterpTime);
			SwapVectors(StartPoint, EndPoint);
		}
	}
}

void AFloatinngPlatform::ToggleInterping()
{
	bInterping = !bInterping;
}

void AFloatinngPlatform::SwapVectors(FVector& VecOne, FVector& VecTwo)
{
	FVector Temp=VecOne;
	VecOne=VecTwo;
	VecTwo=Temp;
}
