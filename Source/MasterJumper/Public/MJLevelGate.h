#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MJLayout.h"
#include "MJLevelGate.generated.h"

class UBoxComponent;
class AMJPlayer;

/** LevelGate: success volume past the obstacle. Wins when the player is grounded with their centre inside it. Origin = box centre. */
UCLASS()
class MASTERJUMPER_API AMJLevelGate : public AActor
{
	GENERATED_BODY()

public:
	AMJLevelGate();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> Volume;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gate")
	float Length = MJLayout::GateEndX - MJLayout::GateStartX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gate")
	float HalfWidth = MJLayout::CorridorHalfWidth;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gate")
	float Height = MJLayout::GateHeight;

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	TWeakObjectPtr<AMJPlayer> PlayerInside;
};
