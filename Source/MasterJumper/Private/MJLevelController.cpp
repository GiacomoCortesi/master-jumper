#include "MJLevelController.h"
#include "MJPlayer.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

AMJLevelController::AMJLevelController()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

AMJLevelController* AMJLevelController::Get(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World)
	{
		return nullptr;
	}
	if (TActorIterator<AMJLevelController> It(World); It)
	{
		return *It;
	}
	return nullptr;
}

void AMJLevelController::RegisterPlayer(AMJPlayer* InPlayer)
{
	Player = InPlayer;
	if (InPlayer)
	{
		InPlayer->SetLevelController(this);
		StartAttempt();
	}
}

void AMJLevelController::StartAttempt()
{
	AMJPlayer* P = Player.Get();
	if (!P)
	{
		return;
	}
	++Attempt;
	Outcome = EMJOutcome::InProgress;
	LastFailReason = EMJFailReason::None;
	UE_LOG(LogMasterJumper, Log, TEXT("[Level] attempt %d"), Attempt);

	P->ResetToStart(PlayerStartLocation);
	P->SetRunEnabled(false);
	Fade(1.f, 0.f);
	GetWorldTimerManager().SetTimer(Timer, this, &AMJLevelController::BeginRun, FMath::Max(RunStartDelay, 0.01f), false);
}

void AMJLevelController::BeginRun()
{
	if (AMJPlayer* P = Player.Get())
	{
		P->SetRunEnabled(true);
	}
}

void AMJLevelController::ReportFail(EMJFailReason Reason)
{
	if (Outcome != EMJOutcome::InProgress)
	{
		return;
	}
	Outcome = EMJOutcome::Failed;
	LastFailReason = Reason;
	UE_LOG(LogMasterJumper, Log, TEXT("[Level] FAIL: %s"), *UEnum::GetValueAsString(Reason));

	if (AMJPlayer* P = Player.Get())
	{
		P->EnterFailed();
	}
	Fade(0.f, 1.f);
	GetWorldTimerManager().SetTimer(Timer, this, &AMJLevelController::StartAttempt, FMath::Max(FailRestartDelay, 0.01f), false);
}

void AMJLevelController::ReportSuccess()
{
	if (Outcome != EMJOutcome::InProgress)
	{
		return;
	}
	Outcome = EMJOutcome::Succeeded;
	UE_LOG(LogMasterJumper, Log, TEXT("[Level] SUCCESS on attempt %d"), Attempt);
	GetWorldTimerManager().ClearTimer(Timer);
	if (AMJPlayer* P = Player.Get())
	{
		P->EnterSucceeded();
	}
}

void AMJLevelController::RequestRestart()
{
	if (Outcome == EMJOutcome::Succeeded)
	{
		StartAttempt();
	}
}

void AMJLevelController::Fade(float From, float To)
{
	if (APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		CameraManager->StartCameraFade(From, To, FadeTime, FLinearColor::Black, false, /*bHoldWhenFinished*/ true);
	}
}
