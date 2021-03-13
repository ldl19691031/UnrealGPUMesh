// Fill out your copyright notice in the Description page of Project Settings.


#include "GPUDynamicVertexBuffer.h"


FGPUDynamicVertexBuffer::FGPUDynamicVertexBuffer()
	: FVertexBufferWithSRV()
{
}

FGPUDynamicVertexBuffer::~FGPUDynamicVertexBuffer()
{
}

void FGPUDynamicVertexBuffer::CleanUp()
{
}

void FGPUDynamicVertexBuffer::Init(uint32 InNumVertices, EUsage usage)
{
	this->NumVertices = InNumVertices;
	this->Usage = usage;
}



void FGPUDynamicVertexBuffer::InitRHI()
{
	FVertexBufferWithSRV::InitRHI();
	typedef TStaticMeshVertexTangentDatum<typename TStaticMeshVertexTangentTypeSelector<EStaticMeshVertexTangentBasisType::Default>::TangentTypeT> TangentType;
	if (NumVertices)
	{
		FRHIResourceCreateInfo CreateInfo;
		uint32 SizeInBytes;
		switch (Usage)
		{
			case EUsage::None:
				SizeInBytes = 0; break;
			case EUsage::PositionBuffer:
				SizeInBytes = NumVertices * sizeof(FPositionVertex); break;
			case EUsage::TangentBuffer:
				SizeInBytes = NumVertices * sizeof(TangentType); break;
		}

		this->VertexBufferRHI = RHICreateVertexBuffer(
			SizeInBytes,
			 BUF_ShaderResource | BUF_UnorderedAccess ,
			CreateInfo
		);
		if(Usage == EUsage::TangentBuffer)
		{
			this->ShaderResourceViewRHI = RHICreateShaderResourceView(
                FShaderResourceViewInitializer(VertexBufferRHI,PF_R8G8B8A8_SNORM )
            );
		}
		else
		{
			this->ShaderResourceViewRHI = RHICreateShaderResourceView(
				FShaderResourceViewInitializer(VertexBufferRHI,PF_R32_FLOAT )
			);
		}
		if(Usage == EUsage::TangentBuffer)
		{
			this->UnorderedAccessViewRHI = RHICreateUnorderedAccessView(VertexBufferRHI, PF_R8G8B8A8_SNORM);
		}
		else
		{
			this->UnorderedAccessViewRHI = RHICreateUnorderedAccessView(VertexBufferRHI, PF_R32_FLOAT);
		}
		
	}
}

void FGPUDynamicVertexBuffer::ReleaseRHI()
{
	FVertexBufferWithSRV::ReleaseRHI();
}

void FGPUDynamicVertexBuffer::BindPositionVertexBuffer(const FVertexFactory* VertexFactory,
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
struct FCustomTangentType
{
	FVector4 TangentX;
	FVector4 TangentZ;
};
void FGPUDynamicVertexBuffer::BindTangentVertexBuffer(const FVertexFactory* VertexFactory,
    FStaticMeshDataType& StaticMeshData)
{
	typedef TStaticMeshVertexTangentDatum<typename TStaticMeshVertexTangentTypeSelector<EStaticMeshVertexTangentBasisType::Default>::TangentTypeT> TangentType;
	{
		StaticMeshData.TangentsSRV = this->ShaderResourceViewRHI;
	}
	uint32 TangentSizeInBytes = 0;
	uint32 TangentXOffset = 0;
	uint32 TangentZOffset = 0;
	EVertexElementType TangentElemType = VET_None;
	TangentElemType = TStaticMeshVertexTangentTypeSelector<EStaticMeshVertexTangentBasisType::Default>::VertexElementType;
	TangentXOffset = STRUCT_OFFSET(FCustomTangentType, TangentX);
	TangentZOffset = STRUCT_OFFSET(FCustomTangentType, TangentZ);
	TangentSizeInBytes = sizeof(FCustomTangentType);
	StaticMeshData.TangentBasisComponents[0] = FVertexStreamComponent(
            this,
            TangentXOffset,
            TangentSizeInBytes,
            TangentElemType,
            EVertexStreamUsage::ManualFetch
    );

	StaticMeshData.TangentBasisComponents[1] = FVertexStreamComponent(
        this,
        TangentZOffset,
        TangentSizeInBytes,
        VET_Float4,
        EVertexStreamUsage::ManualFetch
    );
}

