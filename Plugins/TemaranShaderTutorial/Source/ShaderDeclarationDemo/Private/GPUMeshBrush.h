// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CoreUObject.h"
#include "ShaderDeclarationDemoModule.h"
#include "Components/ActorComponent.h"



#include "GPUMeshBrush.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SHADERDECLARATIONDEMO_API UGPUMeshBrush : public UActorComponent
{
public:
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, Category="GPU Mesh")
	float Radius;
protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	virtual FGPUMeshParameters::FBrushParameter GetGPUMeshParameter();

	virtual void DestroyComponent(bool bPromoteChildren) override;

	virtual void InitializeComponent() override;
};
