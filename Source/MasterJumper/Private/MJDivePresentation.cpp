#include "MJDivePresentation.h"
#include "MJJumpStateMachine.h"
#include "Components/SceneComponent.h"

UMJDivePresentation::UMJDivePresentation()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UMJDivePresentation::BindTo(UMJJumpStateMachine* StateMachine)
{
	if (StateMachine)
	{
		StateMachine->OnStateChanged.AddUniqueDynamic(this, &UMJDivePresentation::HandleStateChanged);
	}
}

void UMJDivePresentation::HandleStateChanged(EMJJumpState OldState, EMJJumpState NewState)
{
	if (NewState == EMJJumpState::Airborne)
	{
		bShowing = true;
		ReachAlpha = 0.f;
		if (ArmsRoot)
		{
			ArmsRoot->SetRelativeLocation(HiddenOffset);
			ArmsRoot->SetVisibility(true, true);
		}
		SetComponentTickEnabled(true);
	}
	else if (NewState != EMJJumpState::Failed)
	{
		// Keep the arms frozen in frame on a crash (reads better than vanishing); hide on every other exit.
		HideArms();
	}
}

void UMJDivePresentation::HideArms()
{
	bShowing = false;
	SetComponentTickEnabled(false);
	if (ArmsRoot)
	{
		ArmsRoot->SetVisibility(false, true);
	}
}

void UMJDivePresentation::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bShowing || !ArmsRoot)
	{
		return;
	}
	ReachAlpha = FMath::Min(1.f, ReachAlpha + DeltaTime / FMath::Max(ReachInTime, KINDA_SMALL_NUMBER));
	const float Eased = 1.f - FMath::Square(1.f - ReachAlpha);
	ArmsRoot->SetRelativeLocation(FMath::Lerp(HiddenOffset, FVector::ZeroVector, Eased));
	if (ReachAlpha >= 1.f)
	{
		SetComponentTickEnabled(false);
	}
}
