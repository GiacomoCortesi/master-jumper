#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MJTypes.h"
#include "MJLayout.h"
#include "MJLevelController.generated.h"

class AMJPlayer;

/** LevelController: starts attempts, freezes on success, soft-restarts (no map reload) on fail. Owns player start + kill plane. */
UCLASS()
class MASTERJUMPER_API AMJLevelController : public AActor
{
	GENERATED_BODY()

public:
	AMJLevelController();

	static AMJLevelController* Get(const UObject* WorldContextObject);

	void RegisterPlayer(AMJPlayer* InPlayer);
	void ReportFail(EMJFailReason Reason);
	void ReportSuccess();
	void RequestRestart();

	UFUNCTION(BlueprintPure, Category = "MasterJumper")
	EMJOutcome GetOutcome() const { return Outcome; }

	UFUNCTION(BlueprintPure, Category = "MasterJumper")
	EMJFailReason GetLastFailReason() const { return LastFailReason; }

	/** Start point on the floor. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
	FVector PlayerStartLocation = FVector(MJLayout::PlayerStartX, 0.f, 0.f);

	/** Kill plane: below this Z the attempt fails (stairwell gap). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
	float KillZ = MJLayout::KillZ;

	/** Standing still before auto-run starts each attempt. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level", meta = (Units = "s"))
	float RunStartDelay = 0.6f;

	/** Freeze time on a fail before the soft restart. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level", meta = (Units = "s"))
	float FailRestartDelay = 1.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level", meta = (Units = "s"))
	float FadeTime = 0.35f;

private:
	void StartAttempt();
	void BeginRun();
	void Fade(float From, float To);

	TWeakObjectPtr<AMJPlayer> Player;
	EMJOutcome Outcome = EMJOutcome::InProgress;
	EMJFailReason LastFailReason = EMJFailReason::None;
	FTimerHandle Timer;
	int32 Attempt = 0;
};
