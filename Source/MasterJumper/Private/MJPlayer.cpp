#include "MJPlayer.h"
#include "MJJumpStateMachine.h"
#include "MJDivePresentation.h"
#include "MJLevelController.h"
#include "MJObstacleStack.h"
#include "MJArtLibrary.h"
#include "MJLayout.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"

AMJPlayer::AMJPlayer()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;

	// Fixed POV: the pawn never follows controller rotation and there is no look input.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	Capsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
	Capsule->InitCapsuleSize(CapsuleRadius, StandingHalfHeight);
	Capsule->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);
	Capsule->SetGenerateOverlapEvents(true);
	Capsule->SetCanEverAffectNavigation(false);
	RootComponent = Capsule;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Capsule);
	Camera->SetRelativeLocation(FVector(0.f, 0.f, RunEyeOffset));
	Camera->bUsePawnControlRotation = false;
	Camera->SetFieldOfView(FieldOfView);

	ArmsRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ArmsRoot"));
	ArmsRoot->SetupAttachment(Camera);

	// Arms are pure presentation: no collision, so they can never block or trigger anything.
	ArmsMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ArmsMesh"));
	ArmsMesh->SetupAttachment(ArmsRoot);
	ArmsMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	ArmsMesh->SetGenerateOverlapEvents(false);
	ArmsMesh->SetCastShadow(false);
	ArmsMesh->SetVisibility(false);

	StateMachine = CreateDefaultSubobject<UMJJumpStateMachine>(TEXT("JumpStateMachine"));
	DivePresentation = CreateDefaultSubobject<UMJDivePresentation>(TEXT("DivePresentation"));
}

void AMJPlayer::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	SetupArms(/*bAllowTransient*/ GetWorld() && GetWorld()->IsGameWorld());
}

void AMJPlayer::BeginPlay()
{
	Super::BeginPlay();

	Capsule->SetCapsuleSize(CapsuleRadius, StandingHalfHeight);
	Camera->SetFieldOfView(FieldOfView);
	Camera->SetRelativeLocation(FVector(0.f, 0.f, RunEyeOffset));
	SetupArms(true);

	DivePresentation->ArmsRoot = ArmsRoot;
	DivePresentation->BindTo(StateMachine);
	DivePresentation->HideArms();

	SpawnFloorLocation = GetActorLocation() - FVector(0.f, 0.f, StandingHalfHeight + FloorGap);
	if (AMJLevelController* Controller = AMJLevelController::Get(this))
	{
		Controller->RegisterPlayer(this);
	}
	else
	{
		UE_LOG(LogMasterJumper, Warning, TEXT("No MJLevelController in the world; running without restart flow."));
		bRunEnabled = true;
	}
}

void AMJPlayer::SetLevelController(AMJLevelController* InController)
{
	LevelController = InController;
}

void AMJPlayer::SetupArms(bool bAllowTransient)
{
	if (UStaticMesh* Mesh = UMJArtLibrary::ResolveMesh(EMJArt::PovArms, bAllowTransient))
	{
		ArmsMesh->SetStaticMesh(Mesh);
		UMJArtLibrary::ApplyArtMaterials(ArmsMesh, EMJArt::PovArms);
	}
}

// ---------------------------------------------------------------- Input

void AMJPlayer::EnsureInputObjects()
{
	if (!JumpChargeAction)
	{
		JumpChargeAction = UMJArtLibrary::LoadOptional<UInputAction>(MJAssets::Input / TEXT("IA_JumpCharge"));
	}
	if (!JumpChargeAction)
	{
		JumpChargeAction = NewObject<UInputAction>(this, TEXT("IA_JumpCharge"));
		JumpChargeAction->ValueType = EInputActionValueType::Boolean;
	}
	if (!MappingContext)
	{
		MappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_MasterJumper"));
		MappingContext->MapKey(JumpChargeAction, EKeys::SpaceBar);
		MappingContext->MapKey(JumpChargeAction, EKeys::LeftMouseButton);
		MappingContext->MapKey(JumpChargeAction, EKeys::Gamepad_FaceButton_Bottom);
	}
}

void AMJPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	EnsureInputObjects();

	if (const APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (const ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				Subsystem->ClearAllMappings();
				Subsystem->AddMappingContext(MappingContext, 0);
			}
		}
	}

	if (UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		Input->BindAction(JumpChargeAction, ETriggerEvent::Started, this, &AMJPlayer::OnJumpStarted);
		Input->BindAction(JumpChargeAction, ETriggerEvent::Completed, this, &AMJPlayer::OnJumpCompleted);
	}
	else
	{
		UE_LOG(LogMasterJumper, Error, TEXT("Enhanced Input component missing - check Config/DefaultInput.ini."));
	}
}

void AMJPlayer::OnJumpStarted()
{
	PressJump();
}

