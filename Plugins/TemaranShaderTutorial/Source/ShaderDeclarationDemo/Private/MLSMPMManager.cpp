// Fill out your copyright notice in the Description page of Project Settings.


#include "MLSMPMManager.h"
#include "Shader.h"
#include "GlobalShader.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "ShaderParameterStruct.h"


#define MLSMPMBUFFER_DEF BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )\
        SHADER_PARAMETER(UINT, max_particles)\
        SHADER_PARAMETER(UINT, n_grid)\
        SHADER_PARAMETER(float, dx)\
        SHADER_PARAMETER(float, inv_dx)\
        SHADER_PARAMETER(float, dt)\
        SHADER_PARAMETER(float, E)\
        SHADER_PARAMETER(UINT, bound)\
        SHADER_PARAMETER(float, p_rho)\
        SHADER_PARAMETER(float, p_vol)\
        SHADER_PARAMETER(float, p_mass)\
        SHADER_PARAMETER(float, gravity)\
        SHADER_PARAMETER(UINT, n_particles)\
        SHADER_PARAMETER_UAV(RWStructuredBuffer<FParticleData>,particleDataBuffer )\
        SHADER_PARAMETER_UAV(RWTexture3D<float4>, grid )\
        SHADER_PARAMETER_UAV(RWTexture3D<uint>,grid_X)\
        SHADER_PARAMETER_UAV(RWTexture3D<uint>,grid_Y)\
        SHADER_PARAMETER_UAV(RWTexture3D<uint>,grid_Z)\
        SHADER_PARAMETER_UAV(RWTexture3D<uint>,grid_W)\
        SHADER_PARAMETER_UAV(RWStructuredBuffer<uint>, grid_atomic )\
  END_SHADER_PARAMETER_STRUCT()


template<typename T>
void GetParametersFromMLSMPMData(const FMLSMPMData* data, T& param)
{
	param.max_particles = data->MaxParticles;
	param.n_grid = data->N_Grid;
	param.dx = data->dx;
	param.inv_dx = data->inv_dx;
	param.dt = data->dt;
	param.E = data->E;
	param.bound = data->bound;
	param.p_rho = data->p_rho;
	param.p_vol = data->p_vol;
	param.p_mass = data->p_mass;
	param.gravity = data->gravity;
	param.n_particles = data->particle_num;
	param.grid_X = data->grid_textureX_uav;
	param.grid_Y = data->grid_textureY_uav;
	param.grid_Z = data->grid_textureZ_uav;
	param.grid_W = data->grid_textureW_uav;
	param.particleDataBuffer = data->particles_buffer_uav;
	param.grid = data->grid_texture_uav;
	param.grid_atomic = data->grid_texture_atomic_uav;
}

class FMLSMPMShaderP2G : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FMLSMPMShaderP2G);
	SHADER_USE_PARAMETER_STRUCT(FMLSMPMShaderP2G, FGlobalShader);

	MLSMPMBUFFER_DEF
public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};
IMPLEMENT_GLOBAL_SHADER(FMLSMPMShaderP2G, "/TutorialShaders/Private/MLSMPMCS.usf", "P2G", SF_Compute);

class FMLSMPMShaderGridMain : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FMLSMPMShaderGridMain);
	SHADER_USE_PARAMETER_STRUCT(FMLSMPMShaderGridMain, FGlobalShader);

	MLSMPMBUFFER_DEF
public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};
IMPLEMENT_GLOBAL_SHADER(FMLSMPMShaderGridMain, "/TutorialShaders/Private/MLSMPMCS.usf", "GridMain", SF_Compute);

class FMLSMPMShaderG2P : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FMLSMPMShaderG2P);
	SHADER_USE_PARAMETER_STRUCT(FMLSMPMShaderG2P, FGlobalShader);

	MLSMPMBUFFER_DEF
public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};
IMPLEMENT_GLOBAL_SHADER(FMLSMPMShaderG2P, "/TutorialShaders/Private/MLSMPMCS.usf", "G2P", SF_Compute);

class FMPMToSDFTextureShader : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FMPMToSDFTextureShader);
		SHADER_USE_PARAMETER_STRUCT(FMPMToSDFTextureShader, FGlobalShader);
		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(UINT, n_particles)
		SHADER_PARAMETER(UINT, texture_size)
		SHADER_PARAMETER_UAV(RWStructuredBuffer<FParticleData>, particleDataBuffer)
		SHADER_PARAMETER_UAV(RWTexture3D<float4>,OutputTexture)
	END_SHADER_PARAMETER_STRUCT()
public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};
IMPLEMENT_GLOBAL_SHADER(FMPMToSDFTextureShader, "/TutorialShaders/Private/MPMParticleToSDFTexture.usf", "Main", SF_Compute);
void FMLSMPMData::Init()
{
	FMLSMPMData* Self = this;
	ENQUEUE_RENDER_COMMAND(InitMPMData)
	(
		[Self](FRHICommandList& RHICmdList)
		{
			Self->Init_RenderThread(RHICmdList);
		}
	);
	FMLSMPMManager::RegisterData(SharedThis(this));
	particle_num = 10;
}

