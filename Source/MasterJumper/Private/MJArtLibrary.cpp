#include "MJArtLibrary.h"
#include "MJTypes.h"
#include "MJLayout.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"

#if WITH_EDITOR
#include "Materials/MaterialInstanceConstant.h"
#include "AssetRegistry/AssetRegistryModule.h"
#endif

namespace
{
	const FName ColorParam(TEXT("Color"));

	FLinearColor Lin(float R, float G, float B) { return FLinearColor(R, G, B, 1.f); }

	void BuildChair(FMJMeshBuilder& B)
	{
		// Origin: floor centre. Seat faces +X, backrest towards -X. ~60 x 60 x 114 cm.
		const int32 Fabric = B.AddSection(TEXT("Fabric"), Lin(0.020f, 0.026f, 0.055f));
		const int32 Frame = B.AddSection(TEXT("Frame"), Lin(0.012f, 0.012f, 0.014f));
		const int32 Chrome = B.AddSection(TEXT("Chrome"), Lin(0.45f, 0.45f, 0.47f));

		for (int32 i = 0; i < 5; ++i)
		{
			const float A = FMath::DegreesToRadians(90.f + i * 72.f);
			const FVector Tip(FMath::Cos(A) * 30.f, FMath::Sin(A) * 30.f, 7.f);
			B.AddBoxBetween(Frame, FVector(0, 0, 10), Tip, 3.f, 2.f);
			B.AddCylinder(Frame, FTransform(FRotator(0.f, FMath::RadiansToDegrees(A), 90.f), Tip - FVector(0, 0, 3.5f)), 3.5f, 2.f, 8);
		}
		B.AddCylinder(Frame, FTransform(FVector(0, 0, 10)), 5.f, 4.f, 10);
		B.AddCylinder(Chrome, FTransform(FVector(0, 0, 28)), 2.5f, 16.f, 10);
		B.AddBox(Frame, FTransform(FVector(0, 0, 45.5f)), FVector(11, 11, 2.5f));

		B.AddBox(Fabric, FTransform(FVector(1, 0, 52)), FVector(25, 24, 5));
		B.AddBoxBetween(Frame, FVector(-12, 0, 47), FVector(-27, 0, 72), 3.f, 1.5f);
		B.AddBox(Fabric, FTransform(FRotator(-8.f, 0.f, 0.f), FVector(-29, 0, 92)), FVector(4, 22, 22));

		for (const float Side : { -1.f, 1.f })
		{
			B.AddBox(Frame, FTransform(FVector(-2, Side * 23.f, 62)), FVector(2, 1.5f, 8));
			B.AddBox(Frame, FTransform(FVector(1, Side * 23.f, 71)), FVector(12, 3, 1.8f));
		}
	}

	void BuildComputer(FMJMeshBuilder& B)
	{
		// Origin: floor centre. Beige desktop case + CRT monitor + keyboard, screen faces +X. ~68 x 46 x 54 cm.
		const int32 Plastic = B.AddSection(TEXT("Plastic"), Lin(0.50f, 0.44f, 0.33f));
		const int32 Screen = B.AddSection(TEXT("Screen"), Lin(0.004f, 0.014f, 0.020f));
		const int32 Detail = B.AddSection(TEXT("Detail"), Lin(0.03f, 0.03f, 0.03f));

		B.AddBox(Plastic, FTransform(FVector(0, 0, 8)), FVector(22, 21, 8));
		B.AddBox(Detail, FTransform(FVector(22.3f, -8, 11)), FVector(0.5f, 7, 0.9f));
		B.AddBox(Detail, FTransform(FVector(22.3f, -8, 6)), FVector(0.5f, 7, 0.9f));
		B.AddBox(Detail, FTransform(FVector(22.3f, 14, 5)), FVector(0.5f, 1.2f, 1.2f));

		B.AddBox(Detail, FTransform(FVector(0, 0, 17)), FVector(9, 9, 1));
		B.AddBox(Plastic, FTransform(FVector(-2, 0, 35.5f)), FVector(18, 19, 17));
		B.AddBox(Plastic, FTransform(FVector(-24, 0, 34)), FVector(9, 13, 12));
		B.AddBox(Screen, FTransform(FVector(16.3f, 0, 36.5f)), FVector(0.5f, 15.5f, 12.5f));

		B.AddBox(Plastic, FTransform(FRotator(4.f, 0.f, 0.f), FVector(34, 0, 1.6f)), FVector(8, 22, 1.6f));
		B.AddBox(Detail, FTransform(FRotator(4.f, 0.f, 0.f), FVector(34, 0, 3.3f)), FVector(6.5f, 19.5f, 0.4f));
	}

