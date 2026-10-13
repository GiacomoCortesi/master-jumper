#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MJLayout.h"
#include "MJObstacleStack.generated.h"

class UBoxComponent;
class UInstancedStaticMeshComponent;

/**
 * ObstacleVolume: the pile of office chairs + computers. One blocking box (what the jump must clear) plus a
 * fail trigger 1 cm larger. Origin = floor centre of the footprint. Meshes are visual only (no collision).
 */
UCLASS()
class MASTERJUMPER_API AMJObstacleStack : public AActor
{
	GENERATED_BODY()

public:
	AMJObstacleStack();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> BlockingVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> FailTrigger;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> Chairs;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> Computers;

	/** Along the run direction. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Obstacle")
	float Depth = MJLayout::ObstacleDepth;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Obstacle")
	float Height = MJLayout::ObstacleHeight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Obstacle")
	float HalfWidth = MJLayout::CorridorHalfWidth;

	/** Seed for the pile's yaw/tilt jitter. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Obstacle")
	int32 PileSeed = 7;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

private:
	void Build(bool bAllowTransient);

	UFUNCTION()
	void HandleFailOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
