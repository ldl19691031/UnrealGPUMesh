// Fill out your copyright notice in the Description page of Project Settings.


#include "MLSMPMFluid.h"

#include "GPUMeshComponent.h"
#include "MLSMPMManager.h"
#include "ShaderDeclarationDemoModule.h"

// Sets default values for this component's properties
UMLSMPMFluid::UMLSMPMFluid()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...

	FluidData = MakeShared<FMLSMPMData>();
}


// Called when the game starts
void UMLSMPMFluid::BeginPlay()
{
	Super::BeginPlay();

	// ...
	FluidData->Init();

	UActorComponent* c =GetOwner()->GetComponentByClass(UGPUMeshComponent::StaticClass());
	if (c != nullptr)
	{
		Visualizer =Cast<UGPUMeshComponent>(c);
	}
}


// Called every frame
void UMLSMPMFluid::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	DeltaTimeCounter += DeltaTime;
	if (DeltaTimeCounter >= 1.0f)
	{
		FluidData->AddParticle(ParticlePerSecond);
		DeltaTimeCounter = 0.0f;
	}
	FluidData->visualizer = this->Visualizer;
	FShaderDeclarationDemoModule::Get().RequestTickMPMFluid();
}

void UMLSMPMFluid::DestroyComponent(bool bPromoteChildren)
{
	Super::DestroyComponent(bPromoteChildren);
	FluidData->Release();
}

