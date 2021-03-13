// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RenderResource.h"
/**
 * 
 */
class  FDynamicPositionVertexBuffer : public FVertexBufferWithSRV
{
public:
	 FDynamicPositionVertexBuffer();
	 ~FDynamicPositionVertexBuffer();

	 void CleanUp();

	 void Init(uint32 NumVertices);

	 uint32 GetNumVertices() const {	return NumVertices;	}

	 virtual void InitRHI() override;

	 virtual void ReleaseRHI() override;

	 void BindPositionVertexBuffer(const FVertexFactory* VertexFactory, FStaticMeshDataType& StaticMeshData);
private:
	uint32 NumVertices;	
};
