#include "MJMeshBuilder.h"
#include "Math/RotationMatrix.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"

int32 FMJMeshBuilder::AddSection(FName Slot, const FLinearColor& Color)
{
	FSection& Section = Sections.AddDefaulted_GetRef();
	Section.Slot = Slot;
	Section.Color = Color;
	return Sections.Num() - 1;
}

void FMJMeshBuilder::AddQuad(int32 SectionIndex, const FVector& A, const FVector& B, const FVector& C, const FVector& D, const FVector& Normal)
{
	FSection& Section = Sections[SectionIndex];
	const FVector N = Normal.GetSafeNormal();
	const FVector Tangent = (B - A).GetSafeNormal();
	const int32 Base = Section.Verts.Num();

	const FVector Corners[4] = { A, B, C, D };
	const FVector2f UVs[4] = { FVector2f(0.f, 0.f), FVector2f(1.f, 0.f), FVector2f(1.f, 1.f), FVector2f(0.f, 1.f) };
	for (int32 i = 0; i < 4; ++i)
	{
		Section.Verts.Add({ FVector3f(Corners[i]), FVector3f(N), FVector3f(Tangent), UVs[i] });
	}

	// Unreal treats a triangle as front-facing when (P1 - P0) x (P2 - P0) points away from the normal.
	const bool bFlip = FVector::DotProduct(FVector::CrossProduct(B - A, C - A), N) > 0.f;
	if (bFlip)
	{
		Section.Indices.Append({ Base + 0, Base + 2, Base + 1, Base + 0, Base + 3, Base + 2 });
	}
	else
	{
		Section.Indices.Append({ Base + 0, Base + 1, Base + 2, Base + 0, Base + 2, Base + 3 });
	}
}

void FMJMeshBuilder::AddBox(int32 SectionIndex, const FTransform& Transform, const FVector& HalfExtent)
{
	// Each face: outward normal, U axis (viewer's right), V axis (down the face for side faces).
	struct FFace { FVector N, U, V; };
	static const FFace Faces[6] =
	{
		{ FVector( 1, 0, 0), FVector( 0,-1, 0), FVector(0, 0, -1) }, // side faces: U = N x Z
		{ FVector(-1, 0, 0), FVector( 0, 1, 0), FVector(0, 0, -1) },
		{ FVector( 0, 1, 0), FVector( 1, 0, 0), FVector(0, 0, -1) },
		{ FVector( 0,-1, 0), FVector(-1, 0, 0), FVector(0, 0, -1) },
		{ FVector( 0, 0, 1), FVector( 1, 0, 0), FVector(0, 1,  0) },
		{ FVector( 0, 0,-1), FVector( 1, 0, 0), FVector(0,-1,  0) },
	};

	for (const FFace& Face : Faces)
	{
		const FVector Center = Face.N * HalfExtent;
		const FVector U = Face.U * HalfExtent;
		const FVector V = Face.V * HalfExtent;
		const FVector A = Transform.TransformPosition(Center - U - V);
		const FVector B = Transform.TransformPosition(Center + U - V);
		const FVector C = Transform.TransformPosition(Center + U + V);
		const FVector D = Transform.TransformPosition(Center - U + V);
		AddQuad(SectionIndex, A, B, C, D, Transform.TransformVectorNoScale(Face.N));
	}
}

void FMJMeshBuilder::AddBoxBetween(int32 SectionIndex, const FVector& From, const FVector& To, float HalfWidth, float HalfThickness)
{
	const FVector Dir = To - From;
	const float Length = Dir.Size();
	if (Length < KINDA_SMALL_NUMBER)
	{
		return;
	}
	const FRotator Rotation = FRotationMatrix::MakeFromXZ(Dir / Length, FVector::UpVector).Rotator();
	AddBox(SectionIndex, FTransform(Rotation, (From + To) * 0.5f), FVector(Length * 0.5f, HalfWidth, HalfThickness));
}