void FMLSMPMData::Init_RenderThread(FRHICommandList& RHICmdList)
{
	check(IsInRenderingThread());
	// if (n_particles_buffer.IsValid() == false)
	// {
	// 	FRHIResourceCreateInfo rhiCreateInfo;
	// 	n_particles_buffer = RHICreateStructuredBuffer(
	// 		sizeof(int), sizeof(int), BUF_ShaderResource | BUF_UnorderedAccess | BUF_Dynamic, rhiCreateInfo
	// 	);
	// 	n_particles_buffer_uav = RHICreateUnorderedAccessView(n_particles_buffer, false, false);
	// 	uint8* Buffer = (uint8*)RHILockStructuredBuffer(n_particles_buffer, 0, sizeof(int), RLM_WriteOnly);
	// 	(*(int*)(Buffer)) = 0;
	// 	RHIUnlockStructuredBuffer(n_particles_buffer);
	// }
	if (particles_buffer.IsValid() == false)
	{
		FRHIResourceCreateInfo rhiCreateInfo;
		particles_buffer = RHICreateStructuredBuffer(
			sizeof(FParticleData), sizeof(FParticleData) * this->MaxParticles, BUF_ShaderResource | BUF_UnorderedAccess, rhiCreateInfo);
		particles_buffer_uav = RHICreateUnorderedAccessView(particles_buffer, false, false);
		TArray<FParticleData> InitialData;
		const FParticleData initialData = FParticleData{
			FVector(0.75f, 0.5f, 0.75f),
			FVector(0.0f, 0.0f, 0.0f),
			1.0f,
			0.0f,
			FMatrix(EForceInit::ForceInitToZero)
		};
		for (UINT i = 0; i < MaxParticles; i ++)
		{
			FParticleData instance = initialData;
			instance.x += FMath::VRand() * 0.05f;
			
			InitialData.Add(instance);
		}
		void* Buffer = RHILockStructuredBuffer(particles_buffer, 0, sizeof(int), RLM_WriteOnly);
		memcpy(Buffer, InitialData.GetData(), InitialData.Num() * sizeof(FParticleData));
		RHIUnlockStructuredBuffer(particles_buffer);
	}
	if (grid_texture.IsValid() == false)
	{
		FRHIResourceCreateInfo CreateInfo;
		grid_texture = RHICreateTexture3D(
			N_Grid,
			N_Grid,
			N_Grid,
			PF_A32B32G32R32F,
			1,
			TexCreate_ShaderResource|TexCreate_UAV,
			CreateInfo
		);
		grid_texture_uav = RHICreateUnorderedAccessView(
			grid_texture
		);
	}

	InitializeTexture(grid_textureX, grid_textureX_uav);
	InitializeTexture(grid_textureY, grid_textureY_uav);
	InitializeTexture(grid_textureZ,grid_textureZ_uav);
	InitializeTexture(grid_textureW,grid_textureW_uav);
	
	if (grid_texture_atomic.IsValid() == false)
	{
		FRHIResourceCreateInfo CreateInfo;
		grid_texture_atomic = RHICreateStructuredBuffer(
			sizeof(UINT32),
			sizeof(UINT32) * N_Grid * N_Grid * N_Grid,
			BUF_ShaderResource | BUF_UnorderedAccess,
			CreateInfo
		);
		// grid_texture_atomic = RHICreateTexture3D(
  //           N_Grid,
  //           N_Grid,
  //           N_Grid,
  //           PF_R32_UINT,
  //           1,
  //           TexCreate_ShaderResource|TexCreate_UAV,
  //           CreateInfo
  //       );
		grid_texture_atomic_uav = RHICreateUnorderedAccessView(
			grid_texture_atomic, false, false
        );
	}
	
}
void FMLSMPMData::InitializeTexture(FTexture3DRHIRef& InOut_grid_texture, FUnorderedAccessViewRHIRef& InOut_grid_texture_uav)
{
	if (InOut_grid_texture.IsValid() == false)
	{
		FRHIResourceCreateInfo CreateInfo;
		InOut_grid_texture = RHICreateTexture3D(
            N_Grid,
            N_Grid,
            N_Grid,
            PF_R32_UINT,
            1,
            TexCreate_ShaderResource|TexCreate_UAV,
            CreateInfo
        );
		InOut_grid_texture_uav = RHICreateUnorderedAccessView(
            InOut_grid_texture
        );
	}
}
void FMLSMPMData::Release()
{
	// n_particles_buffer.SafeRelease();
	// n_particles_buffer_uav.SafeRelease();
	grid_texture.SafeRelease();
	grid_texture_uav.SafeRelease();
	particles_buffer.SafeRelease();
	particles_buffer_uav.SafeRelease();
	grid_texture_atomic.SafeRelease();
	grid_texture_atomic_uav.SafeRelease();
	grid_textureX.SafeRelease();
	grid_textureY.SafeRelease();
	grid_textureZ.SafeRelease();
	grid_textureW.SafeRelease();
	grid_textureX_uav.SafeRelease();
	grid_textureY_uav.SafeRelease();
	grid_textureZ_uav.SafeRelease();
	grid_textureW_uav.SafeRelease();
	FMLSMPMManager::UnregisterData(SharedThis(this));
	
}

