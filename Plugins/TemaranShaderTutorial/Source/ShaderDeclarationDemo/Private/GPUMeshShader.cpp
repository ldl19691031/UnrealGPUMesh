#include "GPUMeshShader.h"
#include "ShaderParameterUtils.h"
#include "RHIStaticStates.h"
#include "Shader.h"
#include "GlobalShader.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "ShaderParameterStruct.h"
#include "UniformBuffer.h"
#include "RHICommandList.h"
#include "Engine/VolumeTexture.h"

#define NUM_THREADS_PER_GROUP_DIMENSION 8
/**********************************************************************************************/
/* This class carries our parameter declarations and acts as the bridge between cpp and HLSL. */
/**********************************************************************************************/
class FGPUMeshShaderCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FGPUMeshShaderCS);
	SHADER_USE_PARAMETER_STRUCT(FGPUMeshShaderCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		//SHADER_PARAMETER_UAV(RWTexture3D<float4>, InputTexture)
		SHADER_PARAMETER_TEXTURE(Texture3D<float4>, InputTexture)
		SHADER_PARAMETER_UAV(RWTexture2D<float4>, OutputTexture)
		SHADER_PARAMETER(FVector4, InputTextureSize)
		SHADER_PARAMETER(FVector4, InputSlice)
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
// This will tell the engine to create the shader and where the shader entry point is.
//                            ShaderType                            ShaderPath                     Shader function name    Type
IMPLEMENT_GLOBAL_SHADER(FGPUMeshShaderCS, "/TutorialShaders/Private/GPUMeshShaderCS.usf", "MainComputeShader", SF_Compute);

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
	//PassParameters.InputTexture = ComputeShaderInputUAV;
	FRHIResourceCreateInfo CreateInfo;
	FTexture2DRHIRef Texture = RHICreateTexture2D(DrawParameters.OutputVertexPositionImage->SizeX, DrawParameters.OutputVertexPositionImage->SizeY, PF_A32B32G32R32F, 1, 1, TexCreate_ShaderResource | TexCreate_UAV, CreateInfo);
	FTexture2DRHIRef OriginalRT = DrawParameters.OutputVertexPositionImage->GetRenderTargetResource()->GetRenderTargetTexture();
	FUnorderedAccessViewRHIRef TextureUAV = RHICreateUnorderedAccessView(Texture);
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

	FUnorderedAccessViewRHIRef SDFTextureUAV = RHICreateUnorderedAccessView(SDFTexture);
	
	PassParameters.InputTexture = SDFTexture;

	PassParameters.OutputTexture = TextureUAV;//DrawParameters.OutputVertexPositionImage->GetRenderTargetResource()->GetRenderTargetUAV();
	PassParameters.InputTextureSize = FVector4(DrawParameters.SDFTexture->GetSizeX(), DrawParameters.SDFTexture->GetSizeY(), DrawParameters.SDFTexture->GetSizeZ(), 0);
	PassParameters.InputSlice = FVector4(Texture->GetSizeX() / SDFTexture->GetSizeX(), Texture->GetSizeY() / SDFTexture->GetSizeY(), 0, 0);
	if (PassParameters.OutputTexture == nullptr)
	{
		return;
	}
	TShaderMapRef<FGPUMeshShaderCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	FIntVector GroupCounts = FIntVector(
		SDFTexture->GetSizeX() / NUM_THREADS_PER_GROUP_DIMENSION,
		SDFTexture->GetSizeY() / NUM_THREADS_PER_GROUP_DIMENSION,
		SDFTexture->GetSizeZ() / NUM_THREADS_PER_GROUP_DIMENSION
		);

	FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, PassParameters, GroupCounts);
	
	FRHICopyTextureInfo CopyInfo = FRHICopyTextureInfo();
	RHICmdList.CopyTexture(Texture, OriginalRT, CopyInfo);
	Texture.SafeRelease();
	
}