	void BuildPovArms(FMJMeshBuilder& B)
	{
		// Camera space (X forward, Y right, Z up). Superman dive: both arms straight ahead, hands converging below centre.
		const int32 Sleeve = B.AddSection(TEXT("Sleeve"), Lin(0.25f, 0.42f, 0.70f));
		const int32 Cuff = B.AddSection(TEXT("Cuff"), Lin(0.80f, 0.80f, 0.80f));
		const int32 Skin = B.AddSection(TEXT("Skin"), Lin(0.70f, 0.42f, 0.30f));

		for (const float Side : { -1.f, 1.f })
		{
			const FVector Shoulder(6.f, Side * 25.f, -36.f);
			const FVector Wrist(62.f, Side * 10.f, -13.f);
			const FVector Dir = (Wrist - Shoulder).GetSafeNormal();
			const FVector Inward(0.f, -Side, 0.f);

			B.AddBoxBetween(Sleeve, Shoulder, Wrist - Dir * 4.f, 6.5f, 6.f);
			B.AddBoxBetween(Cuff, Wrist - Dir * 5.f, Wrist, 6.9f, 6.4f);
			B.AddBoxBetween(Skin, Wrist, Wrist + Dir * 10.f, 4.6f, 1.9f);                // palm (flat, palm down)
			B.AddBoxBetween(Skin, Wrist + Dir * 10.f, Wrist + Dir * 18.f, 4.3f, 1.3f);   // fingers together
			B.AddBoxBetween(Skin, Wrist + Dir * 3.f + Inward * 4.f, Wrist + Dir * 11.f + Inward * 6.f, 1.4f, 1.3f); // thumb
		}
	}

	void BuildWallPanel(FMJMeshBuilder& B)
	{
		// One 400 cm x ceiling-height panel, origin at floor centre; both broad faces carry the full texture (V top->bottom).
		const int32 Wall = B.AddSection(TEXT("Wall"), Lin(0.62f, 0.60f, 0.54f));
		B.AddBox(Wall, FTransform(FVector(0, 0, MJLayout::CeilingZ * 0.5f)), FVector(200.f, MJLayout::WallThickness * 0.5f, MJLayout::CeilingZ * 0.5f));
	}

	TMap<EMJArt, TWeakObjectPtr<UStaticMesh>> GTransientMeshes;

#if WITH_EDITOR
	UMaterialInterface* CreateOrLoadColorInstance(const FString& Folder, const FString& Name, UMaterialInterface* Parent, const FLinearColor& Color)
	{
		const FString PackageName = Folder / Name;
		if (UMaterialInterface* Existing = UMJArtLibrary::LoadOptional<UMaterialInterface>(PackageName))
		{
			return Existing;
		}
		UPackage* Package = CreatePackage(*PackageName);
		UMaterialInstanceConstant* Instance = NewObject<UMaterialInstanceConstant>(Package, *Name, RF_Public | RF_Standalone | RF_Transactional);
		Instance->SetParentEditorOnly(Parent);
		Instance->SetVectorParameterValueEditorOnly(FMaterialParameterInfo(ColorParam), Color);
		Instance->PostEditChange();
		Instance->MarkPackageDirty();
		FAssetRegistryModule::AssetCreated(Instance);
		return Instance;
	}

