#pragma once

#include "CoreMinimal.h"

struct FMeshDescription;

/**
 * Tiny triangle-soup builder for the stylised blockout art (boxes + cylinders, one section per material slot).
 * Output is an FMeshDescription so the same geometry can become a transient runtime mesh or a saved asset.
 */
struct MASTERJUMPER_API FMJMeshBuilder
{
	struct FVert
	{
		FVector3f Position;
		FVector3f Normal;
		FVector3f Tangent;
		FVector2f UV;
	};

	struct FSection
	{
		FName Slot;
		FLinearColor Color;
		TArray<FVert> Verts;
		TArray<int32> Indices;
	};

	TArray<FSection> Sections;

	int32 AddSection(FName Slot, const FLinearColor& Color);

	/** Quad A-B-C-D (perimeter order, UVs 0,0 / 1,0 / 1,1 / 0,1). Winding is fixed up to face Normal. */
	void AddQuad(int32 Section, const FVector& A, const FVector& B, const FVector& C, const FVector& D, const FVector& Normal);

	/** Oriented box. Side faces get V running top->bottom so textures stay upright. */
	void AddBox(int32 Section, const FTransform& Transform, const FVector& HalfExtent);

	/** Box whose local X runs from From to To (local Z kept as close to world up as possible). */
	void AddBoxBetween(int32 Section, const FVector& From, const FVector& To, float HalfWidth, float HalfThickness);

	/** Capped cylinder along local Z. */
	void AddCylinder(int32 Section, const FTransform& Transform, float Radius, float HalfHeight, int32 Segments = 12);

	void BuildMeshDescription(FMeshDescription& Out) const;
};
