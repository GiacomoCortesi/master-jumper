#include "MJHUD.h"
#include "MJLevelController.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"

namespace
{
	FString DescribeFail(EMJFailReason Reason)
	{
		switch (Reason)
		{
		case EMJFailReason::HitObstacle:     return TEXT("Into the pile.");
		case EMJFailReason::HitCeiling:      return TEXT("Bonked the ceiling.");
		case EMJFailReason::HitWall:         return TEXT("Hit the wall.");
		case EMJFailReason::FellInGap:       return TEXT("Down the stairwell.");
		case EMJFailReason::AirborneTimeout: return TEXT("Lost in the air.");
		default:                             return FString();
		}
	}
}

void AMJHUD::DrawHUD()
{
	Super::DrawHUD();
	const AMJLevelController* Controller = AMJLevelController::Get(this);
	if (!Controller || !Canvas)
	{
		return;
	}

	switch (Controller->GetOutcome())
	{
	case EMJOutcome::Failed:
		DrawCentered(TEXT("WIPEOUT"), 0.42f, 3.f, FLinearColor(1.f, 0.25f, 0.2f));
		DrawCentered(DescribeFail(Controller->GetLastFailReason()), 0.52f, 1.5f, FLinearColor::White);
		break;
	case EMJOutcome::Succeeded:
		DrawCentered(TEXT("CLEARED!"), 0.42f, 3.f, FLinearColor(0.3f, 1.f, 0.4f));
		DrawCentered(TEXT("Press Space / Click to run again"), 0.52f, 1.5f, FLinearColor::White);
		break;
	default:
		break;
	}
}

void AMJHUD::DrawCentered(const FString& Text, float ScreenYFraction, float Scale, const FLinearColor& Color)
{
	if (Text.IsEmpty())
	{
		return;
	}
	UFont* Font = GEngine ? GEngine->GetLargeFont() : nullptr;
	float Width = 0.f, Height = 0.f;
	GetTextSize(Text, Width, Height, Font, Scale);
	const float X = (Canvas->ClipX - Width) * 0.5f;
	const float Y = Canvas->ClipY * ScreenYFraction - Height * 0.5f;
	DrawText(Text, FLinearColor::Black, X + 2.f, Y + 2.f, Font, Scale);
	DrawText(Text, Color, X, Y, Font, Scale);
}
