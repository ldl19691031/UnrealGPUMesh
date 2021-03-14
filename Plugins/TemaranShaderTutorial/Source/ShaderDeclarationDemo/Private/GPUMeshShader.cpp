#include "GPUMeshShader.h"
#include "ShaderParameterUtils.h"
#include "RHIStaticStates.h"
#include "Shader.h"
#include "GlobalShader.h"
#include "GPUMeshComponent.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "ShaderParameterStruct.h"
#include "UniformBuffer.h"
#include "RHICommandList.h"
#include "Engine/VolumeTexture.h"
#include "MarchingCubeDef.h"


#define NUM_THREADS_PER_GROUP_DIMENSION 8
/**********************************************************************************************/
/* This class carries our parameter declarations and acts as the bridge between cpp and HLSL. */
/**********************************************************************************************/
class FGPUMeshShaderCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FGPUMeshShaderCS);
	SHADER_USE_PARAMETER_STRUCT(FGPUMeshShaderCS, FGlobalShader);

	struct Vertex
	{
		FVector vPosition;
		FVector vNormal;
	};

	struct Triangle
	{
		Vertex v[3];
	};

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		//SHADER_PARAMETER_UAV(RWTexture3D<float4>, InputTexture)
		SHADER_PARAMETER_TEXTURE(Texture3D<float4>, InputTexture)
		SHADER_PARAMETER_TEXTURE(Texture2D<uint>, TriTableTexture)
		SHADER_PARAMETER_UAV(RWStructuredBuffer<uint>, RWVertexCounterBuffer)
		SHADER_PARAMETER_UAV(RWTexture2D<float4>, OutputTexture)
		SHADER_PARAMETER_UAV(AppendStructuredBuffer<Triangle>,OutputBufferTest)
		SHADER_PARAMETER_SAMPLER(SamplerState, myLinearClampSampler)
		SHADER_PARAMETER(FVector4, InputTextureSize)
		SHADER_PARAMETER(FVector4, InputSlice)
		SHADER_PARAMETER(float,isoLevel)
		SHADER_PARAMETER(int, gridSize)
		SHADER_PARAMETER(int, vertexCounter)
	END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static inline void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_X"), NUM_THREADS_PER_GROUP_DIMENSION);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Y"), NUM_THREADS_PER_GROUP_DIMENSION);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Z"), NUM_THREADS_PER_GROUP_DIMENSION);
	}
};
class FGPUMeshShaderCS2 : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FGPUMeshShaderCS2);
	SHADER_USE_PARAMETER_STRUCT(FGPUMeshShaderCS2, FGlobalShader);

	struct Vertex
	{
		FVector vPosition;
		FVector vNormal;
	};

	struct Triangle
	{
		Vertex v[3];
	};

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        //SHADER_PARAMETER_UAV(RWTexture3D<float4>, InputTexture)
        SHADER_PARAMETER_TEXTURE(Texture3D<float4>, InputTexture)
        SHADER_PARAMETER_TEXTURE(Texture2D<uint>, TriTableTexture)
        SHADER_PARAMETER_UAV(RWStructuredBuffer<uint>, RWVertexCounterBuffer)
        //SHADER_PARAMETER_UAV(RWTexture2D<float4>, OutputTexture)
		SHADER_PARAMETER_UAV(RWStructuredBuffer<float3>, RWVertexPositionBuffer)
		SHADER_PARAMETER_UAV(RWStructuredBuffer<FPackedNormal>, RWVertexTangentBuffer)
        SHADER_PARAMETER_UAV(AppendStructuredBuffer<Triangle>,OutputBufferTest)
        SHADER_PARAMETER_SAMPLER(SamplerState, myLinearClampSampler)
        SHADER_PARAMETER(FVector4, InputTextureSize)
        SHADER_PARAMETER(FVector4, InputSlice)
		SHADER_PARAMETER(FVector,vertexOffset)
		SHADER_PARAMETER(FVector,vertexScale)
        SHADER_PARAMETER(float,isoLevel)
        SHADER_PARAMETER(int, gridSize)
        SHADER_PARAMETER(int, vertexCounter)
    END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static inline void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_X"), NUM_THREADS_PER_GROUP_DIMENSION);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Y"), NUM_THREADS_PER_GROUP_DIMENSION);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Z"), NUM_THREADS_PER_GROUP_DIMENSION);
	}
};
class FGPUMeshCopyToTextureCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FGPUMeshCopyToTextureCS);
	SHADER_USE_PARAMETER_STRUCT(FGPUMeshCopyToTextureCS, FGlobalShader);

	struct Vertex
	{
		FVector vPosition;
		FVector vNormal;
	};

	struct Triangle
	{
		Vertex v[3];
	};
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
			SHADER_PARAMETER_UAV(ConsumeStructuredBuffer<Triangle>,OutputBufferTest2)
            SHADER_PARAMETER_UAV(RWTexture2D<float4>, OutputTexture)
      END_SHADER_PARAMETER_STRUCT()
