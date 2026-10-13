#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MJGameMode.generated.h"

/**
 * Global default game mode. Makes sure the corridor, obstacle, gate and level controller exist (spawning any the
 * map does not contain), so PIE is playable in LVL_OfficeCorridor_01 or even in an empty map.
 * Prefers the BP_MJ_* Blueprint subclasses when Tools/setup_project.py has created them.
 */
UCLASS()
class MASTERJUMPER_API AMJGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMJGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void StartPlay() override;
	virtual void RestartPlayer(AController* NewPlayer) override;

private:
	void EnsureLevelActors();
	bool bLevelActorsEnsured = false;
};
