#include "MJJumpStateMachine.h"
#include "Engine/World.h"

UMJJumpStateMachine::UMJJumpStateMachine()
{
	PrimaryComponentTick.bCanEverTick = false;
}

float UMJJumpStateMachine::GetTimeInState() const
{
	const UWorld* World = GetWorld();
	return World ? static_cast<float>(World->GetTimeSeconds() - EnteredStateAt) : 0.f;
}

bool UMJJumpStateMachine::IsTransitionAllowed(EMJJumpState From, EMJJumpState To)
{
	using S = EMJJumpState;
	switch (From)
	{
	case S::Running:  return To == S::Charging || To == S::Failed || To == S::Succeeded;
	case S::Charging: return To == S::Takeoff || To == S::Failed;
	case S::Takeoff:  return To == S::Airborne || To == S::Failed;
	case S::Airborne: return To == S::Landing || To == S::Failed;
	case S::Landing:  return To == S::Running || To == S::Failed || To == S::Succeeded;
	default:          return false; // Failed / Succeeded are terminal
	}
}

bool UMJJumpStateMachine::TryTransition(EMJJumpState NewState)
{
	if (!IsTransitionAllowed(State, NewState))
	{
		return false;
	}
	SetState(NewState);
	return true;
}

void UMJJumpStateMachine::ForceReset()
{
	SetState(EMJJumpState::Running);
}

void UMJJumpStateMachine::SetState(EMJJumpState NewState)
{
	const EMJJumpState OldState = State;
	if (bLogTransitions)
	{
		UE_LOG(LogMasterJumper, Log, TEXT("[Jump] %s -> %s after %.3fs"),
			*UEnum::GetValueAsString(OldState), *UEnum::GetValueAsString(NewState), GetTimeInState());
	}
	State = NewState;
	if (const UWorld* World = GetWorld())
	{
		EnteredStateAt = World->GetTimeSeconds();
	}
	OnStateChanged.Broadcast(OldState, NewState);
}