public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

};

class FGPUSDFModifyCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FGPUSDFModifyCS);
	SHADER_USE_PARAMETER_STRUCT(FGPUSDFModifyCS, FGlobalShader);

	struct Brush
	{
		FVector position;
		float radius;
	};
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
            SHADER_PARAMETER_SRV(StructuredBuffer<Brush>,InputBrushList)
            SHADER_PARAMETER_UAV(RWTexture3D<float4>, OutputTexture)
			SHADER_PARAMETER(uint32, BrushCount)
			SHADER_PARAMETER(float,Smoothness)
      END_SHADER_PARAMETER_STRUCT()
public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

};
// This will tell the engine to create the shader and where the shader entry point is.
//                            ShaderType                            ShaderPath                     Shader function name    Type
IMPLEMENT_GLOBAL_SHADER(FGPUMeshShaderCS, "/TutorialShaders/Private/GPUMeshShaderCS.usf", "MainComputeShader", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FGPUMeshCopyToTextureCS, "/TutorialShaders/Private/GPUMeshCopyToTextureCS.usf", "MainComputeShader", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FGPUSDFModifyCS, "/TutorialShaders/Private/GPUSDFModifier.usf","MainComputeShader",SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FGPUMeshShaderCS2, "/TutorialShaders/Private/GPUMeshShaderCS2.usf", "MainComputeShader", SF_Compute);

static FTexture2DRHIRef TriLookUpTexture;
static FStructuredBufferRHIRef VertexCounterBuffer;
static FUnorderedAccessViewRHIRef VertexCounterBufferUAV;
static FStructuredBufferRHIRef AppendBufferTest;
static FUnorderedAccessViewRHIRef AppendBufferUAV;
static FUnorderedAccessViewRHIRef ConsumeBufferUAV;
static FStructuredBufferRHIRef BrushBuffer;
static FShaderResourceViewRHIRef BrushBufferShaderResourceView;
static FSamplerStateRHIRef SamplerState;
static FTexture2DRHIRef ShaderOutputTexture;
static FUnorderedAccessViewRHIRef ShaderOutputTextureUAV;
void FGPUMeshShader::UpdateSDFTextureByBrushGPU(FRHICommandListImmediate& RHICmdList, const FGPUMeshParameters& DrawParameters, FTexture3DRHIRef SDFTexture, FUnorderedAccessViewRHIRef SDFTextureUAV)
{
	if (BrushBuffer.IsValid() == false)
	{
		FRHIResourceCreateInfo rhiCreateInfo;
		BrushBuffer = RHICreateStructuredBuffer(
			sizeof(FGPUSDFModifyCS::Brush),
			sizeof(FGPUSDFModifyCS::Brush) * 64,
			BUF_ShaderResource| BUF_UnorderedAccess,
			rhiCreateInfo
		);
		BrushBufferShaderResourceView = RHICreateShaderResourceView(BrushBuffer);
			
	}
	FGPUSDFModifyCS::FParameters SdfShaderParameters;
	SdfShaderParameters.BrushCount = DrawParameters.BrushList.Num();
	void* LockedMemory = RHILockStructuredBuffer(BrushBuffer,0, BrushBuffer->GetSize(),EResourceLockMode::RLM_WriteOnly);
	FMemory::Memcpy(LockedMemory,DrawParameters.BrushList.GetData(),BrushBuffer->GetSize());
	RHIUnlockStructuredBuffer(BrushBuffer);
	SdfShaderParameters.InputBrushList = BrushBufferShaderResourceView;
	SdfShaderParameters.OutputTexture = SDFTextureUAV;
	TShaderMapRef<FGPUSDFModifyCS> SDFComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	FIntVector GroupCounts = FIntVector(
		SDFTexture->GetSizeX() / 8,
		SDFTexture->GetSizeY() / 8,
		1
	);
	FComputeShaderUtils::Dispatch(RHICmdList,SDFComputeShader, SdfShaderParameters, GroupCounts);
}

