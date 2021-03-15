// Fill out your copyright notice in the Description page of Project Settings.


#include "GPUMeshComponent.h"

#include "GPUMeshBrush.h"
#include "GPUMeshShader.h"
#include "Rendering/PositionVertexBuffer.h"
#include "ShaderDeclarationDemoModule.h"
#include "Engine/VolumeTexture.h"

class FGPUMeshSceneProxy : public FPrimitiveSceneProxy
{
public:
	SIZE_T GetTypeHash() const override
    	{
    		static size_t UniquePointer;
    		return reinterpret_cast<size_t>(&UniquePointer);
    	}

	void InitSDFTexture(UGPUMeshComponent* Component)
	{
		FGPUMeshSceneProxy* Self = this;
		UVolumeTexture* SDFTextureVolume = Component->SDFTexture;
		if (IsInRenderingThread())
		{
			FRHIResourceCreateInfo CreateInfo;
			Self->SDFTexture =  RHICreateTexture3D(
                SDFTextureVolume->GetSizeX(),
                SDFTextureVolume->GetSizeY(),
                SDFTextureVolume->GetSizeZ(),
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
				copyInfo.NumSlices = SDFTextureVolume->GetSizeZ();
			}
			GetImmediateCommandList_ForRenderCommand().CopyTexture(SDFTextureVolume->Resource->TextureRHI, Self->SDFTexture, copyInfo);
			Self->SDFTextureUAV = RHICreateUnorderedAccessView(Self->SDFTexture);
		}else
		{
			ENQUEUE_RENDER_COMMAND(InitVolumeTextureFromVolumeTexture)
	        (
	            [Self, SDFTextureVolume](FRHICommandList& RHICmdList)
	            {
	                FRHIResourceCreateInfo CreateInfo;
	                Self->SDFTexture =  RHICreateTexture3D(
	                    SDFTextureVolume->GetSizeX(),
	                    SDFTextureVolume->GetSizeY(),
	                    SDFTextureVolume->GetSizeZ(),
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
	                    copyInfo.NumSlices = SDFTextureVolume->GetSizeZ();
	                }
	                RHICmdList.CopyTexture(SDFTextureVolume->Resource->TextureRHI, Self->SDFTexture, copyInfo);
	                Self->SDFTextureUAV = RHICreateUnorderedAccessView(Self->SDFTexture);
	            	Self->OwnerComponent->SDFTextureUAV = Self->SDFTextureUAV;
	            	
	            }
	        );
		}
		
	}

	FGPUMeshSceneProxy(UGPUMeshComponent* Component)
		:FPrimitiveSceneProxy(Component)
		, Material(Component->RenderMaterial)
		,SdfVolumeTexture(Component->SDFTexture)
		, MaterialRelevance(Component->GetMaterialRelevance(GetScene().GetFeatureLevel()))
		, VertexFactory(GetScene().GetFeatureLevel(), "FGPUMeshSceneProxy")
		, OwnerComponent(Component)
	{
		VertexBuffers.Init(&VertexFactory, Component->NumVertics);//InitWithDummyData(&VertexFactory, Component->NumVertics, 4);
		for (int i = 0 ; i < Component->NumVertics; i++ )
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
		if (Component->SDFTexture != nullptr)
		{
			InitSDFTexture(Component);
		}
		if (BrushBuffer.IsValid() == false)
		{
			FGPUMeshSceneProxy* Self = this;
			ENQUEUE_RENDER_COMMAND(InitBrushBuffers)
			(
				[Self](FRHICommandList& RHIcmd)
				{
					FGPUMeshShader::InitBrushBuffer(
		                Self->BrushBuffer,
		                Self->BrushBufferShaderResourceView
		            );
				}
			);
		}
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
		if (SDFTexture.IsValid())
		{
			SDFTexture.SafeRelease();
		}
		if (SDFTextureUAV.IsValid())
		{
			SDFTextureUAV.SafeRelease();
		}
		BrushBuffer.SafeRelease();
		BrushBufferShaderResourceView.SafeRelease();
	}
	void UpdateMesh_RenderThread()
	{
		check(IsInRenderingThread());
		if (SDFTexture.IsValid() == false || SDFTextureUAV.IsValid() == false)
		{
			if (OwnerComponent->SDFTexture != nullptr)
			{
				InitSDFTexture(OwnerComponent);
			}
			else
			{
				return;
			}
		}
		FShaderDeclarationDemoModule::Get().UpdateGPUMesh(
			SDFTexture,
			FGPUMeshControlParams{
                OwnerComponent->VertexOffset,
                OwnerComponent->VertexScale
            },
			&VertexBuffers
		);
	}

	void UpdateSDFTexture_RenderThread(FRHICommandListImmediate& RHICmdList, const TArray<FGPUMeshParameters::FBrushParameter>& Brushes)
	{
		check(IsInRenderingThread());
		if (SDFTexture.IsValid() == false || SDFTextureUAV.IsValid() == false)
		{
			if (OwnerComponent->SDFTexture != nullptr)
			{
				InitSDFTexture(OwnerComponent);
			}
			else
			{
				return;
			}
			
		}
		
		FGPUMeshShader::UpdateSdfTextureByBrushGPU(
			RHICmdList,
			Brushes,
			BrushBuffer,
			BrushBufferShaderResourceView,
			SDFTexture,
			SDFTextureUAV,
			FGPUMeshShader::FSDFBrushRenderControlParameter
			{
				OwnerComponent->Smoothness	
			}
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

	FTexture3DRHIRef SDFTexture;

	FUnorderedAccessViewRHIRef SDFTextureUAV;

	FStructuredBufferRHIRef BrushBuffer;

	FShaderResourceViewRHIRef BrushBufferShaderResourceView;

	UGPUMeshComponent* OwnerComponent;
};
// Sets default values for this component's properties
UGPUMeshComponent::UGPUMeshComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	bTickInEditor = true;
	bAutoActivate = true;
	// ...
}


// Called when the game starts
void UGPUMeshComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UGPUMeshComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                       FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (SceneProxy && !IsRenderStateDirty())
	{
		FGPUMeshSceneProxy* ProcMeshSceneProxy = (FGPUMeshSceneProxy*)SceneProxy;
		TArray<FGPUMeshParameters::FBrushParameter> BrushParameters;
		for (UGPUMeshBrush* brush : MeshBrushes)
		{
			FGPUMeshParameters::FBrushParameter parameter = brush->GetGPUMeshParameter();
			//parameter.center /= scale;
			FVector scale = GetOwner()->GetActorScale();
			parameter.center = parameter.center * (1.0f / 8.0f) + 64.0f;
			//parameter.center = (parameter.center - scale * 0.5f) / (scale / FVector(SDFTexture->GetSizeX(), SDFTexture->GetSizeY(), SDFTexture->GetSizeZ()));
			BrushParameters.Add(parameter);
		}
		ENQUEUE_RENDER_COMMAND(FGPUMeshUpdate)
        ([ProcMeshSceneProxy,BrushParameters](FRHICommandListImmediate& RHICmdList)
        {
	        if (BrushParameters.Num() > 0)
	        {
	        	ProcMeshSceneProxy->UpdateSDFTexture_RenderThread(RHICmdList,BrushParameters);
	        }
	        ProcMeshSceneProxy->UpdateMesh_RenderThread();
        });

	}
}

FPrimitiveSceneProxy* UGPUMeshComponent::CreateSceneProxy()
{
	return new FGPUMeshSceneProxy(this);
}

FBoxSphereBounds UGPUMeshComponent::CalcBounds(const FTransform& LocalToWorld) const
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

