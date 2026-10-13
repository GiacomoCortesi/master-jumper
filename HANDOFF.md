# HANDOFF — Master Jumper Sbanfer MVP

## What was built
- One level (`LVL_OfficeCorridor_01`, or any map via the global game mode): a 41 m office corridor. It has a 24 m run-up with tape markers every 2 m, a 2.2 m deep × 1.65 m high × 4 m wide pile of office chairs and CRT computers, a 5.1 m landing strip, and then an open stairwell (the overshoot fail).
- A C++ gameplay core implementing the brief's architecture exactly: PlayerPawn, JumpStateMachine, JumpCharge, DivePresentation, ObstacleVolume, LevelGate and LevelController.
- A hold → release jump with four phases: pre-jump run (`Running`/`Charging`), `Takeoff` (0.12 s grounded dip), `Airborne` (dive arms shown), `Landing` (0.25 s rise back to eye height).
- Fail paths (pile, ceiling, wall, stairwell or kill plane, air timeout) fade to black and soft-restart without reloading the map. Success freezes the game, shows CLEARED!, and the jump key runs again.
- An art pipeline: geometry is authored in code (`UMJArtLibrary`), then baked into `SM_*` assets by the editor script, or built transiently at runtime as a fallback.
- An offline tuning and smoke-test simulator: `Tools/jump_sim.py`.

The design decisions taken without asking: auto-run (as the brief preferred), one jump per attempt, keys Space/LMB/Gamepad A, a fixed camera with a ±1.2 cm run bob plus a takeoff dip and landing rise, C++ over Blueprints, and a kinematic pawn instead of CharacterMovement (for determinism).

## How to play
Follow `README.md` → *How to open and play*. In short: build, run `Tools/setup_project.py` once in the editor, then press Play. Hold Space, let go, and dive.

## Tuning knobs (on `BP_MJ_Player` → *Jump → Charge*, plus *Run*, *Body*, *Camera*)
| Knob | Default | Effect |
|---|---|---|
| `MinHoldSeconds` | 0.20 s | Below this the jump fumbles (`FumbleStrength` 0.30) |
| `OptimalHoldSeconds` | 0.55 s | Hold that produces strength 1.0. Never shown to the player. |
| `MaxHoldSeconds` | 0.90 s | Strength stops rising (1.22) |
| `HoldToStrength` curve | (0,0.30) (0.20,0.62) (0.55,1.0) (0.90,1.22), linear | The feel curve. Edit together with the three holds above. |
| `HorizontalImpulse` / `VerticalImpulse` | 820 / 800 cm/s | Launch velocity at strength 1.0 |
| `GravityScaleInAir` | 1.9 (× 980) | Arc snappiness |
| `TakeoffDuration` / `LandingBlendTime` / `MaxAirborneSeconds` | 0.12 / 0.25 / 2.5 s | Phase timings |
| `RunSpeed` | 600 cm/s | Run-up pace. Changes the whole timing window. |
| `StandingHalfHeight` / `DiveHalfHeight` / `CapsuleRadius` | 90 / 45 / 30 cm | Collision while running vs diving |
| `RunEyeOffset` / `DiveEyeOffset` | 70 / 35 cm | Camera height. The dive eye must stay below `DiveHalfHeight` so the camera can't clip into the ceiling. |
| Level geometry | `MJLayout.h` | Obstacle position, size, landing strip, ceiling, gap |

**Re-tuning workflow:** edit the numbers in `Tools/jump_sim.py`, run it, and check that the practiced-player medians stay at 2–3 or more per 10 for σ 70 ms and that the hold window is wide enough. Then copy the values into `MJJumpCharge.h` / `MJLayout.h`. The simulator mirrors the pawn's movement code, but **keep the two in sync by hand**.

## Next-level difficulty levers (design notes only — NOT implemented)
1. **Obstacle height** (165 cm now). Every +10 cm needs about +4% strength. Raising it narrows the hold window from the short side.
2. **Obstacle depth** (220 cm). Deeper piles demand a later apex, which narrows the release-position window.
3. **Landing strip length / gap width** (510 cm strip, then the stairwell). A shorter strip punishes over-holding. A real gap in front of the landing (the "into gap" case) reuses the same dive. Just remove floor tiles between the obstacle and the gate.
4. **Ceiling height** (330 cm). Lowering it caps over-charged jumps and adds a second ceiling-side failure mode.
5. **Run speed** (600 cm/s). Faster runs compress the press/release timing in time while keeping the distances.
6. **Combos:** a second obstacle after the first landing would need jump re-arming (`bJumpSpent`) per obstacle. That's a deliberate MVP restriction today.
7. **Run-up markers:** removing or spacing out the floor tape takes away the player's rhythm reference. It's a cheap "hard mode" lever.

## Open items for whoever opens it in UE 5.4 first
- Compile, fix anything the compiler flags, and run the README acceptance checklist in PIE.
- Check that the lighting exposure looks like a lit office. If not, adjust `AMJCorridor::LightIntensity` or add a post-process volume.
- Commit the generated `Content/MasterJumper/**` assets (Git LFS).
