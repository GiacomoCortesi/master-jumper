#pragma once

#include "CoreMinimal.h"

/**
 * Single source of truth for the one level's geometry (cm; X = run direction, Z up, floor at Z = 0).
 * Tools/jump_sim.py mirrors these numbers - keep them in sync when re-tuning.
 */
namespace MJLayout
{
	inline constexpr float CorridorHalfWidth = 200.f;
	inline constexpr float CeilingZ = 330.f;
	inline constexpr float CeilingThickness = 20.f;
	inline constexpr float WallThickness = 20.f;

	inline constexpr float CorridorStartX = -300.f;
	inline constexpr float FloorEndX = 3150.f;      // stairwell gap starts here (overshoot = fall)
	inline constexpr float CorridorEndX = 3800.f;
	inline constexpr float PitBottomZ = -900.f;
	inline constexpr float KillZ = -250.f;

	inline constexpr float PlayerStartX = 0.f;

	inline constexpr float ObstacleFrontX = 2400.f;
	inline constexpr float ObstacleDepth = 220.f;
	inline constexpr float ObstacleHeight = 165.f;

	inline constexpr float GateStartX = 2640.f;
	inline constexpr float GateEndX = FloorEndX;
	inline constexpr float GateHeight = 150.f;

	inline constexpr float RunUpMarkerStartX = 800.f;
	inline constexpr float RunUpMarkerSpacing = 200.f;
}
