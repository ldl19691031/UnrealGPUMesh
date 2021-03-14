// Fill out your copyright notice in the Description page of Project Settings.


#include "GPUMeshManager.h"


AGPUMeshManager::AGPUMeshManager(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	//RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	GPUMeshComponent = CreateDefaultSubobject<UGPUMeshComponent>(TEXT("GPU Mesh"));
	RootComponent = GPUMeshComponent;
	//GPUMeshComponent->AttachToComponent(this->RootComponent,FAttachmentTransformRules::KeepRelativeTransform);
	this->AddOwnedComponent(GPUMeshComponent);
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AGPUMeshManager::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AGPUMeshManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

