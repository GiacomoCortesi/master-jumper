#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MJPlayerController.generated.h"

/** Game-only input, hidden cursor. There is deliberately no look binding: the POV camera is fixed. */
UCLASS()
class MASTERJUMPER_API AMJPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMJPlayerController();

protected:
	virtual void BeginPlay() override;
};
