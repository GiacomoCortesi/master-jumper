#pragma once

#include "CoreMinimal.h"
#include "Curves/CurveFloat.h"
#include "MJJumpCharge.generated.h"

/**
 * JumpCharge: hold-time accumulator + the hold -> launch mapping, with every jump tuning knob.
 * None of these values are ever shown to the player.
 * Defaults are tuned with Tools/jump_sim.py for the single obstacle in MJLayout.h.
 */
USTRUCT(BlueprintType)
struct MASTERJUMPER_API FMJJumpCharge
{
	GENERATED_BODY()

	FMJJumpCharge();

	/** Below this hold the jump fumbles into a weak hop. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hold", meta = (ClampMin = "0.0", Units = "s"))
	float MinHoldSeconds = 0.20f;

	/** Hold that produces exactly HorizontalImpulse / VerticalImpulse (strength 1.0). Internal only. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hold", meta = (ClampMin = "0.0", Units = "s"))
	float OptimalHoldSeconds = 0.55f;

	/** Strength stops growing past this hold (curve end). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hold", meta = (ClampMin = "0.0", Units = "s"))
	float MaxHoldSeconds = 0.90f;

	/** X = seconds held, Y = launch strength multiplier. Keys default to Min/Optimal/Max above (edit both together). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hold")
	FRuntimeFloatCurve HoldToStrength;

	/** Strength used when the hold is shorter than MinHoldSeconds. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hold")
	float FumbleStrength = 0.30f;

	/** Forward launch speed at strength 1.0 (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Launch")
	float HorizontalImpulse = 820.f;

	/** Upward launch speed at strength 1.0 (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Launch")
	float VerticalImpulse = 800.f;

	/** Multiplier on 980 cm/s^2 while airborne. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Launch")
	float GravityScaleInAir = 1.9f;

	/** Grounded wind-up between release and leaving the floor. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phases", meta = (Units = "s"))
	float TakeoffDuration = 0.12f;

	/** Safety window: an airborne phase longer than this is a fail. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phases", meta = (Units = "s"))
	float MaxAirborneSeconds = 2.5f;

	/** Time to rise from the dive back to running height after touchdown. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phases", meta = (Units = "s"))
	float LandingBlendTime = 0.25f;

	void Begin(double Now);
	void Reset();
	bool IsCharging() const { return bCharging; }
	float GetHeldSeconds(double Now) const;

	/** Ends the charge and returns the held seconds. */
	float Commit(double Now);

	float EvaluateStrength(float HeldSeconds) const;

	/** X = forward speed, Y = upward speed. */
	FVector2D EvaluateLaunch(float HeldSeconds) const;

	float GetAirGravity() const { return 980.f * GravityScaleInAir; }

private:
	double ChargeStartTime = 0.0;
	bool bCharging = false;
};
