#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MJTypes.h"
#include "MJDivePresentation.generated.h"

class USceneComponent;

/** DivePresentation: shows the first-person Superman arms while Airborne (same pose for obstacle and gap dives). */
UCLASS(ClassGroup = (MasterJumper), meta = (BlueprintSpawnableComponent))
class MASTERJUMPER_API UMJDivePresentation : public UActorComponent
{
	GENERATED_BODY()

public:
	UMJDivePresentation();

	/** Root of the arm meshes (attached to the camera). Set by the owning pawn. */
	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> ArmsRoot;

	/** Arms slide in from below the frame over this time when the dive starts. */
	UPROPERTY(EditAnywhere, Category = "Dive", meta = (Units = "s"))
	float ReachInTime = 0.12f;

	/** Camera-space offset the arms start from (off-screen). */
	UPROPERTY(EditAnywhere, Category = "Dive")
	FVector HiddenOffset = FVector(-25.f, 0.f, -30.f);

	void BindTo(class UMJJumpStateMachine* StateMachine);
	void HideArms();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UFUNCTION()
	void HandleStateChanged(EMJJumpState OldState, EMJJumpState NewState);

	float ReachAlpha = 0.f;
	bool bShowing = false;
};
