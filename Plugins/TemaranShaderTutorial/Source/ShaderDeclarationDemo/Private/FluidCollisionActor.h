// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"


#include "FluidCollisionActor.generated.h"

UCLASS()
class SHADERDECLARATIONDEMO_API AFluidCollisionActor : public AActor
{
	GENERATED_BODY()
	UPROPERTY(VisibleAnywhere)
	FVector Velocity;
public:
	// Sets default values for this actor's properties
	AFluidCollisionActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual  FVector GetVelocity() const override {return Velocity;}

private:
	FVector LastPosition;
};
