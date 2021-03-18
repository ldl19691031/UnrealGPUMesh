// Fill out your copyright notice in the Description page of Project Settings.


#include "MLSMPMFluid.h"


#include "EngineUtils.h"
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
	
	for(TActorIterator<AFluidCollisionActor> It(GetWorld()); It; ++It)
	{
		CollisionActors.Add(*It);
	}
	
}


// Called every frame
void UMLSMPMFluid::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	DeltaTimeCounter += DeltaTime;
	FluidData->AddParticle((uint32)(ParticlePerSecond * DeltaTime));
	// if (DeltaTimeCounter >= 1.0f)
	// {
	// 	FluidData->AddParticle(ParticlePerSecond);
	// 	DeltaTimeCounter = 0.0f;
	// }
	TArray<FMLSMPMData::FCollisionData> CollisionDatas;
	for(auto CollisionActor : this->CollisionActors)
	{
		if (CollisionActor == nullptr || IsValid(CollisionActor) == false)
		{
			continue;;
		}
		FVector origin, extend;
		CollisionActor->GetActorBounds(true, origin, extend);
		if (extend.Size() <= 0.01f)
		{
			continue;
		}
		float scale = 1280.0f;
		CollisionDatas.Add(
            FMLSMPMData::FCollisionData{
                origin / scale,
                extend.Size() / scale
            }
        );
	}
	if (CollisionDatas.Num() > 0)
	{
		FluidData->SetCollision(CollisionDatas);
	}
	FluidData->gameThreadTickTime = DeltaTime;
	FluidData->visualizer = this->Visualizer;
	

	
	FShaderDeclarationDemoModule::Get().RequestTickMPMFluid();
}

void UMLSMPMFluid::DestroyComponent(bool bPromoteChildren)
{
	Super::DestroyComponent(bPromoteChildren);
	FluidData->Release();
}

