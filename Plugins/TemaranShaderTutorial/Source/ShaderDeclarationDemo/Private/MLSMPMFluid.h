// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "GPUMeshComponent.h"
#include "Components/ActorComponent.h"
#include "MLSMPMFluid.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SHADERDECLARATIONDEMO_API UMLSMPMFluid : public UActorComponent
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fluid")
	uint8 ParticlePerSecond = 1;
public:
	// Sets default values for this component's properties
	UMLSMPMFluid();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	virtual void DestroyComponent(bool bPromoteChildren) override;
private:
	TSharedPtr<struct FMLSMPMData> FluidData;

	float DeltaTimeCounter = 0.0f;

	UGPUMeshComponent* Visualizer;
};