void FMLSMPMData::Substep(FRHICommandList& RHICmdList)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_ShaderPlugin_MPLMPMSubStep); // Used to gather CPU profiling data for the UE4 session frontend
	SCOPED_DRAW_EVENT(RHICmdList, ShaderPlugin_MPLMPMSubStep);
	check(IsInRenderingThread());
	RHICmdList.ClearUAVFloat(grid_texture_uav, FVector4(0,0,0,0));
	RHICmdList.ClearUAVUint(grid_texture_atomic_uav, FUintVector4(0,0,0,0));
	RHICmdList.ClearUAVUint(grid_textureX_uav,FUintVector4(0,0,0,0) );
	RHICmdList.ClearUAVUint(grid_textureY_uav,FUintVector4(0,0,0,0) );
	RHICmdList.ClearUAVUint(grid_textureZ_uav,FUintVector4(0,0,0,0) );
	RHICmdList.ClearUAVUint(grid_textureW_uav,FUintVector4(0,0,0,0) );
	// NO!!!!!
	// uint8* Buffer = (uint8*)RHILockStructuredBuffer(n_particles_buffer, 0, sizeof(int), RLM_ReadOnly);
	// particle_num = (*(int*)(Buffer));
	// RHIUnlockStructuredBuffer(n_particles_buffer);
	//P2G
	{
		FMLSMPMShaderP2G::FParameters P2GParameters;
		GetParametersFromMLSMPMData(this, P2GParameters);
		TShaderMapRef<FMLSMPMShaderP2G> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
		int DispatchNum = particle_num / 8 + 1;
		FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, P2GParameters, FIntVector(DispatchNum, 1, 1));
	}

	//Grid
	{
		FMLSMPMShaderGridMain::FParameters GridMainParameters;
		GetParametersFromMLSMPMData(this, GridMainParameters);
		TShaderMapRef<FMLSMPMShaderGridMain> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
		FIntVector DispatchNum = FIntVector(N_Grid / 4, N_Grid / 4, N_Grid / 4);
		FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, GridMainParameters, DispatchNum);
	}

	//G2P
	{
		FMLSMPMShaderG2P::FParameters G2PParameters;
		GetParametersFromMLSMPMData(this, G2PParameters);
		TShaderMapRef<FMLSMPMShaderG2P> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
		int DispatchNum = particle_num / 8 + 1;
		FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, G2PParameters, FIntVector(DispatchNum, 1, 1));
	}
	if (visualizer != nullptr && visualizer->SDFTextureUAV.IsValid())
	{
		RHICmdList.ClearUAVFloat(visualizer->SDFTextureUAV, FVector4(0,0,0,0));
		FMPMToSDFTextureShader::FParameters PassParameters;
		PassParameters.n_particles = this->particle_num;
		PassParameters.texture_size = visualizer->GetTextureSize();
		PassParameters.particleDataBuffer = particles_buffer_uav;
		PassParameters.OutputTexture = visualizer->SDFTextureUAV;
		TShaderMapRef<FMPMToSDFTextureShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
		int DispatchNum = particle_num / 8 + 1;
		FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, PassParameters, FIntVector(DispatchNum, 1, 1));
	
	}
}
void FMLSMPMData::AddParticle(uint32 num)
{
	particle_num += num;
	NeedUpdateParticleNum = true;
}

void FMLSMPMManager::RegisterData(TSharedPtr<FMLSMPMData> data)
{
	MLSMPMDatas.Add(data);
}

void FMLSMPMManager::UnregisterData(TSharedPtr<FMLSMPMData> data)
{
	MLSMPMDatas.Remove(data);
}

void FMLSMPMManager::Update_RenderThread(FRHICommandList& RHICmdList)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_ShaderPlugin_MPLMPMUpdate); // Used to gather CPU profiling data for the UE4 session frontend
	SCOPED_DRAW_EVENT(RHICmdList, ShaderPlugin_MPLMPMUpdate);
	check(IsInRenderingThread());
	for (auto data : MLSMPMDatas)
	{
		for (int i = 0; i < 5; i++)
		{
			data->Substep(RHICmdList);
		}
		
	}
}


TArray<TSharedPtr<FMLSMPMData>> FMLSMPMManager::MLSMPMDatas;
