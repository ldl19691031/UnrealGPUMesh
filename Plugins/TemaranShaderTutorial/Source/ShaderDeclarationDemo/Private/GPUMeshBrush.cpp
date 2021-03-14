// Fill out your copyright notice in the Description page of Project Settings.


#include "GPUMeshBrush.h"


#include "EngineUtils.h"
#include "GPUMeshManager.h"


// Called when the game starts
void UGPUMeshBrush::BeginPlay()
{
	Super::BeginPlay();

	// ...
	if (GetWorld() == nullptr)
	{
		return;
	}
	for (TActorIterator<AGPUMeshManager> It(GetWorld()); It; ++It)
	{
		It->RegisterBrush(this);
	}
}


// Called every frame
void UGPUMeshBrush::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

FGPUMeshParameters::FBrushParameter UGPUMeshBrush::GetGPUMeshParameter()
{
	return FGPUMeshParameters::FBrushParameter{
		this->GetOwner()->GetActorLocation(),
		Radius
	};
}

void UGPUMeshBrush::DestroyComponent(bool bPromoteChildren)
{
	if (GetWorld() == nullptr)
	{
		return;
	}
	for (TActorIterator<AGPUMeshManager> It(GetWorld()); It; ++It)
	{
		It->UnregisterBrush(this);
	}
	Super::DestroyComponent(bPromoteChildren);
}

void UGPUMeshBrush::InitializeComponent()
{
	Super::InitializeComponent();
}