	bool BakeOne(EMJArt Art, const FString& MeshFolder, const FString& MaterialFolder)
	{
		const FString AssetName = UMJArtLibrary::GetMeshAssetName(Art);
		const FString PackageName = MeshFolder / AssetName;
		if (UMJArtLibrary::LoadOptional<UStaticMesh>(PackageName))
		{
			UE_LOG(LogMasterJumper, Display, TEXT("Bake: %s already exists, skipping (delete it to rebake)."), *PackageName);
			return true;
		}

		const FMJMeshBuilder Builder = UMJArtLibrary::BuildArt(Art);
		UMaterialInterface* Parent = UMJArtLibrary::LoadOptional<UMaterialInterface>(MaterialFolder / UMJArtLibrary::GetParentMaterialName(Art));
		if (!Parent)
		{
			UE_LOG(LogMasterJumper, Warning, TEXT("Bake: parent material %s missing, using BasicShapeMaterial."), *UMJArtLibrary::GetParentMaterialName(Art));
			Parent = UMJArtLibrary::GetBasicShapeMaterial();
		}

		TArray<FStaticMaterial> StaticMaterials;
		const FString ShortName = AssetName.RightChop(3); // drop "SM_"
		for (const FMJMeshBuilder::FSection& Section : Builder.Sections)
		{
			UMaterialInterface* SlotMaterial = (Art == EMJArt::CorridorWallPanel)
				? Parent
				: CreateOrLoadColorInstance(MaterialFolder, FString::Printf(TEXT("MI_%s_%s"), *ShortName, *Section.Slot.ToString()), Parent, Section.Color);
			StaticMaterials.Add(FStaticMaterial(SlotMaterial, Section.Slot, Section.Slot));
		}

		UPackage* Package = CreatePackage(*PackageName);
		UStaticMesh* Mesh = NewObject<UStaticMesh>(Package, *AssetName, RF_Public | RF_Standalone | RF_Transactional);
		Mesh->InitResources();
		Mesh->SetLightingGuid();

		FStaticMeshSourceModel& Source = Mesh->AddSourceModel();
		Source.BuildSettings.bRecomputeNormals = false;
		Source.BuildSettings.bRecomputeTangents = false;
		Source.BuildSettings.bRemoveDegenerates = true;
		Source.BuildSettings.bGenerateLightmapUVs = true;
		Source.BuildSettings.SrcLightmapIndex = 0;
		Source.BuildSettings.DstLightmapIndex = 1;

		FMeshDescription Description;
		Builder.BuildMeshDescription(Description);
		Mesh->CreateMeshDescription(0, MoveTemp(Description));
		Mesh->CommitMeshDescription(0);

		Mesh->SetStaticMaterials(StaticMaterials);
		Mesh->SetLightMapCoordinateIndex(1);
		Mesh->Build(false);
		Mesh->PostEditChange();
		Mesh->MarkPackageDirty();
		FAssetRegistryModule::AssetCreated(Mesh);

		UE_LOG(LogMasterJumper, Display, TEXT("Bake: created %s (%d material slots)."), *PackageName, StaticMaterials.Num());
		return true;
	}
#endif
}

FMJMeshBuilder UMJArtLibrary::BuildArt(EMJArt Art)
{
	FMJMeshBuilder Builder;
	switch (Art)
	{
	case EMJArt::OfficeChair:       BuildChair(Builder); break;
	case EMJArt::OfficeComputer:    BuildComputer(Builder); break;
	case EMJArt::PovArms:           BuildPovArms(Builder); break;
	case EMJArt::CorridorWallPanel: BuildWallPanel(Builder); break;
	}
	return Builder;
}

FString UMJArtLibrary::GetMeshAssetName(EMJArt Art)
{
	switch (Art)
	{
	case EMJArt::OfficeChair:       return TEXT("SM_OfficeChair");
	case EMJArt::OfficeComputer:    return TEXT("SM_OfficeComputer");
	case EMJArt::PovArms:           return TEXT("SM_POV_Arms");
	case EMJArt::CorridorWallPanel: return TEXT("SM_CorridorWallPanel");
	}
	return FString();
}