void FMJMeshBuilder::AddCylinder(int32 SectionIndex, const FTransform& Transform, float Radius, float HalfHeight, int32 Segments)
{
	Segments = FMath::Max(Segments, 3);
	const FVector Up = Transform.TransformVectorNoScale(FVector::UpVector);
	const FVector TopCenter = Transform.TransformPosition(FVector(0, 0, HalfHeight));
	const FVector BottomCenter = Transform.TransformPosition(FVector(0, 0, -HalfHeight));

	for (int32 i = 0; i < Segments; ++i)
	{
		const float A0 = 2.f * PI * i / Segments;
		const float A1 = 2.f * PI * (i + 1) / Segments;
		const FVector L0(FMath::Cos(A0) * Radius, FMath::Sin(A0) * Radius, 0.f);
		const FVector L1(FMath::Cos(A1) * Radius, FMath::Sin(A1) * Radius, 0.f);
		const FVector T0 = Transform.TransformPosition(L0 + FVector(0, 0, HalfHeight));
		const FVector T1 = Transform.TransformPosition(L1 + FVector(0, 0, HalfHeight));
		const FVector B0 = Transform.TransformPosition(L0 - FVector(0, 0, HalfHeight));
		const FVector B1 = Transform.TransformPosition(L1 - FVector(0, 0, HalfHeight));
		const FVector SideNormal = Transform.TransformVectorNoScale((L0 + L1).GetSafeNormal());

		AddQuad(SectionIndex, T0, T1, B1, B0, SideNormal);
		// Caps as degenerate quads (fan triangle + repeated centre keeps the builder quad-only).
		AddQuad(SectionIndex, TopCenter, T1, T0, TopCenter, Up);
		AddQuad(SectionIndex, BottomCenter, B0, B1, BottomCenter, -Up);
	}
}

void FMJMeshBuilder::BuildMeshDescription(FMeshDescription& Out) const
{
	FStaticMeshAttributes Attributes(Out);
	Attributes.Register();

	TVertexAttributesRef<FVector3f> Positions = Attributes.GetVertexPositions();
	TVertexInstanceAttributesRef<FVector3f> Normals = Attributes.GetVertexInstanceNormals();
	TVertexInstanceAttributesRef<FVector3f> Tangents = Attributes.GetVertexInstanceTangents();
	TVertexInstanceAttributesRef<float> BinormalSigns = Attributes.GetVertexInstanceBinormalSigns();
	TVertexInstanceAttributesRef<FVector4f> Colors = Attributes.GetVertexInstanceColors();
	TVertexInstanceAttributesRef<FVector2f> UVs = Attributes.GetVertexInstanceUVs();
	TPolygonGroupAttributesRef<FName> SlotNames = Attributes.GetPolygonGroupMaterialSlotNames();
	UVs.SetNumChannels(1);

	for (const FSection& Section : Sections)
	{
		const FPolygonGroupID Group = Out.CreatePolygonGroup();
		SlotNames[Group] = Section.Slot;

		TArray<FVertexInstanceID> Instances;
		Instances.Reserve(Section.Verts.Num());
		for (const FVert& Vert : Section.Verts)
		{
			const FVertexID VertexID = Out.CreateVertex();
			Positions[VertexID] = Vert.Position;
			const FVertexInstanceID Instance = Out.CreateVertexInstance(VertexID);
			Normals[Instance] = Vert.Normal;
			Tangents[Instance] = Vert.Tangent;
			BinormalSigns[Instance] = 1.f;
			Colors[Instance] = FVector4f(Section.Color);
			UVs.Set(Instance, 0, Vert.UV);
			Instances.Add(Instance);
		}

		for (int32 i = 0; i + 2 < Section.Indices.Num(); i += 3)
		{
			const int32 I0 = Section.Indices[i], I1 = Section.Indices[i + 1], I2 = Section.Indices[i + 2];
			if (I0 == I1 || I1 == I2 || I0 == I2 ||
				Section.Verts[I0].Position.Equals(Section.Verts[I1].Position) ||
				Section.Verts[I1].Position.Equals(Section.Verts[I2].Position) ||
				Section.Verts[I0].Position.Equals(Section.Verts[I2].Position))
			{
				continue; // skip degenerate cap triangles
			}
			const FVertexInstanceID Tri[3] = { Instances[I0], Instances[I1], Instances[I2] };
			Out.CreateTriangle(Group, Tri);
		}
	}
}
