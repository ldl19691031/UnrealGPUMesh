// Fill out your copyright notice in the Description page of Project Settings.


#include "DynamicPositionVertexBuffer.h"


FDynamicPositionVertexBuffer::FDynamicPositionVertexBuffer()
	: FVertexBufferWithSRV()
{
}

FDynamicPositionVertexBuffer::~FDynamicPositionVertexBuffer()
{
}

void FDynamicPositionVertexBuffer::CleanUp()
{
}

void FDynamicPositionVertexBuffer::Init(uint32 InNumVertices)
{
	this->NumVertices = InNumVertices;
}

void FDynamicPositionVertexBuffer::InitRHI()
{
	FVertexBufferWithSRV::InitRHI();
	if (NumVertices)
	{
		FRHIResourceCreateInfo CreateInfo;
		uint32 SizeInBytes = NumVertices * sizeof(FPositionVertex);
		this->VertexBufferRHI = RHICreateVertexBuffer(
			SizeInBytes,
			 BUF_ShaderResource | BUF_UnorderedAccess | BUF_SourceCopy | BUF_ByteAddressBuffer,
			CreateInfo
		);
		this->ShaderResourceViewRHI = RHICreateShaderResourceView(
			FShaderResourceViewInitializer(VertexBufferRHI,PF_R32_FLOAT )
		);
		this->UnorderedAccessViewRHI = RHICreateUnorderedAccessView(VertexBufferRHI, PF_R32_FLOAT);
	}
}

void FDynamicPositionVertexBuffer::ReleaseRHI()
{
	FVertexBufferWithSRV::ReleaseRHI();
}

void FDynamicPositionVertexBuffer::BindPositionVertexBuffer(const FVertexFactory* VertexFactory,
	FStaticMeshDataType& StaticMeshData)
{
	StaticMeshData.PositionComponent = FVertexStreamComponent(
        this,
        STRUCT_OFFSET(FPositionVertex, Position),
        sizeof(FPositionVertex),
        VET_Float3
    );
	StaticMeshData.PositionComponentSRV = ShaderResourceViewRHI;
}
