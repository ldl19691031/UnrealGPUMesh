// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "GPUMeshComponent.h"
#include "GameFramework/Actor.h"
#include "GPUMeshManager.generated.h"

UCLASS(Blueprintable,BlueprintType)
class SHADERDECLARATIONDEMO_API AGPUMeshManager : public AActor
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly,Category="GPU Mesh")
	UGPUMeshComponent* GPUMeshComponent;	
public:
	AGPUMeshManager(const FObjectInitializer& ObjectInitializer);
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void RegisterBrush(class UGPUMeshBrush* brush)
	{
		if (GPUMeshComponent)
		{
			GPUMeshComponent->MeshBrushes.Add(brush);
		}
	}

	virtual void UnregisterBrush(class UGPUMeshBrush* brush)
	{
		if (GPUMeshComponent)
		{
			GPUMeshComponent->MeshBrushes.Remove(brush);
		}	
	}

	
};
