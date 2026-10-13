#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/Package.h"
#include "Misc/PackageName.h"
#include "MJMeshBuilder.h"
#include "MJArtLibrary.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

/** The custom art pieces required by the MVP (plus the wall panel that carries the corridor texture). */
UENUM(BlueprintType)
enum class EMJArt : uint8
{
	OfficeChair,
	OfficeComputer,
	PovArms,
	CorridorWallPanel,
};

namespace MJAssets
{
	inline const FString Root = TEXT("/Game/MasterJumper");
	inline const FString Meshes = Root / TEXT("Meshes");
	inline const FString Materials = Root / TEXT("Materials");
	inline const FString Textures = Root / TEXT("Textures");
	inline const FString Input = Root / TEXT("Input");
	inline const FString Blueprints = Root / TEXT("Blueprints");
}

/**
 * Art pipeline. Geometry is authored in code (BuildArt) so it is identical whether it is:
 *  - baked into /Game/MasterJumper/Meshes/SM_* by Tools/setup_project.py (BakeArtAssets), or
 *  - built as a transient mesh at runtime when the project has not been set up yet.
 */
UCLASS()
class MASTERJUMPER_API UMJArtLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static FMJMeshBuilder BuildArt(EMJArt Art);
	static FString GetMeshAssetName(EMJArt Art);
	static FString GetParentMaterialName(EMJArt Art);

	/** Baked SM_* asset when present, else (if allowed) a cached transient mesh built from code. */
	static UStaticMesh* ResolveMesh(EMJArt Art, bool bAllowTransient);

	/** Baked meshes keep their own material instances; transient ones get colour MIDs per slot. */
	static void ApplyArtMaterials(UStaticMeshComponent* Component, EMJArt Art);

	/** Solid-colour lit material for blockout pieces (floor, ceiling, doors, markers). */
	static UMaterialInstanceDynamic* MakeSolidMaterial(UObject* Outer, const FLinearColor& Color, UMaterialInterface* Parent = nullptr);

	static UStaticMesh* GetEngineCube();
	static UMaterialInterface* GetBasicShapeMaterial();

	/** Finds/loads an asset by package path ("/Game/X/Y") without logging when it does not exist. */
	template <typename T>
	static T* LoadOptional(const FString& PackagePath)
	{
		const FString ObjectPath = PackagePath + TEXT(".") + FPackageName::GetShortName(PackagePath);
		if (T* Found = FindObject<T>(nullptr, *ObjectPath))
		{
			return Found;
		}
		if (!FPackageName::DoesPackageExist(PackagePath))
		{
			return nullptr;
		}
		return LoadObject<T>(nullptr, *ObjectPath);
	}

	/** Editor only: writes SM_* meshes and their MI_* colour instances (parents must already exist). Unsaved; the caller saves. */
	UFUNCTION(BlueprintCallable, Category = "MasterJumper|Art")
	static bool BakeArtAssets(const FString& MeshFolder, const FString& MaterialFolder);
};
