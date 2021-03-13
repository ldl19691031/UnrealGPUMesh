// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RenderResource.h"
/**
 * 
 */
class  FGPUDynamicVertexBuffer : public FVertexBufferWithSRV

{
	
public:
	enum class EUsage
	{
		None,
         PositionBuffer,
         TangentBuffer
    } Usage;
	
	 FGPUDynamicVertexBuffer();
	 ~FGPUDynamicVertexBuffer();

	 void CleanUp();

	 void Init(uint32 NumVertices, EUsage usage = EUsage::PositionBuffer);

	 uint32 GetNumVertices() const {	return NumVertices;	}

	 virtual void InitRHI() override;

	 virtual void ReleaseRHI() override;

	 void BindPositionVertexBuffer(const FVertexFactory* VertexFactory, FStaticMeshDataType& StaticMeshData);
	 void BindTangentVertexBuffer(const FVertexFactory* VertexFactory, FStaticMeshDataType& StaticMeshData);
 private:
	uint32 NumVertices;

	 
};
