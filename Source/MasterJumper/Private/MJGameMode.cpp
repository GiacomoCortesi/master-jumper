#include "MJGameMode.h"
#include "MJArtLibrary.h"
#include "MJCorridor.h"
#include "MJHUD.h"
#include "MJLayout.h"
#include "MJLevelController.h"
#include "MJLevelGate.h"
#include "MJObstacleStack.h"
#include "MJPlayer.h"
#include "MJPlayerController.h"
#include "Engine/World.h"
#include "EngineUtils.h"

namespace
{
	/** Loads BP_MJ_<Name>'s generated class if the Blueprint exists and derives from Base. */
	UClass* LoadBlueprintClass(const TCHAR* Name, UClass* Base)
	{
		const FString PackagePath = MJAssets::Blueprints / Name;
		if (!FPackageName::DoesPackageExist(PackagePath))
		{
			return nullptr;
		}
		UClass* Class = LoadObject<UClass>(nullptr, *FString::Printf(TEXT("%s.%s_C"), *PackagePath, Name));
		return (Class && Class->IsChildOf(Base)) ? Class : nullptr;
	}

	template <typename T>
	T* FindOrSpawn(UWorld* World, const TCHAR* BlueprintName, const FTransform& Transform)
	{
		for (TActorIterator<T> It(World); It; ++It)
		{
			return *It;
		}
		UClass* Class = BlueprintName ? LoadBlueprintClass(BlueprintName, T::StaticClass()) : nullptr;
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		return World->SpawnActor<T>(Class ? Class : T::StaticClass(), Transform, Params);
	}
}

AMJGameMode::AMJGameMode()
{
	DefaultPawnClass = AMJPlayer::StaticClass();
	PlayerControllerClass = AMJPlayerController::StaticClass();
	HUDClass = AMJHUD::StaticClass();
}

void AMJGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	if (UClass* PlayerBlueprint = LoadBlueprintClass(TEXT("BP_MJ_Player"), AMJPlayer::StaticClass()))
	{
		DefaultPawnClass = PlayerBlueprint;
	}
}

void AMJGameMode::StartPlay()
{
	EnsureLevelActors();
	Super::StartPlay();
}

void AMJGameMode::EnsureLevelActors()
{
	UWorld* World = GetWorld();
	if (bLevelActorsEnsured || !World)
	{
		return;
	}
	bLevelActorsEnsured = true;

	using namespace MJLayout;
	FindOrSpawn<AMJCorridor>(World, nullptr, FTransform::Identity);
	FindOrSpawn<AMJObstacleStack>(World, TEXT("BP_MJ_ObstacleStack"), FTransform(FVector(ObstacleFrontX + ObstacleDepth * 0.5f, 0.f, 0.f)));
	FindOrSpawn<AMJLevelGate>(World, nullptr, FTransform(FVector((GateStartX + GateEndX) * 0.5f, 0.f, GateHeight * 0.5f)));
	FindOrSpawn<AMJLevelController>(World, TEXT("BP_MJ_LevelController"), FTransform::Identity);
}

void AMJGameMode::RestartPlayer(AController* NewPlayer)
{
	if (!NewPlayer || NewPlayer->IsPendingKillPending())
	{
		return;
	}
	EnsureLevelActors();

	FVector Start(MJLayout::PlayerStartX, 0.f, 0.f);
	if (const AMJLevelController* Controller = AMJLevelController::Get(this))
	{
		Start = Controller->PlayerStartLocation;
	}
	const AMJPlayer* PawnDefaults = GetDefault<AMJPlayer>();
	Start.Z += PawnDefaults->StandingHalfHeight + PawnDefaults->FloorGap;
	RestartPlayerAtTransform(NewPlayer, FTransform(FRotator::ZeroRotator, Start));
}
