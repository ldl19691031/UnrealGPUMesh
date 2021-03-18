// Fill out your copyright notice in the Description page of Project Settings.


#include "FluidCollisionActor.h"


// Sets default values
AFluidCollisionActor::AFluidCollisionActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AFluidCollisionActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AFluidCollisionActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Velocity = GetActorLocation() - LastPosition;
	
}

