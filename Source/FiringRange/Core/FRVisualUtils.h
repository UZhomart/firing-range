// Copyright zutemiss & dshadykh. Educational project.

#pragma once

#include "CoreMinimal.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;

/** Primitive shapes shipped with the engine under /Engine/BasicShapes. */
enum class EFRBasicShape : uint8
{
	Cube,
	Sphere,
	Cylinder,
	Cone,
	Plane
};

/**
 * Helpers that build the look of the project out of engine content only.
 *
 * The whole game is authored in C++ and ships without a single imported asset,
 * so every mesh comes from /Engine/BasicShapes and every surface is a dynamic
 * instance of the engine's BasicShapeMaterial tinted at runtime. Loading is done
 * lazily and cached, never from a constructor, because constructors run while
 * the class default object is being created and asset loading there is unsafe.
 */
namespace FRVisual
{
	/** Returns the engine primitive for a shape, or nullptr when it cannot be loaded. */
	UStaticMesh* GetBasicMesh(EFRBasicShape Shape);

	/** Returns the engine material that exposes a "Color" vector parameter. */
	UMaterialInterface* GetBasicMaterial();

	/**
	 * Creates a dynamic material instance on the component and tints it.
	 *
	 * Returns the instance so callers can keep animating the colour, which is how
	 * targets flash when they are hit.
	 */
	UMaterialInstanceDynamic* SetMeshColor(UMeshComponent* MeshComponent, const FLinearColor& Color);

	/**
	 * One call setup for a piece of scenery: assigns the mesh, tints it, and
	 * decides whether the surface should take part in collision at all.
	 */
	UMaterialInstanceDynamic* BuildMesh(
		UStaticMeshComponent* MeshComponent,
		EFRBasicShape Shape,
		const FLinearColor& Color,
		bool bCollides = true);

	/**
	 * Engine primitives are 100 units wide, so a component scale of one equals a
	 * one metre box. This converts a desired size in centimetres into that scale.
	 */
	FVector SizeToScale(const FVector& SizeInCentimetres);
}
