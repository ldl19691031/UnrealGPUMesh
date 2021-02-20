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
	static void RunComputeShader_RenderThread(
		FRHICommandListImmediate& RHICmdList,
		const FGPUMeshParameters& DrawParameters,
		FUnorderedAccessViewRHIRef ComputeShaderInputUAV,
		FUnorderedAccessViewRHIRef ComputeShaderOutputUAV);
};