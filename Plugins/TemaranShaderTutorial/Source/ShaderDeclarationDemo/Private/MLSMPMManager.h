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
	uint32 MaxParticles = 40960;
	const uint32 N_Grid = 32;
	const float dx = 1.0f / N_Grid;
	const float inv_dx = 1.0f / dx;
	const float dt = 2e-4f;
	const float E = 400;
	const uint32 bound = 5;

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

	UGPUMeshComponent* visualizer;
	
	uint32 particle_num = 0;
	FMLSMPMData()
	{
			
	}
	void Init();

	
	
	void Init_RenderThread(FRHICommandList& RHICmdList);

	void Release();

	void Substep(FRHICommandList& RHICmdList);

	void AddParticle(uint32 num);
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
	static TArray<TSharedPtr<FMLSMPMData>> MLSMPMDatas;

	static void RegisterData(TSharedPtr<FMLSMPMData> data);

	static void UnregisterData(TSharedPtr<FMLSMPMData> data);

	static void Update_RenderThread(FRHICommandList& RHICmdList);
};