FString UMJArtLibrary::GetParentMaterialName(EMJArt Art)
{
	switch (Art)
	{
	case EMJArt::OfficeChair:       return TEXT("M_OfficeChair");
	case EMJArt::OfficeComputer:    return TEXT("M_OfficePC");
	case EMJArt::PovArms:           return TEXT("M_POV_Arms");
	case EMJArt::CorridorWallPanel: return TEXT("M_CorridorWall");
	}
	return FString();
}

UStaticMesh* UMJArtLibrary::GetEngineCube()
{
	return LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
}

UMaterialInterface* UMJArtLibrary::GetBasicShapeMaterial()
{
	return LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
}

UStaticMesh* UMJArtLibrary::ResolveMesh(EMJArt Art, bool bAllowTransient)
{
	if (UStaticMesh* Baked = LoadOptional<UStaticMesh>(MJAssets::Meshes / GetMeshAssetName(Art)))
	{
		return Baked;
	}
	if (!bAllowTransient)
	{
		return nullptr;
	}
	if (UStaticMesh* Cached = GTransientMeshes.FindRef(Art).Get())
	{
		return Cached;
	}

	const FMJMeshBuilder Builder = BuildArt(Art);
	const FName MeshName = MakeUniqueObjectName(GetTransientPackage(), UStaticMesh::StaticClass(), FName(*(GetMeshAssetName(Art) + TEXT("_Runtime"))));
	UStaticMesh* Mesh = NewObject<UStaticMesh>(GetTransientPackage(), MeshName, RF_Transient);

	TArray<FStaticMaterial> StaticMaterials;
	for (const FMJMeshBuilder::FSection& Section : Builder.Sections)
	{
		StaticMaterials.Add(FStaticMaterial(nullptr, Section.Slot, Section.Slot));
	}
	Mesh->SetStaticMaterials(StaticMaterials);

	FMeshDescription Description;
	Builder.BuildMeshDescription(Description);
	UStaticMesh::FBuildMeshDescriptionsParams Params;
	Params.bBuildSimpleCollision = false;
	Params.bCommitMeshDescription = false;
	Params.bFastBuild = true;
	const TArray<const FMeshDescription*> Descriptions = { &Description };
	Mesh->BuildFromMeshDescriptions(Descriptions, Params);

	GTransientMeshes.Add(Art, Mesh);
	return Mesh;
}

void UMJArtLibrary::ApplyArtMaterials(UStaticMeshComponent* Component, EMJArt Art)
{
	if (!Component || !Component->GetStaticMesh())
	{
		return;
	}
	Component->EmptyOverrideMaterials();
	if (!Component->GetStaticMesh()->HasAnyFlags(RF_Transient))
	{
		return; // baked asset: slots already hold the MI_* colour instances
	}

	UMaterialInterface* Parent = LoadOptional<UMaterialInterface>(MJAssets::Materials / GetParentMaterialName(Art));
	if (!Parent)
	{
		Parent = GetBasicShapeMaterial();
	}
	const FMJMeshBuilder Builder = BuildArt(Art);
	for (int32 Slot = 0; Slot < Builder.Sections.Num(); ++Slot)
	{
		Component->SetMaterial(Slot, MakeSolidMaterial(Component, Builder.Sections[Slot].Color, Parent));
	}
}

UMaterialInstanceDynamic* UMJArtLibrary::MakeSolidMaterial(UObject* Outer, const FLinearColor& Color, UMaterialInterface* Parent)
{
	UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Parent ? Parent : GetBasicShapeMaterial(), Outer);
	if (Material)
	{
		Material->SetVectorParameterValue(ColorParam, Color);
	}
	return Material;
}

bool UMJArtLibrary::BakeArtAssets(const FString& MeshFolder, const FString& MaterialFolder)
{
#if WITH_EDITOR
	bool bAllOk = true;
	for (const EMJArt Art : { EMJArt::OfficeChair, EMJArt::OfficeComputer, EMJArt::PovArms, EMJArt::CorridorWallPanel })
	{
		bAllOk &= BakeOne(Art, MeshFolder, MaterialFolder);
	}
	return bAllOk;
#else
	UE_LOG(LogMasterJumper, Warning, TEXT("BakeArtAssets is editor-only."));
	return false;
#endif
}
