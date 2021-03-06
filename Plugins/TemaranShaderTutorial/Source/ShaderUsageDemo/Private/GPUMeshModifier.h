#pragma once
#include "CoreMinimal.h"
#include "Actor.h"
#include "Engine/VolumeTexture.h"
#include "GPUMeshModifier.generated.h"

UCLASS(Blueprintable,BlueprintType)
class AGPUMeshModifier : public AActor
{
	GENERATED_BODY()
	AGPUMeshModifier();
public:
	UPROPERTY(Category="GPUMesh", EditAnywhere)
	TArray<FVector> MetaballCenters;
	UPROPERTY(Category="GPUMesh", EditAnywhere)
	UVolumeTexture* GPUMeshSDFTexture;
	UPROPERTY(Category="GPUMesh", EditAnywhere)
	float MetaBallRadius = 1.0f;
public:
	virtual void Tick(float DeltaSeconds) override;

	virtual  void BeginPlay() override;
};

