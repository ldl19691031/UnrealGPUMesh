// Fill out your copyright notice in the Description page of Project Settings.


#include "UGPUMeshComponent.h"
#include "Rendering/PositionVertexBuffer.h"
#include "ShaderDeclarationDemoModule.h"


class FGPUMeshSceneProxy : public FPrimitiveSceneProxy
{
public:
	SIZE_T GetTypeHash() const override
    	{
    		static size_t UniquePointer;
    		return reinterpret_cast<size_t>(&UniquePointer);
    	}

	FGPUMeshSceneProxy(UUGPUMeshComponent* Component)
		:FPrimitiveSceneProxy(Component)
		, Material(Component->RenderMaterial)
		,SdfVolumeTexture(Component->SDFTexture)
		, MaterialRelevance(Component->GetMaterialRelevance(GetScene().GetFeatureLevel()))
		, VertexFactory(GetScene().GetFeatureLevel(), "FGPUMeshSceneProxy")
	{
		VertexBuffers.Init(&VertexFactory, 40960);//InitWithDummyData(&VertexFactory, Component->NumVertics, 4);
		for (int i = 0 ; i < 10000; i++ )
		{
			IndexBuffer.Indices.Add(i * 3);
			IndexBuffer.Indices.Add(i * 3 + 1);
			IndexBuffer.Indices.Add(i * 3 + 2);
		}
		BeginInitResource(&VertexBuffers.PositionVertexBuffer);
		BeginInitResource(&VertexBuffers.StaticMeshVertexBuffer);
		BeginInitResource(&VertexBuffers.ColorVertexBuffer);
		BeginInitResource(&IndexBuffer);
		BeginInitResource(&VertexFactory);
	}
	
	virtual ~FGPUMeshSceneProxy()
	{
		VertexBuffers.PositionVertexBuffer.ReleaseResource();
		VertexBuffers.DynamicTangentXBuffer.ReleaseResource();
		VertexBuffers.DynamicTangentZBuffer.ReleaseResource();
		VertexBuffers.StaticMeshVertexBuffer.ReleaseResource();
		VertexBuffers.ColorVertexBuffer.ReleaseResource();
		IndexBuffer.ReleaseResource();
		VertexFactory.ReleaseResource();
	}
	void UpdateMesh_RenderThread()
	{
		check(IsInRenderingThread());
		if (SdfVolumeTexture == nullptr)
		{
			return;
		}
		FShaderDeclarationDemoModule::Get().UpdateGPUMesh(
			SdfVolumeTexture,
			&VertexBuffers
		);
	}

virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
	{
		const bool bWireframe = AllowDebugViewmodes() && ViewFamily.EngineShowFlags.Wireframe;

		FColoredMaterialRenderProxy* WireframeMaterialInstance = NULL;
		if (bWireframe)
		{
			WireframeMaterialInstance = new FColoredMaterialRenderProxy(
                GEngine->WireframeMaterial ? GEngine->WireframeMaterial->GetRenderProxy() : NULL,
                FLinearColor(0, 0.5f, 1.f)
                );

			Collector.RegisterOneFrameMaterialProxy(WireframeMaterialInstance);
		}
	
		FMaterialRenderProxy* MaterialProxy ;
		if (bWireframe)
		{
			MaterialProxy = WireframeMaterialInstance;
		}else if(Material != nullptr)
		{
			MaterialProxy = Material->GetRenderProxy();
		}else
		{
			MaterialProxy = UMaterial::GetDefaultMaterial(MD_Surface)->GetRenderProxy();
		}

		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			if (VisibilityMap & (1 << ViewIndex))
			{
				const FSceneView* View = Views[ViewIndex];
				// Draw the mesh.
				FMeshBatch& Mesh = Collector.AllocateMesh();
				FMeshBatchElement& BatchElement = Mesh.Elements[0];
				BatchElement.IndexBuffer = &IndexBuffer;
				Mesh.bWireframe = bWireframe;
				Mesh.VertexFactory = &VertexFactory;
				Mesh.MaterialRenderProxy = MaterialProxy;

				bool bHasPrecomputedVolumetricLightmap;
				FMatrix PreviousLocalToWorld;
				int32 SingleCaptureIndex;
				bool bOutputVelocity;
				GetScene().GetPrimitiveUniformShaderParameters_RenderThread(GetPrimitiveSceneInfo(), bHasPrecomputedVolumetricLightmap, PreviousLocalToWorld, SingleCaptureIndex, bOutputVelocity);

				FDynamicPrimitiveUniformBuffer& DynamicPrimitiveUniformBuffer = Collector.AllocateOneFrameResource<FDynamicPrimitiveUniformBuffer>();
				DynamicPrimitiveUniformBuffer.Set(GetLocalToWorld(), PreviousLocalToWorld, GetBounds(), GetLocalBounds(), true, bHasPrecomputedVolumetricLightmap, DrawsVelocity(), bOutputVelocity);
				BatchElement.PrimitiveUniformBufferResource = &DynamicPrimitiveUniformBuffer.UniformBuffer;

				BatchElement.FirstIndex = 0;
				BatchElement.NumPrimitives = IndexBuffer.Indices.Num() / 3;
				BatchElement.MinVertexIndex = 0;
				BatchElement.MaxVertexIndex = VertexBuffers.PositionVertexBuffer.GetNumVertices() - 1;
				Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
				Mesh.Type = PT_TriangleList;
				Mesh.DepthPriorityGroup = SDPG_World;
				Mesh.bCanApplyViewModeOverrides = false;
				Collector.AddMesh(ViewIndex, Mesh);
			}
		}

	}

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const
	{
		FPrimitiveViewRelevance Result;
		Result.bDrawRelevance = IsShown(View);
		Result.bShadowRelevance = IsShadowCast(View);
		Result.bDynamicRelevance = true;
		Result.bRenderInMainPass = ShouldRenderInMainPass();
		Result.bUsesLightingChannels = GetLightingChannelMask() != GetDefaultLightingChannelMask();
		Result.bRenderCustomDepth = ShouldRenderCustomDepth();
		Result.bTranslucentSelfShadow = bCastVolumetricTranslucentShadow;
		MaterialRelevance.SetPrimitiveViewRelevance(Result);
		Result.bVelocityRelevance = IsMovable() && Result.bOpaque && Result.bRenderInMainPass;
		return Result;
	}

	virtual bool CanBeOccluded() const override
	{
		return !MaterialRelevance.bDisableDepthTest;
	}

	virtual uint32 GetMemoryFootprint(void) const
	{
		return(sizeof(*this) + GetAllocatedSize());
	}

	uint32 GetAllocatedSize(void) const
	{
		return(FPrimitiveSceneProxy::GetAllocatedSize());
	}
