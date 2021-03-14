#pragma once
#pragma once

#include "CoreMinimal.h"
#include "ShaderDeclarationDemoModule.h"

/**************************************************************************************/
/* This is just an interface we use to keep all the compute shading code in one file. */
/**************************************************************************************/
class FGPUMeshShader
{
public:
	static void UpdateSDFTextureByBrushGPU(FRHICommandListImmediate& RHICmdList,
	                                       const FGPUMeshParameters& DrawParameters,
	                                       FTexture3DRHIRef SDFTexture, FUnorderedAccessViewRHIRef SDFTextureUAV);

	static void InitBrushBuffer(
	FStructuredBufferRHIRef& BrushBuffer,
	FShaderResourceViewRHIRef& BrushBufferShaderResourceView
	);
	struct FSDFBrushRenderControlParameter
	{
		float Smoothness;
	};
	static void UpdateSdfTextureByBrushGPU(
		FRHICommandListImmediate& RHICmdList,
		const TArray<FGPUMeshParameters::FBrushParameter>& Brushes,
		FStructuredBufferRHIRef BrushBuffer_Local,
		FShaderResourceViewRHIRef BrushBufferShaderResourceView,
        FTexture3DRHIRef SDFTexture,
        FUnorderedAccessViewRHIRef SDFTextureUAV,
        const FSDFBrushRenderControlParameter& ControlParameter
        );
	static void RunComputeShader_RenderThread(
		FRHICommandListImmediate& RHICmdList,
		const FGPUMeshParameters& DrawParameters,
		FUnorderedAccessViewRHIRef ComputeShaderInputUAV,
		FUnorderedAccessViewRHIRef ComputeShaderOutputUAV);

	static void UpdateMeshBySDFGPU(
		FRHICommandListImmediate& RHICmdList,
		FTexture3DRHIRef SDFTexture,
		FGPUMeshVertexBuffers* VertexBuffers,
		const FGPUMeshControlParams& ControlParams
	);
};
