// Fill out your copyright notice in the Description page of Project Settings.


#include "MLSMPMManager.h"
#include "Shader.h"
#include "GlobalShader.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "ShaderCompilerCore.h"
#include "ShaderParameterStruct.h"


#define MLSMPMBUFFER_DEF BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )\
        SHADER_PARAMETER(float, dx)\
        SHADER_PARAMETER(float, inv_dx)\
        SHADER_PARAMETER(float, dt)\
        SHADER_PARAMETER(float, E)\
        SHADER_PARAMETER(float, p_rho)\
        SHADER_PARAMETER(float, p_vol)\
        SHADER_PARAMETER(float, p_mass)\
        SHADER_PARAMETER(float, gravity)\
        SHADER_PARAMETER(FVector4, collision)\
        SHADER_PARAMETER(UINT, n_particles)\
		SHADER_PARAMETER(UINT, n_grid)\
		SHADER_PARAMETER(UINT, max_particles)\
		SHADER_PARAMETER(UINT, bound)\
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
	//param.collisionData = data->collision_buffer_srv;
	//param.collision_data_size = data->collision_data_num;
	if(data->collision_data_cpu.Num() > 0)
	{
		param.collision = data->collision_data_cpu[0].AsFloat4();
	}
	// for(int i = 0; i < 8; i++)
	// {
	// 	if (i >= data->collision_data_cpu.Num())
	// 	{
	// 		param.collision_data_array[i] = FVector4(0,0,0,0);
	// 	}else
	// 	{
	// 		param.collision_data_array[i] = data->collision_data_cpu[i].AsFloat4();
	// 	}
	// }
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
	// static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	// {
	// 	FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	// 	// Force shader model 6.0+
	// 	OutEnvironment.CompilerFlags.Add(CFLAG_ForceDXC);
	// }
	
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

#define MPMPARTICLETOTEXTUERPARAMETER  BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )\
			SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)\
			SHADER_PARAMETER(UINT, n_particles)\
			SHADER_PARAMETER(UINT, texture_size)\
			SHADER_PARAMETER_UAV(RWStructuredBuffer<FParticleData>, particleDataBuffer)\
			SHADER_PARAMETER_UAV(RWTexture3D<uint>, SDFTempTexture)\
			SHADER_PARAMETER_UAV(RWTexture3D<float4>,OutputTexture)\
			SHADER_PARAMETER_UAV(RWTexture3D<uint>, Grid_W)\
			SHADER_PARAMETER_UAV(RWStructuredBuffer<uint>, GridCoordBuffer)\
		END_SHADER_PARAMETER_STRUCT()
class FMPMToSDFTextureShader : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FMPMToSDFTextureShader);
	SHADER_USE_PARAMETER_STRUCT(FMPMToSDFTextureShader, FGlobalShader);
	MPMPARTICLETOTEXTUERPARAMETER
public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};
IMPLEMENT_GLOBAL_SHADER(FMPMToSDFTextureShader, "/TutorialShaders/Private/MPMParticleToSDFTexture.usf", "Main", SF_Compute);

class FScatterParticleToTempBufferShader : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FScatterParticleToTempBufferShader);
	SHADER_USE_PARAMETER_STRUCT(FScatterParticleToTempBufferShader, FGlobalShader);
	MPMPARTICLETOTEXTUERPARAMETER
public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};
IMPLEMENT_GLOBAL_SHADER(FScatterParticleToTempBufferShader, "/TutorialShaders/Private/MPMParticleToSDFTexture.usf", "ScatterParticleToTempTexture", SF_Compute);

class FTempBufferToOutputTextureShader : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FTempBufferToOutputTextureShader);
	SHADER_USE_PARAMETER_STRUCT(FTempBufferToOutputTextureShader, FGlobalShader);
	MPMPARTICLETOTEXTUERPARAMETER
public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};
IMPLEMENT_GLOBAL_SHADER(FTempBufferToOutputTextureShader, "/TutorialShaders/Private/MPMParticleToSDFTexture.usf", "UpdateOutputTexture", SF_Compute);


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
	FMLSMPMManager::RegisterData(this);
	//particle_num = 10;
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
			FVector(0.0f, -1.0f, -3.0f),
			1.0f,
			0.0f,
			FMatrix(EForceInit::ForceInitToZero)
		};
		UINT initialNum = 0;
		for (UINT i = 10; i < N_Grid - 10; i += 2 )
		{
			for (UINT j = 10; j < N_Grid - 10; j+= 2)
			{
				for (UINT k = 35; k < 60; k+= 2)
				{
					FParticleData instance = initialData;
					instance.x = FVector(i,j,k) / (float)N_Grid;
					instance.v = FVector::ZeroVector;
					InitialData.Add(instance);
					initialNum ++;
				}
			}
		}
		this->particle_num = initialNum;
		for (UINT i = initialNum; i < MaxParticles; i ++)
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

		grid_texture_atomic_uav = RHICreateUnorderedAccessView(
			grid_texture_atomic, false, false
        );
	}
	if (sdf_temp_texture.IsValid() == false && visualizer != nullptr && IsValid(visualizer))
	{
		FRHIResourceCreateInfo CreateInfo;
		uint32 size = visualizer->GetTextureSize();
		sdf_temp_texture =  RHICreateTexture3D(
            size,
            size,
            size,
            PF_R32_UINT,
            1,
            TexCreate_ShaderResource|TexCreate_UAV,
            CreateInfo
        );
		sdf_temp_texture_uav = RHICreateUnorderedAccessView(sdf_temp_texture);
	}
	if (collision_buffer.IsValid() == false)
	{
		FRHIResourceCreateInfo CreateInfo;
		collision_buffer = RHICreateStructuredBuffer(
			sizeof(FCollisionData),
			sizeof(FCollisionData) * max_collision_count,
			BUF_ShaderResource | BUF_UnorderedAccess,
			CreateInfo
		);
		collision_buffer_srv = RHICreateShaderResourceView(
			collision_buffer
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
	sdf_temp_texture.SafeRelease();
	sdf_temp_texture_uav.SafeRelease();
	collision_buffer.SafeRelease();
	collision_buffer_srv.SafeRelease();
	FMLSMPMManager::UnregisterData(this);
	
}
void FMLSMPMData::SetCollision(const TArray<FCollisionData>& CollisionDatas)
{
	FMLSMPMData* Self = this;
	collision_data_num = CollisionDatas.Num();
	collision_data_cpu = CollisionDatas;
	ENQUEUE_RENDER_COMMAND(SetCollisionData)
    (
        [Self, CollisionDatas](FRHICommandList& RHICmdList)
        {
            void* Buffer = RHILockStructuredBuffer(Self->collision_buffer, 0, sizeof(FCollisionData) * Self->max_collision_count, EResourceLockMode::RLM_WriteOnly );
        	memcpy(Buffer, CollisionDatas.GetData(), CollisionDatas.Num() * sizeof(FCollisionData));
        	RHIUnlockStructuredBuffer(Self->collision_buffer);
        }
    );
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
		int DispatchNum = particle_num / GROUP_SIZE + 1;
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
		int DispatchNum = particle_num / GROUP_SIZE + 1;
		FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, G2PParameters, FIntVector(DispatchNum, 1, 1));
	}
	
}



