// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Components/ActorComponent.h"
#include "Components/MeshComponent.h"
#include "GPUDynamicVertexBuffer.h"
#include "GPUMeshComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent),Blueprintable)
class SHADERDECLARATIONDEMO_API UGPUMeshComponent : public UMeshComponent
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category="GPU Mesh")
	UVolumeTexture* SDFTexture;

	UPROPERTY(EditAnywhere, Category="GPU Mesh")
	UMaterialInterface* RenderMaterial;

	UPROPERTY(EditAnywhere, Category="GPU Mesh")
	int NumVertics = 10240;

	UPROPERTY(EditAnywhere, Category="GPU Mesh")
	FVector VertexOffset;

	UPROPERTY(EditAnywhere, Category="GPU Mesh")
	FVector VertexScale;

	UPROPERTY(EditAnywhere, Category="GPU Mesh")
	float Smoothness = 1.0f;

	UPROPERTY(VisibleAnywhere, Category="GPU Mesh")
	TArray<class UGPUMeshBrush*> MeshBrushes;
public:
	// Sets default values for this component's properties
	UGPUMeshComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;


	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;

	virtual int32 GetNumMaterials() const override { return 1; }

	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
};
struct FGPUMeshVertexBuffers
{
	/** The buffer containing vertex data. */
	FStaticMeshVertexBuffer StaticMeshVertexBuffer;

	
	/** The buffer containing the position vertex data. */
	FGPUDynamicVertexBuffer PositionVertexBuffer;

	FGPUDynamicVertexBuffer DynamicTangentXBuffer;
	
	FGPUDynamicVertexBuffer DynamicTangentZBuffer;
	
	/** The buffer containing the vertex color data. */
	FColorVertexBuffer ColorVertexBuffer;

	void Init(FLocalVertexFactory* VertexFactory, uint32 NumVertics);

	static inline void InitOrUpdateResource(FRenderResource* Resource)
	{
		if (!Resource->IsInitialized())
		{
			Resource->InitResource();
		}
		else
		{
			Resource->UpdateRHI();
		}
	}
};