void FGPUMeshShader::InitBrushBuffer(FStructuredBufferRHIRef& BrushBuffer_Local,
	FShaderResourceViewRHIRef& BrushBufferShaderResourceView_Local)
{
	FRHIResourceCreateInfo rhiCreateInfo;
	BrushBuffer_Local = RHICreateStructuredBuffer(
        sizeof(FGPUSDFModifyCS::Brush),
        sizeof(FGPUSDFModifyCS::Brush) * 64,
        BUF_ShaderResource| BUF_UnorderedAccess,
        rhiCreateInfo
    );
	BrushBufferShaderResourceView_Local = RHICreateShaderResourceView(BrushBuffer_Local);
}

void FGPUMeshShader::UpdateSdfTextureByBrushGPU(FRHICommandListImmediate& RHICmdList,
	const TArray<FGPUMeshParameters::FBrushParameter>& Brushes, FStructuredBufferRHIRef BrushBuffer_Local,
	FShaderResourceViewRHIRef BrushBufferShaderResourceView_Local, FTexture3DRHIRef SDFTexture,
	FUnorderedAccessViewRHIRef SDFTextureUAV,const FSDFBrushRenderControlParameter& ControlParameter)
{
	FGPUSDFModifyCS::FParameters SdfShaderParameters;
	SdfShaderParameters.BrushCount = Brushes.Num();
	SdfShaderParameters.Smoothness = ControlParameter.Smoothness;
	void* LockedMemory = RHILockStructuredBuffer(BrushBuffer_Local,0, BrushBuffer_Local->GetSize(),EResourceLockMode::RLM_WriteOnly);
	FMemory::Memcpy(LockedMemory,Brushes.GetData(),BrushBuffer_Local->GetSize());
	RHIUnlockStructuredBuffer(BrushBuffer_Local);
	SdfShaderParameters.InputBrushList = BrushBufferShaderResourceView_Local;
	SdfShaderParameters.OutputTexture = SDFTextureUAV;
	RHICmdList.ClearUAVFloat(SDFTextureUAV, FVector4(0,0,0,0));
	TShaderMapRef<FGPUSDFModifyCS> SDFComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	FIntVector GroupCounts = FIntVector(
        SDFTexture->GetSizeX() / 8,
        SDFTexture->GetSizeY() / 8,
        1
    );
	FComputeShaderUtils::Dispatch(RHICmdList,SDFComputeShader, SdfShaderParameters, GroupCounts);
}