void FMLSMPMData::Visualize(FRHICommandList& RHICmdList, FPostOpaqueRenderParameters& Parameters)
{
	if (visualizer != nullptr && visualizer->SDFTextureUAV.IsValid())
	{
		RHICmdList.ClearUAVFloat(visualizer->SDFTextureUAV, FVector4(0,0,0,0));
		RHICmdList.ClearUAVUint(sdf_temp_texture_uav, FUintVector4(0,0,0,0));
		RHICmdList.ClearUAVUint(visualizer->GridIndexBufferUAV, FUintVector4(0,0,0,0));
		{
			//Scatter Pass
			FScatterParticleToTempBufferShader::FParameters ScatterPassParams;
			ScatterPassParams.n_particles = this->particle_num;
			ScatterPassParams.texture_size = visualizer->GetTextureSize();
			ScatterPassParams.particleDataBuffer = particles_buffer_uav;
			ScatterPassParams.SDFTempTexture = sdf_temp_texture_uav;
			const TShaderMapRef<FScatterParticleToTempBufferShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			int DispatchNum = particle_num / 1024 + 1;
			FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, ScatterPassParams, FIntVector(DispatchNum, 1, 1));
		}
		{
			//Output Update Pass
			FTempBufferToOutputTextureShader::FParameters OutputUpdatePassParams;
			OutputUpdatePassParams.n_particles = this->particle_num;
			OutputUpdatePassParams.texture_size = visualizer->GetTextureSize();
			OutputUpdatePassParams.particleDataBuffer = particles_buffer_uav;
			OutputUpdatePassParams.SDFTempTexture = sdf_temp_texture_uav;
			OutputUpdatePassParams.OutputTexture = visualizer->SDFTextureUAV;
			OutputUpdatePassParams.GridCoordBuffer = visualizer->GridIndexBufferUAV;
			OutputUpdatePassParams.Grid_W = this->grid_textureW_uav;
			TUniformBufferRef<FViewUniformShaderParameters> Ref;
			*Ref.GetInitReference() = Parameters.ViewUniformBuffer;
			Ref->AddRef();
			OutputUpdatePassParams.View = Ref;
			
			const TShaderMapRef<FTempBufferToOutputTextureShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			const FIntVector DispatchNum = FIntVector(
				OutputUpdatePassParams.texture_size / 4 + 1
			);
			
			FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, OutputUpdatePassParams, DispatchNum);
			//Ref->Release();
			//*Ref.GetInitReference() = nullptr;
			
		}
	// 	FMPMToSDFTextureShader::FParameters PassParameters;
	// 	PassParameters.n_particles = this->particle_num;
	// 	PassParameters.texture_size = visualizer->GetTextureSize();
	// 	PassParameters.particleDataBuffer = particles_buffer_uav;
	// 	PassParameters.OutputTexture = visualizer->SDFTextureUAV;
	// 	TShaderMapRef<FMPMToSDFTextureShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	// 	int DispatchNum = particle_num / 8 + 1;
	// 	FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, PassParameters, FIntVector(DispatchNum, 1, 1));
	}
}
void FMLSMPMData::AddParticle(uint32 num)
{
	particle_num += num;
	NeedUpdateParticleNum = true;
}

void FMLSMPMManager::RegisterData(FMLSMPMData* data)
{
	MLSMPMDatas.Add(data);
}

void FMLSMPMManager::UnregisterData(FMLSMPMData* data)
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
		int SubStepCount = data->gameThreadTickTime / data->dt;
		for (int i = 0; i < 12; i++)
		{
			data->Substep(RHICmdList);
		}
		//data->Visualize(RHICmdList);
	}
}
void FMLSMPMManager::Visualize_RenderThread(FRHICommandList& RHICmdList, FPostOpaqueRenderParameters& Parameters)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_ShaderPlugin_MPLMPMVisualize); // Used to gather CPU profiling data for the UE4 session frontend
	SCOPED_DRAW_EVENT(RHICmdList, ShaderPlugin_MPLMPMVisualize);
	check(IsInRenderingThread());
	for (auto data : MLSMPMDatas)
	{
		data->Visualize(RHICmdList, Parameters);
	}
}

TArray<FMLSMPMData*> FMLSMPMManager::MLSMPMDatas;
