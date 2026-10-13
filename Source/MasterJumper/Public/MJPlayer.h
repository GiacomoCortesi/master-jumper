#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "MJTypes.h"
#include "MJJumpCharge.h"
#include "MJPlayer.generated.h"

class UCapsuleComponent;
class UCameraComponent;
class UStaticMeshComponent;
class UInputAction;
class UInputMappingContext;
class UMJJumpStateMachine;
class UMJDivePresentation;
struct FHitResult;

/**
 * PlayerPawn: auto-runs down +X, fixed first-person camera (no look input exists), one hold/release key.
 * Movement is kinematic (swept capsule) so the jump is deterministic and matches Tools/jump_sim.py.
 */
UCLASS()
class MASTERJUMPER_API AMJPlayer : public APawn
{
	GENERATED_BODY()

public:
	AMJPlayer();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// --- Called by input / the level controller ---
	void PressJump();
	void ReleaseJump();

	/** Soft restart: FloorLocation is the start point on the floor (Z = floor height). */
	void ResetToStart(const FVector& FloorLocation);
	void SetLevelController(class AMJLevelController* InController);
	void SetRunEnabled(bool bEnabled) { bRunEnabled = bEnabled; }
	void EnterFailed();
	void EnterSucceeded();

	bool IsGrounded() const { return !bInAir; }
	EMJJumpState GetJumpState() const;

	UMJJumpStateMachine* GetStateMachine() const { return StateMachine; }

	// --- Components ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCapsuleComponent> Capsule;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> ArmsRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> ArmsMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UMJJumpStateMachine> StateMachine;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UMJDivePresentation> DivePresentation;

	// --- Tuning ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Jump")
	FMJJumpCharge Charge;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Run", meta = (Units = "cm/s"))
	float RunSpeed = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body")
	float CapsuleRadius = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body")
	float StandingHalfHeight = 90.f;

	/** Collision half-height while diving (body is horizontal, so the capsule shrinks around the centre). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body")
	float DiveHalfHeight = 45.f;

	/** Eye height above the capsule centre while running (centre is 90 cm up -> eyes at 160 cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	float RunEyeOffset = 70.f;

	/** Eye height above centre while diving; must stay below DiveHalfHeight so the camera never pokes through the ceiling. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	float DiveEyeOffset = 35.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	float FieldOfView = 95.f;

	/** Tiny vertical running bob (cm). 0 disables it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	float RunBobAmplitude = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	float RunBobFrequency = 2.6f;

	/** Hover gap kept between capsule and floor so horizontal sweeps never start in penetration. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body")
	float FloorGap = 2.f;

	/** Optional input assets; runtime equivalents are created when these are not set / not found. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> JumpChargeAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> MappingContext;

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	void EnsureInputObjects();
	void SetupArms(bool bAllowTransient);

	void Launch();
	void TickGrounded(float DeltaSeconds);
	void TickInAir(float DeltaSeconds);
	bool FindFloor(FHitResult& OutHit) const;
	void Touchdown(const FHitResult& Hit);
	void HandleBlockingHit(const FHitResult& Hit);
	void ReportFail(EMJFailReason Reason);
	void SetCapsuleHalfHeightKeepCentre(float HalfHeight);
	void UpdateCamera(float DeltaSeconds);

	UFUNCTION()
	void OnJumpStarted();

	UFUNCTION()
	void OnJumpCompleted();

	TWeakObjectPtr<class AMJLevelController> LevelController;
	FVector SpawnFloorLocation = FVector::ZeroVector;
	FVector Velocity = FVector::ZeroVector;
	bool bInAir = false;
	bool bRunEnabled = false;
	bool bJumpSpent = false;
	float LastHeldSeconds = 0.f;
	float RunBobTime = 0.f;
	float LandingStartEye = 0.f;
};
