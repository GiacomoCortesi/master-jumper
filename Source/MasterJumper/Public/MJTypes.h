#pragma once

#include "CoreMinimal.h"
#include "MJTypes.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogMasterJumper, Log, All);

/** The jump state machine states (authoritative list from the MVP brief). */
UENUM(BlueprintType)
enum class EMJJumpState : uint8
{
	Running,
	Charging,
	Takeoff,
	Airborne,
	Landing,
	Failed,
	Succeeded,
};

UENUM(BlueprintType)
enum class EMJFailReason : uint8
{
	None,
	HitObstacle,
	HitCeiling,
	HitWall,
	FellInGap,
	AirborneTimeout,
};

UENUM(BlueprintType)
enum class EMJOutcome : uint8
{
	InProgress,
	Failed,
	Succeeded,
};
