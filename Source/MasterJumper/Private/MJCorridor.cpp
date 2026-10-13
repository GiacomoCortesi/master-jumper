#include "MJCorridor.h"
#include "MJArtLibrary.h"
#include "MJLayout.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"

namespace
{
	// Engine cube is 100 cm, centred: scale = size / 100.
	FTransform CubeBox(const FVector& Min, const FVector& Max)
	{
		return FTransform(FRotator::ZeroRotator, (Min + Max) * 0.5f, (Max - Min) / 100.f);
	}
}

AMJCorridor::AMJCorridor()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	auto MakeIsm = [this](const TCHAR* Name, bool bCollides)
	{
		UInstancedStaticMeshComponent* Component = CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
		Component->SetupAttachment(RootComponent);
		Component->SetCollisionProfileName(bCollides ? UCollisionProfile::BlockAll_ProfileName : UCollisionProfile::NoCollision_ProfileName);
		Component->SetGenerateOverlapEvents(false);
		return Component;
	};
	WallPanels = MakeIsm(TEXT("WallPanels"), false);
	FloorTilesA = MakeIsm(TEXT("FloorTilesA"), true);
	FloorTilesB = MakeIsm(TEXT("FloorTilesB"), true);
	Ceiling = MakeIsm(TEXT("Ceiling"), true);
	LightPanels = MakeIsm(TEXT("LightPanels"), false);
	Doors = MakeIsm(TEXT("Doors"), false);
	DoorFrames = MakeIsm(TEXT("DoorFrames"), false);
	RunUpMarkers = MakeIsm(TEXT("RunUpMarkers"), false);
	Concrete = MakeIsm(TEXT("Concrete"), true);
	ExitSign = MakeIsm(TEXT("ExitSign"), false);

	for (int32 i = 0; i < NumLights; ++i)
	{
		UPointLightComponent* Light = CreateDefaultSubobject<UPointLightComponent>(*FString::Printf(TEXT("CeilingLight%d"), i));
		Light->SetupAttachment(RootComponent);
		Light->SetMobility(EComponentMobility::Movable);
		Lights.Add(Light);
	}
}

void AMJCorridor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Build(GetWorld() && GetWorld()->IsGameWorld());
}

void AMJCorridor::BeginPlay()
{
	Super::BeginPlay();
	Build(true);
}

