#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MJTypes.h"
#include "MJJumpStateMachine.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMJOnJumpStateChanged, EMJJumpState, OldState, EMJJumpState, NewState);

/**
 * JumpStateMachine: Running -> Charging -> Takeoff -> Airborne -> Landing -> Running,
 * with Failed / Succeeded as terminal states that only ForceReset() leaves.
 */
UCLASS(ClassGroup = (MasterJumper), meta = (BlueprintSpawnableComponent))
class MASTERJUMPER_API UMJJumpStateMachine : public UActorComponent
{
	GENERATED_BODY()

public:
	UMJJumpStateMachine();

	UFUNCTION(BlueprintPure, Category = "MasterJumper|Jump")
	EMJJumpState GetState() const { return State; }

	UFUNCTION(BlueprintPure, Category = "MasterJumper|Jump")
	float GetTimeInState() const;

	/** Returns false (and changes nothing) if the transition is not in the allowed table. */
	UFUNCTION(BlueprintCallable, Category = "MasterJumper|Jump")
	bool TryTransition(EMJJumpState NewState);

	/** Back to Running from any state (soft restart). */
	UFUNCTION(BlueprintCallable, Category = "MasterJumper|Jump")
	void ForceReset();

	UPROPERTY(BlueprintAssignable, Category = "MasterJumper|Jump")
	FMJOnJumpStateChanged OnStateChanged;

	UPROPERTY(EditAnywhere, Category = "MasterJumper|Jump")
	bool bLogTransitions = true;

	static bool IsTransitionAllowed(EMJJumpState From, EMJJumpState To);

private:
	void SetState(EMJJumpState NewState);

	EMJJumpState State = EMJJumpState::Running;
	double EnteredStateAt = 0.0;
};
