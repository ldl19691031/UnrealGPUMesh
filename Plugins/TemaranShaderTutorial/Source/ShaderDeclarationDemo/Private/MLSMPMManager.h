// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "GPUMeshComponent.h"

struct FMLSMPMData : TSharedFromThis<FMLSMPMData>
{
	struct FParticleData
	{
		FVector x;
		FVector v;
		float J;
		float pad;
		FMatrix C;
	};
	struct FCollisionData
	{
		FVector pos;
		float radius;

		FVector4 AsFloat4() const
		{
			return FVector4(pos, radius);
		}
	};
	uint32 MaxParticles = 1048576;
	const uint32 N_Grid = 64;
	const float dx = 1.0f / N_Grid;
	const float inv_dx = 1.0f / dx;
	const float dt = 4e-4f;

	const float E = 500;
	const uint32 bound = 5;
	float gameThreadTickTime;
	float p_rho = 1.0f;
	const float p_vol = (dx * 0.5f) *(dx * 0.5f) *(dx * 0.5f) ;
	const float p_mass = p_vol * p_rho;
	const float gravity = 9.8f;
	
	FStructuredBufferRHIRef n_particles_buffer;
	FUnorderedAccessViewRHIRef n_particles_buffer_uav;

	FStructuredBufferRHIRef particles_buffer;
	FUnorderedAccessViewRHIRef particles_buffer_uav;

	FTexture3DRHIRef grid_texture;
	FUnorderedAccessViewRHIRef grid_texture_uav;
	
	FTexture3DRHIRef grid_textureX;
	FUnorderedAccessViewRHIRef grid_textureX_uav;
	FTexture3DRHIRef grid_textureY;
	FUnorderedAccessViewRHIRef grid_textureY_uav;
	FTexture3DRHIRef grid_textureZ;
	FUnorderedAccessViewRHIRef grid_textureZ_uav;
	FTexture3DRHIRef grid_textureW;
	FUnorderedAccessViewRHIRef grid_textureW_uav;
	
	FStructuredBufferRHIRef grid_texture_atomic;
	FUnorderedAccessViewRHIRef grid_texture_atomic_uav;

	FStructuredBufferRHIRef collision_buffer;
	//FUnorderedAccessViewRHIRef collision_buffer_uav;
	FShaderResourceViewRHIRef collision_buffer_srv;
	TArray<FCollisionData> collision_data_cpu;
	const int max_collision_count = 64;
	
	UGPUMeshComponent* visualizer;

	FTexture3DRHIRef sdf_temp_texture;
	FUnorderedAccessViewRHIRef sdf_temp_texture_uav;


	
	uint32 particle_num = 0;

	uint32 collision_data_num = 0;

	
	const UINT32 GROUP_SIZE = 1024;
	FMLSMPMData()
	{
			
	}
	void Init();

	
	
	void Init_RenderThread(FRHICommandList& RHICmdList);

	void Release();

	void Substep(FRHICommandList& RHICmdList);

	void Visualize(FRHICommandList& RHICmdList, FPostOpaqueRenderParameters& Parameters);
	void AddParticle(uint32 num);

	void SetCollision(const TArray<FCollisionData>& CollisionDatas);
private:
	void InitializeTexture(FTexture3DRHIRef& RefCount, FUnorderedAccessViewRHIRef& RefCountPtr);
private:
	bool NeedUpdateParticleNum = false;
};



/**
 * 
 */
class SHADERDECLARATIONDEMO_API FMLSMPMManager
{
public:
	static TArray<FMLSMPMData*> MLSMPMDatas;

	static void RegisterData(FMLSMPMData* data);

	static void UnregisterData(FMLSMPMData* data);

	static void Update_RenderThread(FRHICommandList& RHICmdList);

	static void Visualize_RenderThread(FRHICommandList& RHICmdList, FPostOpaqueRenderParameters& Parameters);
};
