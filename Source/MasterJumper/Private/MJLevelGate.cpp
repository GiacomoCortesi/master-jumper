#include "MJLevelGate.h"
#include "MJLevelController.h"
#include "MJPlayer.h"
#include "Components/BoxComponent.h"

AMJLevelGate::AMJLevelGate()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Volume = CreateDefaultSubobject<UBoxComponent>(TEXT("Volume"));
	Volume->SetCollisionProfileName(TEXT("Trigger"));
	Volume->SetGenerateOverlapEvents(true);
	Volume->ShapeColor = FColor::Green;
	RootComponent = Volume;
}

void AMJLevelGate::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Volume->SetBoxExtent(FVector(Length * 0.5f, HalfWidth, Height * 0.5f));
}

void AMJLevelGate::BeginPlay()
{
	Super::BeginPlay();
	Volume->OnComponentBeginOverlap.AddUniqueDynamic(this, &AMJLevelGate::HandleBeginOverlap);
	Volume->OnComponentEndOverlap.AddUniqueDynamic(this, &AMJLevelGate::HandleEndOverlap);
}

void AMJLevelGate::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (AMJPlayer* Player = Cast<AMJPlayer>(OtherActor))
	{
		PlayerInside = Player;
		SetActorTickEnabled(true);
	}
}

void AMJLevelGate::HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor && OtherActor == PlayerInside.Get())
	{
		PlayerInside.Reset();
		SetActorTickEnabled(false);
	}
}

void AMJLevelGate::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const AMJPlayer* Player = PlayerInside.Get();
	if (!Player || !Player->IsGrounded())
	{
		return;
	}
	const EMJJumpState State = Player->GetJumpState();
	if (State != EMJJumpState::Landing && State != EMJJumpState::Running)
	{
		return;
	}
	// Centre must be over the gate (not just the capsule edge), so a touchdown on the stairwell lip does not count.
	const FVector Local = GetActorTransform().InverseTransformPosition(Player->GetActorLocation());
	if (FMath::Abs(Local.X) <= Length * 0.5f && FMath::Abs(Local.Y) <= HalfWidth)
	{
		if (AMJLevelController* Controller = AMJLevelController::Get(this))
		{
			Controller->ReportSuccess();
		}
	}
}
