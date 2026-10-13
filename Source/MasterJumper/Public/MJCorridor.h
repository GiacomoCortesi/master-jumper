#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MJCorridor.generated.h"

class UInstancedStaticMeshComponent;
class UPointLightComponent;

/**
 * The office corridor environment: textured wall panels, carpet-tile floor, ceiling with light panels,
 * doors, run-up tape markers, and the stairwell gap past the landing zone. Built from MJLayout.
 */
UCLASS()
class MASTERJUMPER_API AMJCorridor : public AActor
{
	GENERATED_BODY()

public:
	AMJCorridor();

	static constexpr int32 NumLights = 9;

	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UInstancedStaticMeshComponent> WallPanels;
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UInstancedStaticMeshComponent> FloorTilesA;
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UInstancedStaticMeshComponent> FloorTilesB;
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UInstancedStaticMeshComponent> Ceiling;
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UInstancedStaticMeshComponent> LightPanels;
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UInstancedStaticMeshComponent> Doors;
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UInstancedStaticMeshComponent> DoorFrames;
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UInstancedStaticMeshComponent> RunUpMarkers;
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UInstancedStaticMeshComponent> Concrete;
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UInstancedStaticMeshComponent> ExitSign;
	UPROPERTY(VisibleAnywhere, Category = "Components") TArray<TObjectPtr<UPointLightComponent>> Lights;

	/** Candela per ceiling light. */
	UPROPERTY(EditAnywhere, Category = "Lighting")
	float LightIntensity = 60.f;

	UPROPERTY(EditAnywhere, Category = "Lighting")
	FLinearColor LightColor = FLinearColor(1.f, 0.95f, 0.88f);

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

private:
	void Build(bool bAllowTransient);
};