void AMJCorridor::Build(bool bAllowTransient)
{
	using namespace MJLayout;
	UStaticMesh* Cube = UMJArtLibrary::GetEngineCube();
	const float HW = CorridorHalfWidth;
	const float OuterHW = CorridorHalfWidth + WallThickness;

	auto Prepare = [Cube](UInstancedStaticMeshComponent* Component, const FLinearColor& Color)
	{
		Component->ClearInstances();
		Component->SetStaticMesh(Cube);
		Component->SetMaterial(0, UMJArtLibrary::MakeSolidMaterial(Component, Color));
	};

	// --- Textured wall panels (the M_CorridorWall / T_CorridorWall asset) ---
	WallPanels->ClearInstances();
	if (UStaticMesh* Panel = UMJArtLibrary::ResolveMesh(EMJArt::CorridorWallPanel, bAllowTransient))
	{
		WallPanels->SetStaticMesh(Panel);
		UMJArtLibrary::ApplyArtMaterials(WallPanels, EMJArt::CorridorWallPanel);
		for (float X = CorridorStartX + 200.f; X - 200.f < CorridorEndX; X += 400.f)
		{
			for (const float Side : { -1.f, 1.f })
			{
				WallPanels->AddInstance(FTransform(FVector(X, Side * (HW + WallThickness * 0.5f), 0.f)));
			}
		}
	}

	// --- Carpet tiles (alternating, gives a sense of speed) up to the stairwell gap ---
	Prepare(FloorTilesA, FLinearColor(0.035f, 0.040f, 0.050f));
	Prepare(FloorTilesB, FLinearColor(0.050f, 0.055f, 0.065f));
	int32 TileIndex = 0;
	for (float X = CorridorStartX; X < FloorEndX; X += 200.f, ++TileIndex)
	{
		const float X1 = FMath::Min(X + 200.f, FloorEndX);
		(TileIndex % 2 ? FloorTilesB : FloorTilesA)->AddInstance(CubeBox(FVector(X, -OuterHW, -20.f), FVector(X1, OuterHW, 0.f)));
	}

	// --- Ceiling slab + light panels + point lights ---
	Prepare(Ceiling, FLinearColor(0.55f, 0.55f, 0.52f));
	Ceiling->AddInstance(CubeBox(FVector(CorridorStartX - 20.f, -OuterHW, CeilingZ), FVector(CorridorEndX + 20.f, OuterHW, CeilingZ + CeilingThickness)));

	Prepare(LightPanels, FLinearColor(1.f, 1.f, 0.97f));
	for (int32 i = 0; i < Lights.Num(); ++i)
	{
		const float X = CorridorStartX + 300.f + i * ((CorridorEndX - CorridorStartX - 400.f) / FMath::Max(Lights.Num() - 1, 1));
		LightPanels->AddInstance(CubeBox(FVector(X - 60.f, -30.f, CeilingZ - 2.f), FVector(X + 60.f, 30.f, CeilingZ)));

		UPointLightComponent* Light = Lights[i];
		Light->SetRelativeLocation(FVector(X, 0.f, CeilingZ - 25.f));
		Light->SetIntensityUnits(ELightUnits::Candelas);
		Light->SetIntensity(LightIntensity);
		Light->SetLightColor(LightColor);
		Light->SetAttenuationRadius(900.f);
		Light->SetSourceRadius(25.f);
		Light->SetMobility(EComponentMobility::Movable);
	}

	// --- Office doors along the walls (alternating sides) ---
	Prepare(Doors, FLinearColor(0.16f, 0.08f, 0.035f));
	Prepare(DoorFrames, FLinearColor(0.02f, 0.02f, 0.022f));
	const float DoorXs[] = { 350.f, 1250.f, 1900.f, 2900.f };
	for (int32 i = 0; i < static_cast<int32>(UE_ARRAY_COUNT(DoorXs)); ++i)
	{
		const float Side = (i % 2) ? 1.f : -1.f;
		const float Face = Side * HW;
		const float X = DoorXs[i];
		Doors->AddInstance(CubeBox(FVector(X - 50.f, FMath::Min(Face, Face - Side * 3.f), 0.f), FVector(X + 50.f, FMath::Max(Face, Face - Side * 3.f), 210.f)));
		DoorFrames->AddInstance(CubeBox(FVector(X - 58.f, FMath::Min(Face, Face - Side * 4.f), 210.f), FVector(X + 58.f, FMath::Max(Face, Face - Side * 4.f), 218.f)));
		for (const float Edge : { -1.f, 1.f })
		{
			const float FX = X + Edge * 54.f;
			DoorFrames->AddInstance(CubeBox(FVector(FX - 4.f, FMath::Min(Face, Face - Side * 4.f), 0.f), FVector(FX + 4.f, FMath::Max(Face, Face - Side * 4.f), 218.f)));
		}
	}

	// --- Run-up tape markers (environmental rhythm cue, not UI) ---
	Prepare(RunUpMarkers, FLinearColor(0.75f, 0.55f, 0.02f));
	for (float X = RunUpMarkerStartX; X < ObstacleFrontX - 50.f; X += RunUpMarkerSpacing)
	{
		RunUpMarkers->AddInstance(CubeBox(FVector(X - 5.f, -HW + 20.f, 0.f), FVector(X + 5.f, HW - 20.f, 0.6f)));
	}

	// --- Stairwell gap, end walls ---
	Prepare(Concrete, FLinearColor(0.08f, 0.08f, 0.085f));
	Concrete->AddInstance(CubeBox(FVector(CorridorStartX - 20.f, -OuterHW, -20.f), FVector(CorridorStartX, OuterHW, CeilingZ)));          // back wall
	Concrete->AddInstance(CubeBox(FVector(CorridorEndX, -OuterHW, PitBottomZ), FVector(CorridorEndX + 20.f, OuterHW, CeilingZ)));        // far wall
	Concrete->AddInstance(CubeBox(FVector(FloorEndX, -OuterHW, PitBottomZ - 20.f), FVector(CorridorEndX, OuterHW, PitBottomZ)));          // pit floor
	Concrete->AddInstance(CubeBox(FVector(FloorEndX - 20.f, -OuterHW, PitBottomZ), FVector(FloorEndX, OuterHW, -20.f)));                 // pit near wall
	for (const float Side : { -1.f, 1.f })
	{
		Concrete->AddInstance(CubeBox(FVector(FloorEndX, FMath::Min(Side * HW, Side * OuterHW), PitBottomZ), FVector(CorridorEndX, FMath::Max(Side * HW, Side * OuterHW), 0.f)));
	}

	Prepare(ExitSign, FLinearColor(0.02f, 0.45f, 0.08f));
	ExitSign->AddInstance(CubeBox(FVector(CorridorEndX - 4.f, -50.f, 250.f), FVector(CorridorEndX, 50.f, 290.f)));
}
