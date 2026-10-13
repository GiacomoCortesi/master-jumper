#include "MJJumpCharge.h"

FMJJumpCharge::FMJJumpCharge()
{
	FRichCurve* Curve = HoldToStrength.GetRichCurve();
	auto AddLinearKey = [Curve](float Time, float Value)
	{
		const FKeyHandle Key = Curve->AddKey(Time, Value);
		Curve->SetKeyInterpMode(Key, RCIM_Linear);
	};
	AddLinearKey(0.f, 0.30f);
	AddLinearKey(MinHoldSeconds, 0.62f);
	AddLinearKey(OptimalHoldSeconds, 1.0f);
	AddLinearKey(MaxHoldSeconds, 1.22f);
}

void FMJJumpCharge::Begin(double Now)
{
	ChargeStartTime = Now;
	bCharging = true;
}

void FMJJumpCharge::Reset()
{
	ChargeStartTime = 0.0;
	bCharging = false;
}

float FMJJumpCharge::GetHeldSeconds(double Now) const
{
	return bCharging ? static_cast<float>(Now - ChargeStartTime) : 0.f;
}

float FMJJumpCharge::Commit(double Now)
{
	const float Held = GetHeldSeconds(Now);
	bCharging = false;
	return Held;
}

float FMJJumpCharge::EvaluateStrength(float HeldSeconds) const
{
	if (HeldSeconds < MinHoldSeconds)
	{
		return FumbleStrength;
	}
	const FRichCurve* Curve = HoldToStrength.GetRichCurveConst();
	if (Curve && Curve->GetNumKeys() > 0)
	{
		return Curve->Eval(FMath::Min(HeldSeconds, MaxHoldSeconds));
	}
	return FMath::Min(HeldSeconds, MaxHoldSeconds) / FMath::Max(OptimalHoldSeconds, KINDA_SMALL_NUMBER);
}

FVector2D FMJJumpCharge::EvaluateLaunch(float HeldSeconds) const
{
	const float Strength = EvaluateStrength(HeldSeconds);
	return FVector2D(HorizontalImpulse * Strength, VerticalImpulse * Strength);
}