void AMJPlayer::OnJumpCompleted()
{
	ReleaseJump();
}

void AMJPlayer::PressJump()
{
	const EMJJumpState State = GetJumpState();
	if (State == EMJJumpState::Succeeded)
	{
		if (AMJLevelController* Controller = LevelController.Get())
		{
			Controller->RequestRestart();
		}
		return;
	}
	// One jump per attempt: an early jump that lands short leaves you running into the pile.
	if (!bRunEnabled || bJumpSpent || bInAir || State != EMJJumpState::Running)
	{
		return;
	}
	if (StateMachine->TryTransition(EMJJumpState::Charging))
	{
		Charge.Begin(GetWorld()->GetTimeSeconds());
	}
}

void AMJPlayer::ReleaseJump()
{
	if (GetJumpState() != EMJJumpState::Charging)
	{
		return;
	}
	LastHeldSeconds = Charge.Commit(GetWorld()->GetTimeSeconds());
	bJumpSpent = true;
	StateMachine->TryTransition(EMJJumpState::Takeoff);
}

// ---------------------------------------------------------------- State / movement

EMJJumpState AMJPlayer::GetJumpState() const
{
	return StateMachine ? StateMachine->GetState() : EMJJumpState::Running;
}

void AMJPlayer::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const EMJJumpState State = GetJumpState();
	if (State == EMJJumpState::Failed || State == EMJJumpState::Succeeded)
	{
		return; // frozen
	}

	if (State == EMJJumpState::Takeoff && StateMachine->GetTimeInState() >= Charge.TakeoffDuration)
	{
		Launch();
	}

	if (bInAir)
	{
		TickInAir(DeltaSeconds);
	}
	else
	{
		TickGrounded(DeltaSeconds);
	}

	const EMJJumpState After = GetJumpState();
	if (After == EMJJumpState::Landing && StateMachine->GetTimeInState() >= Charge.LandingBlendTime)
	{
		StateMachine->TryTransition(EMJJumpState::Running);
	}
	else if (After == EMJJumpState::Airborne && StateMachine->GetTimeInState() > Charge.MaxAirborneSeconds)
	{
		ReportFail(EMJFailReason::AirborneTimeout);
	}

	const float KillZ = LevelController.IsValid() ? LevelController->KillZ : MJLayout::KillZ;
	if (GetActorLocation().Z < KillZ)
	{
		ReportFail(EMJFailReason::FellInGap);
	}

	UpdateCamera(DeltaSeconds);
}

void AMJPlayer::Launch()
{
	const FVector2D LaunchVelocity = Charge.EvaluateLaunch(LastHeldSeconds);
	UE_LOG(LogMasterJumper, Verbose, TEXT("[Jump] launch: held %.3fs -> strength %.2f"), LastHeldSeconds, Charge.EvaluateStrength(LastHeldSeconds));

	Velocity = FVector(LaunchVelocity.X, 0.f, LaunchVelocity.Y);
	SetCapsuleHalfHeightKeepCentre(DiveHalfHeight);
	bInAir = true;
	StateMachine->TryTransition(EMJJumpState::Airborne);
}

void AMJPlayer::TickGrounded(float DeltaSeconds)
{
	if (!bRunEnabled)
	{
		return;
	}

	FHitResult Hit;
	AddActorWorldOffset(FVector(RunSpeed * DeltaSeconds, 0.f, 0.f), true, &Hit);
	if (Hit.bBlockingHit)
	{
		HandleBlockingHit(Hit);
		return;
	}

	FHitResult Floor;
	if (FindFloor(Floor))
	{
		FVector Location = GetActorLocation();
		Location.Z = Floor.ImpactPoint.Z + Capsule->GetUnscaledCapsuleHalfHeight() + FloorGap;
		SetActorLocation(Location);
	}
	else
	{
		bInAir = true; // ran off an edge: fall with the jump gravity, the kill plane decides
		Velocity = FVector(RunSpeed, 0.f, 0.f);
	}
}

void AMJPlayer::TickInAir(float DeltaSeconds)
{
	Velocity.Z -= Charge.GetAirGravity() * DeltaSeconds;

	FHitResult Hit;
	AddActorWorldOffset(Velocity * DeltaSeconds, true, &Hit);
	if (Hit.bBlockingHit)
	{
		HandleBlockingHit(Hit);
	}
}

bool AMJPlayer::FindFloor(FHitResult& OutHit) const
{
	const FVector Start = GetActorLocation();
	const FVector End = Start - FVector(0.f, 0.f, Capsule->GetUnscaledCapsuleHalfHeight() + FloorGap + 25.f);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MJFindFloor), false, this);
	return GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, ECC_Visibility, Params) && OutHit.ImpactNormal.Z > 0.7f;
}

