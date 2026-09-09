// Copyright zutemiss & dshadykh. Educational project.

#include "Core/FRVisualUtils.h"

#include "Components/MeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

#include "FiringRange.h"

namespace
{
	/** Asset path of every primitive used by the project. */
	const TCHAR* GetShapePath(EFRBasicShape Shape)
	{
		switch (Shape)
		{
		case EFRBasicShape::Sphere:   return TEXT("/Engine/BasicShapes/Sphere.Sphere");
		case EFRBasicShape::Cylinder: return TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
		case EFRBasicShape::Cone:     return TEXT("/Engine/BasicShapes/Cone.Cone");
		case EFRBasicShape::Plane:    return TEXT("/Engine/BasicShapes/Plane.Plane");
		case EFRBasicShape::Cube:
		default:                      return TEXT("/Engine/BasicShapes/Cube.Cube");
		}
	}

	/** Loaded meshes are kept alive for the lifetime of the module. */
	TMap<EFRBasicShape, TObjectPtr<UStaticMesh>>& GetMeshCache()
	{
		static TMap<EFRBasicShape, TObjectPtr<UStaticMesh>> Cache;
		return Cache;
	}

	TObjectPtr<UMaterialInterface>& GetMaterialCache()
	{
		static TObjectPtr<UMaterialInterface> Cache = nullptr;
		return Cache;
	}
}

UStaticMesh* FRVisual::GetBasicMesh(EFRBasicShape Shape)
{
	TMap<EFRBasicShape, TObjectPtr<UStaticMesh>>& Cache = GetMeshCache();

	if (const TObjectPtr<UStaticMesh>* Found = Cache.Find(Shape))
	{
		if (*Found)
		{
			return *Found;
		}
	}

	UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, GetShapePath(Shape));
	if (!Mesh)
	{
		UE_LOG(LogFiringRange, Error, TEXT("Failed to load engine primitive %s."), GetShapePath(Shape));
		return nullptr;
	}

	// Engine content is garbage collected like any other object. Adding the mesh
	// to the root set keeps the cache valid for the whole session.
	Mesh->AddToRoot();
	Cache.Add(Shape, Mesh);
	return Mesh;
}

UMaterialInterface* FRVisual::GetBasicMaterial()
{
	TObjectPtr<UMaterialInterface>& Cache = GetMaterialCache();
	if (Cache)
	{
		return Cache;
	}

	UMaterialInterface* Material =
		LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	if (!Material)
	{
		UE_LOG(LogFiringRange, Error, TEXT("Failed to load BasicShapeMaterial."));
		return nullptr;
	}

	Material->AddToRoot();
	Cache = Material;
	return Material;
}

UMaterialInstanceDynamic* FRVisual::SetMeshColor(UMeshComponent* MeshComponent, const FLinearColor& Color)
{
	if (!MeshComponent)
	{
		return nullptr;
	}

	UMaterialInterface* BaseMaterial = GetBasicMaterial();
	if (!BaseMaterial)
	{
		return nullptr;
	}

	UMaterialInstanceDynamic* Instance = UMaterialInstanceDynamic::Create(BaseMaterial, MeshComponent);
	if (!Instance)
	{
		return nullptr;
	}

	// BasicShapeMaterial exposes a single vector parameter named "Color".
	Instance->SetVectorParameterValue(TEXT("Color"), Color);
	MeshComponent->SetMaterial(0, Instance);
	return Instance;
}

UMaterialInstanceDynamic* FRVisual::BuildMesh(
	UStaticMeshComponent* MeshComponent,
	EFRBasicShape Shape,
	const FLinearColor& Color,
	bool bCollides)
{
	if (!MeshComponent)
	{
		return nullptr;
	}

	if (UStaticMesh* Mesh = GetBasicMesh(Shape))
	{
		MeshComponent->SetStaticMesh(Mesh);
	}

	if (bCollides)
	{
		// BlockAllDynamic rather than BlockAll. Both stop everything, but BlockAll
		// declares the body as world static, and every collider in this project is
		// created at runtime, with the target boards actually moving. A static body
		// that moves is a contradiction the physics scene has to work around on
		// every frame.
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		MeshComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	}
	else
	{
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	return SetMeshColor(MeshComponent, Color);
}

FVector FRVisual::SizeToScale(const FVector& SizeInCentimetres)
{
	// Every primitive under /Engine/BasicShapes measures 100 units on each axis,
	// so dividing by 100 turns a size in centimetres into a component scale.
	return SizeInCentimetres / 100.0f;
}