private:
	UVolumeTexture* SdfVolumeTexture;
	
	UMaterialInterface* Material;

	FGPUMeshVertexBuffers VertexBuffers;

	FDynamicMeshIndexBuffer32 IndexBuffer;

	FLocalVertexFactory VertexFactory;

	FMaterialRelevance MaterialRelevance;
};
// Sets default values for this component's properties
UUGPUMeshComponent::UUGPUMeshComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UUGPUMeshComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UUGPUMeshComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                       FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (SceneProxy && !IsRenderStateDirty())
	{
		FGPUMeshSceneProxy* ProcMeshSceneProxy = (FGPUMeshSceneProxy*)SceneProxy;
		ENQUEUE_RENDER_COMMAND(FGPUMeshUpdate)
        ([ProcMeshSceneProxy](FRHICommandListImmediate& RHICmdList) { ProcMeshSceneProxy->UpdateMesh_RenderThread(); });

	}
}

FPrimitiveSceneProxy* UUGPUMeshComponent::CreateSceneProxy()
{
	return new FGPUMeshSceneProxy(this);
}

FBoxSphereBounds UUGPUMeshComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	return FBoxSphereBounds(
		FVector(0,0,0),
		FVector(100000, 100000,100000),
		200000
		);
}

void FGPUMeshVertexBuffers::Init(FLocalVertexFactory* VertexFactory, uint32 NumVertics)
{
	PositionVertexBuffer.Init(NumVertics);
	DynamicTangentXBuffer.Init(NumVertics, FGPUDynamicVertexBuffer::EUsage::TangentBuffer);
	DynamicTangentZBuffer.Init(NumVertics);
	StaticMeshVertexBuffer.SetUseFullPrecisionUVs(true);
	StaticMeshVertexBuffer.Init(NumVertics, 2);
	FGPUMeshVertexBuffers* Self = this;
	ENQUEUE_RENDER_COMMAND(GPUMeshDynamicBufferInit)(
		[VertexFactory, NumVertics, Self](FRHICommandListImmediate& RHICmdList)
		{
			InitOrUpdateResource(&Self->PositionVertexBuffer);
			InitOrUpdateResource(&Self->DynamicTangentXBuffer);
			//InitOrUpdateResource(&Self->DynamicTangentZBuffer);
			InitOrUpdateResource(&Self->StaticMeshVertexBuffer);
			check(Self->PositionVertexBuffer.IsInitialized());
			check(Self->StaticMeshVertexBuffer.IsInitialized());
			check(Self->DynamicTangentXBuffer.IsInitialized());
			//check(Self->DynamicTangentZBuffer.IsInitialized());
			FLocalVertexFactory::FDataType Data;
			Self->PositionVertexBuffer.BindPositionVertexBuffer(VertexFactory, Data);
			
			Self->DynamicTangentXBuffer.BindTangentVertexBuffer(VertexFactory, Data);
			//Self->DynamicTangentZBuffer.BindTangentVertexBuffer(VertexFactory, Data);
			//Self->StaticMeshVertexBuffer.BindTangentVertexBuffer(VertexFactory, Data);
			Self->StaticMeshVertexBuffer.BindPackedTexCoordVertexBuffer(VertexFactory, Data);
			Self->StaticMeshVertexBuffer.BindLightMapVertexBuffer(VertexFactory, Data, 1);
			FColorVertexBuffer::BindDefaultColorVertexBuffer(VertexFactory, Data,
			                                                 FColorVertexBuffer::NullBindStride::ZeroForDefaultBufferBind);
			VertexFactory->SetData(Data);
			VertexFactory->InitResource();
		}
	);
}

