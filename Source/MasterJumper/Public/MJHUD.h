#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MJHUD.generated.h"

/** Win / lose text only. Never shows anything about hold timing or charge. */
UCLASS()
class MASTERJUMPER_API AMJHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	void DrawCentered(const FString& Text, float ScreenYFraction, float Scale, const FLinearColor& Color);
};
