#include "GPUMeshModifier.h"

#include "ShaderDeclarationDemoModule.h"

AGPUMeshModifier::AGPUMeshModifier()
	:Super()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void AGPUMeshModifier::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
    if (MetaballCenters.Num() == 0)
    {
        return;
    }

    
    FGPUMeshParameters GPUMeshParameters = FShaderDeclarationDemoModule::Get().GetGPUMeshParameters();
    GPUMeshParameters.SDFTexture = this->GPUMeshSDFTexture ;
    GPUMeshParameters.SDFTextureSize = FIntVector(
        this->GPUMeshSDFTexture->GetSizeX(),
        this->GPUMeshSDFTexture->GetSizeY(),
        this->GPUMeshSDFTexture->GetSizeZ()
        );
    GPUMeshParameters.BrushList.Empty();
    for (FVector center : MetaballCenters)
    {
        GPUMeshParameters.BrushList.Add(
            FGPUMeshParameters::FBrushParameter
            {
                center,
                MetaBallRadius
            }
        );
    }
    FShaderDeclarationDemoModule::Get().UpdateGPUMeshParameters(GPUMeshParameters);

}

void AGPUMeshModifier::BeginPlay()
{
	Super::BeginPlay();
    if (this->MetaballCenters.Num() == 0)
    {
        return;
    }
// 	this->GPUMeshSDFTexture->UpdateSourceFromFunction(
// [=](int x, int y, int z, void* output)
//     {
//         FVector pos = FVector(x,y,z);
//         bool isInside = false;
//         for (FVector center : MetaballCenters)
//         {
//             float distance = FVector::Distance(center / 100.0f,pos);
//             if ( distance < MetaBallRadius)
//             {
//                 isInside = true;
//             }
//         }
//         if(isInside)
//         {
//             *((FFloat16*)output) = FFloat16(1.0f);
//         }else
//         {
//             *((FFloat16*)output) = FFloat16(0.0f);
//         }
//     },
//     GPUMeshSDFTexture->GetSizeX(),
//     GPUMeshSDFTexture->GetSizeY(),
//     GPUMeshSDFTexture->GetSizeZ(),
//     ETextureSourceFormat::TSF_G16
//);
}