void FGPUMeshShader::RunComputeShader_RenderThread(
	FRHICommandListImmediate& RHICmdList, 
	const FGPUMeshParameters& DrawParameters, 
	FUnorderedAccessViewRHIRef ComputeShaderInputUAV,
	FUnorderedAccessViewRHIRef ComputeShaderOutputUAV)
{
	if (DrawParameters.OutputVertexPositionImage == nullptr)
	{
		return;
	}
	QUICK_SCOPE_CYCLE_COUNTER(STAT_ShaderPlugin_GPUMeshComputeShader); // Used to gather CPU profiling data for the UE4 session frontend
	SCOPED_DRAW_EVENT(RHICmdList, ShaderPlugin_GPUMeshCompute); // Used to profile GPU activity and add metadata to be consumed by for example RenderDoc
	RHICmdList.TransitionResource(EResourceTransitionAccess::ERWBarrier, EResourceTransitionPipeline::EGfxToCompute, ComputeShaderInputUAV);
	RHICmdList.TransitionResource(EResourceTransitionAccess::ERWBarrier, EResourceTransitionPipeline::EGfxToCompute, ComputeShaderOutputUAV);
	FGPUMeshShaderCS::FParameters PassParameters;
	FGPUMeshShaderCS::FParameters EmptyParameters;
	
	//PassParameters.InputTexture = ComputeShaderInputUAV;
	FRHIResourceCreateInfo CreateInfo;

	if (ShaderOutputTexture.IsValid() == false)
	{
		ShaderOutputTexture = RHICreateTexture2D(
			DrawParameters.OutputVertexPositionImage->SizeX,
			DrawParameters.OutputVertexPositionImage->SizeY,
			PF_A32B32G32R32F,
			1, 1,
			TexCreate_ShaderResource | TexCreate_UAV, CreateInfo);
		ShaderOutputTextureUAV = RHICreateUnorderedAccessView(ShaderOutputTexture);
	}
	FTexture2DRHIRef OriginalRT = DrawParameters.OutputVertexPositionImage->GetRenderTargetResource()->GetRenderTargetTexture();
	FTexture3DRHIRef SDFTexture = RHICreateTexture3D(
	DrawParameters.SDFTexture->GetSizeX(),
	DrawParameters.SDFTexture->GetSizeY(),
	DrawParameters.SDFTexture->GetSizeZ(),
	PF_B8G8R8A8,
	1,
	TexCreate_ShaderResource | TexCreate_UAV,
	CreateInfo
	);
	FRHICopyTextureInfo copyInfo;
	{
		copyInfo.NumMips = 0;
		copyInfo.SourceMipIndex = 0;
		copyInfo.DestMipIndex = 0;
		copyInfo.NumSlices = DrawParameters.SDFTexture->GetSizeZ();
	}
	RHICmdList.CopyTexture(DrawParameters.SDFTexture->Resource->TextureRHI, SDFTexture, copyInfo);
	if (VertexCounterBuffer.IsValid() == false)
	{
		FRHIResourceCreateInfo rhiCreateInfo;
		VertexCounterBuffer = RHICreateStructuredBuffer(sizeof(int), sizeof(int), BUF_ShaderResource| BUF_UnorderedAccess,rhiCreateInfo);
		VertexCounterBufferUAV = RHICreateUnorderedAccessView(VertexCounterBuffer,false,false);
		
		uint8* Buffer = (uint8*)RHILockStructuredBuffer(VertexCounterBuffer, 0, sizeof(int), RLM_WriteOnly);
		(*(int*)(Buffer))= 0;
		RHIUnlockStructuredBuffer(VertexCounterBuffer);
	}
	RHICmdList.TransitionResource(EResourceTransitionAccess::ERWBarrier, EResourceTransitionPipeline::EGfxToCompute, VertexCounterBufferUAV);
	RHIGetDefaultContext()->RHIClearUAVUint(VertexCounterBufferUAV,FUintVector4(0));
	
	PassParameters.RWVertexCounterBuffer = VertexCounterBufferUAV;
	
	FUnorderedAccessViewRHIRef SDFTextureUAV = RHICreateUnorderedAccessView(SDFTexture);

	if (DrawParameters.BrushList.Num()>0)
	{
		UpdateSDFTextureByBrushGPU(RHICmdList, DrawParameters, SDFTexture, SDFTextureUAV);
	}
	
	PassParameters.InputTexture = SDFTexture;
	RHICmdList.TransitionResource(EResourceTransitionAccess::ERWBarrier, EResourceTransitionPipeline::EGfxToCompute, ShaderOutputTextureUAV);
	//RHIGetDefaultContext()->RHIClearUAVFloat(TextureUAV, FVector4(0.0f));
	PassParameters.OutputTexture = ShaderOutputTextureUAV;//DrawParameters.OutputVertexPositionImage->GetRenderTargetResource()->GetRenderTargetUAV();
	PassParameters.InputTextureSize = FVector4(DrawParameters.SDFTexture->GetSizeX(), DrawParameters.SDFTexture->GetSizeY(), DrawParameters.SDFTexture->GetSizeZ(), 0);
	PassParameters.InputSlice = FVector4(ShaderOutputTexture->GetSizeX() / SDFTexture->GetSizeX(), ShaderOutputTexture->GetSizeY() / SDFTexture->GetSizeY(), 0, 0);
	PassParameters.isoLevel = 0.5f;
	PassParameters.gridSize = SDFTexture->GetSizeX();
	PassParameters.vertexCounter = 0;
	if (SamplerState.IsValid() == false)
	{
		SamplerState =  RHICreateSamplerState(FSamplerStateInitializerRHI(
			ESamplerFilter::SF_Bilinear,
			ESamplerAddressMode::AM_Clamp,
			ESamplerAddressMode::AM_Clamp,
			ESamplerAddressMode::AM_Clamp
		));
	}
	PassParameters.myLinearClampSampler = SamplerState;
	if (TriLookUpTexture.IsValid() == false)
	{
		FRHIResourceCreateInfo createInfoTexture;
		TriLookUpTexture = RHICreateTexture2D(
			16,
			256,
			PF_R8_UINT,
			1,
			1,
			TexCreate_ShaderResource | TexCreate_UAV,
			createInfoTexture
			);
		uint32 DestStride;
		void* data = RHICmdList.LockTexture2D(TriLookUpTexture,0, EResourceLockMode::RLM_WriteOnly,DestStride, false);
		memcpy(data, triTable, sizeof(triTable) * sizeof(uint8));
		RHICmdList.UnlockTexture2D(TriLookUpTexture, 0, false);
	}
	PassParameters.TriTableTexture = TriLookUpTexture;

	if (AppendBufferTest.IsValid() == false)
	{
		FRHIResourceCreateInfo rhiCreateInfo;
		AppendBufferTest = RHICreateStructuredBuffer(
			sizeof(FGPUMeshShaderCS::Triangle),
			sizeof(FGPUMeshShaderCS::Triangle) * 4096,
			BUF_ShaderResource|BUF_UnorderedAccess|BUF_StructuredBuffer,
			rhiCreateInfo
		);

		AppendBufferUAV = RHICreateUnorderedAccessView(AppendBufferTest,false,true);
		
	}
	
	PassParameters.OutputBufferTest = AppendBufferUAV;
	TShaderMapRef<FGPUMeshShaderCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	FIntVector GroupCounts = FIntVector(
		SDFTexture->GetSizeX() / NUM_THREADS_PER_GROUP_DIMENSION,
		SDFTexture->GetSizeY() / NUM_THREADS_PER_GROUP_DIMENSION,
		SDFTexture->GetSizeZ() / NUM_THREADS_PER_GROUP_DIMENSION
		);

	FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, PassParameters, GroupCounts);

	RHIAcquireTransientResource(ShaderOutputTexture);
	FRHICopyTextureInfo CopyInfo = FRHICopyTextureInfo();

	RHICmdList.CopyToResolveTarget(ShaderOutputTexture, OriginalRT, FResolveParams());
}