void AMJPlayer::HandleBlockingHit(const FHitResult& Hit)
{
	const AActor* Other = Hit.GetActor();
	if (Other && Other->IsA<AMJObstacleStack>())
	{
		ReportFail(EMJFailReason::HitObstacle);
	}
	else if (bInAir && Hit.ImpactNormal.Z > 0.7f)
	{
		Touchdown(Hit);
	}
	else if (Hit.ImpactNormal.Z < -0.7f)
	{
		ReportFail(EMJFailReason::HitCeiling);
	}
	else
	{
		ReportFail(EMJFailReason::HitWall);
	}
}

void AMJPlayer::Touchdown(const FHitResult& Hit)
{
	FHitResult Floor;
	if (!FindFloor(Floor))
	{
		// Clipped a ledge corner with the centre already past it: keep falling forward.
		Velocity.Z = FMath::Min(Velocity.Z, 0.f);
		return;
	}

	const float EyeWorldZ = Camera->GetComponentLocation().Z;
	bInAir = false;
	Velocity = FVector::ZeroVector;

	SetCapsuleHalfHeightKeepCentre(StandingHalfHeight);
	FVector Location = GetActorLocation();
	Location.Z = Floor.ImpactPoint.Z + StandingHalfHeight + FloorGap;
	SetActorLocation(Location);

	if (GetJumpState() == EMJJumpState::Airborne)
	{
		// Keep the eye where it was at impact, then rise back to running height during Landing.
		LandingStartEye = EyeWorldZ - Location.Z;
		Camera->SetRelativeLocation(FVector(0.f, 0.f, LandingStartEye));
		StateMachine->TryTransition(EMJJumpState::Landing);
	}
}

void AMJPlayer::SetCapsuleHalfHeightKeepCentre(float HalfHeight)
{
	Capsule->SetCapsuleHalfHeight(HalfHeight, /*bUpdateOverlaps*/ true);
}

void AMJPlayer::ReportFail(EMJFailReason Reason)
{
	if (AMJLevelController* Controller = LevelController.Get())
	{
		Controller->ReportFail(Reason);
	}
	else
	{
		EnterFailed();
		ResetToStart(SpawnFloorLocation);
	}
}

void AMJPlayer::EnterFailed()
{
	StateMachine->TryTransition(EMJJumpState::Failed);
	Velocity = FVector::ZeroVector;
}

void AMJPlayer::EnterSucceeded()
{
	StateMachine->TryTransition(EMJJumpState::Succeeded);
	Velocity = FVector::ZeroVector;
}

void AMJPlayer::ResetToStart(const FVector& FloorLocation)
{
	Charge.Reset();
	bInAir = false;
	bJumpSpent = false;
	Velocity = FVector::ZeroVector;
	LastHeldSeconds = 0.f;
	RunBobTime = 0.f;

	Capsule->SetCapsuleHalfHeight(StandingHalfHeight, false);
	SetActorLocationAndRotation(FloorLocation + FVector(0.f, 0.f, StandingHalfHeight + FloorGap), FRotator::ZeroRotator,
		false, nullptr, ETeleportType::TeleportPhysics);
	Camera->SetRelativeLocation(FVector(0.f, 0.f, RunEyeOffset));

	StateMachine->ForceReset();
	DivePresentation->HideArms();
}

// ---------------------------------------------------------------- Camera

void AMJPlayer::UpdateCamera(float DeltaSeconds)
{
	const float TimeInState = StateMachine->GetTimeInState();
	float Eye = RunEyeOffset;

	switch (GetJumpState())
	{
	case EMJJumpState::Running:
	case EMJJumpState::Charging:
		if (bRunEnabled && !bInAir)
		{
			RunBobTime += DeltaSeconds;
			Eye += RunBobAmplitude * FMath::Sin(RunBobTime * RunBobFrequency * 2.f * PI);
		}
		break;
	case EMJJumpState::Takeoff:
	{
		const float Alpha = FMath::Clamp(TimeInState / FMath::Max(Charge.TakeoffDuration, KINDA_SMALL_NUMBER), 0.f, 1.f);
		Eye = FMath::Lerp(RunEyeOffset, DiveEyeOffset, FMath::SmoothStep(0.f, 1.f, Alpha));
		break;
	}
	case EMJJumpState::Airborne:
		Eye = DiveEyeOffset;
		break;
	case EMJJumpState::Landing:
	{
		const float Alpha = FMath::Clamp(TimeInState / FMath::Max(Charge.LandingBlendTime, KINDA_SMALL_NUMBER), 0.f, 1.f);
		Eye = FMath::Lerp(LandingStartEye, RunEyeOffset, 1.f - FMath::Square(1.f - Alpha));
		break;
	}
	default:
		return;
	}

	Camera->SetRelativeLocation(FVector(0.f, 0.f, Eye));
}