void FGPUMeshShader::UpdateMeshBySDFGPU(
	FRHICommandListImmediate& RHICmdList,
	FTexture3DRHIRef SDFTexture,
	FGPUMeshVertexBuffers* VertexBuffers,
	const FGPUMeshControlParams& ControlParams
	)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_ShaderPlugin_GPUMeshComputeShaderUpdate); // Used to gather CPU profiling data for the UE4 session frontend
	SCOPED_DRAW_EVENT(RHICmdList, ShaderPlugin_GPUMeshComputeShaderUpdate); // Used to profile GPU activity and add metadata to be consumed by for example RenderDoc
	
	if (SDFTexture.IsValid() == false || VertexBuffers == nullptr)
	{
		return;
	}
	FGPUMeshShaderCS2::FParameters PassParameters;
	PassParameters.vertexOffset = ControlParams.Offset;
	PassParameters.vertexScale = ControlParams.Scale;
	PassParameters.InputTexture = SDFTexture;
	if (VertexCounterBuffer.IsValid() == false)
	{
		FRHIResourceCreateInfo rhiCreateInfo;
		VertexCounterBuffer = RHICreateStructuredBuffer(sizeof(int), sizeof(int), BUF_ShaderResource| BUF_UnorderedAccess,rhiCreateInfo);
		VertexCounterBufferUAV = RHICreateUnorderedAccessView(VertexCounterBuffer,false,false);
		
		uint8* Buffer = (uint8*)RHILockStructuredBuffer(VertexCounterBuffer, 0, sizeof(int), RLM_WriteOnly);
		(*(int*)(Buffer))= 0;
		RHIUnlockStructuredBuffer(VertexCounterBuffer);
	}
	RHICmdList.TransitionResource(EResourceTransitionAccess::ERWBarrier, EResourceTransitionPipeline::EGfxToCompute, VertexCounterBufferUAV);
	RHIGetDefaultContext()->RHIClearUAVUint(VertexCounterBufferUAV,FUintVector4(0));
	RHIGetDefaultContext()->RHIClearUAVFloat( VertexBuffers->PositionVertexBuffer.UnorderedAccessViewRHI, FVector4(0,0,0,0));
	RHIGetDefaultContext()->RHIClearUAVFloat( VertexBuffers->DynamicTangentXBuffer.UnorderedAccessViewRHI, FVector4(0,0,0,0));
	PassParameters.RWVertexCounterBuffer = VertexCounterBufferUAV;

	PassParameters.InputTextureSize = FVector4(SDFTexture->GetSizeX(), SDFTexture->GetSizeY(), SDFTexture->GetSizeZ(), 0);
	//PassParameters.InputSlice = FVector4(ShaderOutputTexture->GetSizeX() / SDFTexture->GetSizeX(), ShaderOutputTexture->GetSizeY() / SDFTexture->GetSizeY(), 0, 0);
	PassParameters.isoLevel = 0.5f;
	PassParameters.gridSize = SDFTexture->GetSizeX();
	PassParameters.vertexCounter = 0;
	if (SamplerState.IsValid() == false)
	{
		SamplerState =  RHICreateSamplerState(FSamplerStateInitializerRHI(
            ESamplerFilter::SF_Trilinear,
            ESamplerAddressMode::AM_Clamp,
            ESamplerAddressMode::AM_Clamp,
            ESamplerAddressMode::AM_Clamp
        ));
	}
	PassParameters.myLinearClampSampler = SamplerState;
	if (TriLookUpTexture.IsValid() == false)
	{
		FRHIResourceCreateInfo createInfoTexture;
		TriLookUpTexture = RHICreateTexture2D(
            16,
            256,
            PF_R8_UINT,
            1,
            1,
            TexCreate_ShaderResource | TexCreate_UAV,
            createInfoTexture
            );
		uint32 DestStride;
		void* data = RHICmdList.LockTexture2D(TriLookUpTexture,0, EResourceLockMode::RLM_WriteOnly,DestStride, false);
		memcpy(data, triTable, sizeof(triTable) * sizeof(uint8));
		RHICmdList.UnlockTexture2D(TriLookUpTexture, 0, false);
	}
	PassParameters.TriTableTexture = TriLookUpTexture;
	
	TShaderMapRef<FGPUMeshShaderCS2> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	FIntVector GroupCounts = FIntVector(
        SDFTexture->GetSizeX() / NUM_THREADS_PER_GROUP_DIMENSION,
        SDFTexture->GetSizeY() / NUM_THREADS_PER_GROUP_DIMENSION,
        SDFTexture->GetSizeZ() / NUM_THREADS_PER_GROUP_DIMENSION
        );

	PassParameters.RWVertexPositionBuffer = VertexBuffers->PositionVertexBuffer.UnorderedAccessViewRHI;
	PassParameters.RWVertexTangentBuffer = VertexBuffers->DynamicTangentXBuffer.UnorderedAccessViewRHI;
	FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, PassParameters, GroupCounts);
}
