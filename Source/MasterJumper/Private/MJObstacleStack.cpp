#include "MJObstacleStack.h"
#include "MJArtLibrary.h"
#include "MJLevelController.h"
#include "MJPlayer.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"

AMJObstacleStack::AMJObstacleStack()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	BlockingVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("BlockingVolume"));
	BlockingVolume->SetupAttachment(RootComponent);
	BlockingVolume->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	BlockingVolume->SetGenerateOverlapEvents(false);

	FailTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("FailTrigger"));
	FailTrigger->SetupAttachment(RootComponent);
	FailTrigger->SetCollisionProfileName(TEXT("Trigger"));
	FailTrigger->SetGenerateOverlapEvents(true);

	auto MakeVisual = [this](const TCHAR* Name)
	{
		UInstancedStaticMeshComponent* Component = CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
		Component->SetupAttachment(RootComponent);
		Component->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		Component->SetGenerateOverlapEvents(false);
		return Component;
	};
	Chairs = MakeVisual(TEXT("Chairs"));
	Computers = MakeVisual(TEXT("Computers"));
}

void AMJObstacleStack::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Build(GetWorld() && GetWorld()->IsGameWorld());
}

void AMJObstacleStack::BeginPlay()
{
	Super::BeginPlay();
	Build(true);
	FailTrigger->OnComponentBeginOverlap.AddUniqueDynamic(this, &AMJObstacleStack::HandleFailOverlap);
}

void AMJObstacleStack::Build(bool bAllowTransient)
{
	const FVector Extent(Depth * 0.5f, HalfWidth, Height * 0.5f);
	BlockingVolume->SetBoxExtent(Extent);
	BlockingVolume->SetRelativeLocation(FVector(0.f, 0.f, Height * 0.5f));
	FailTrigger->SetBoxExtent(Extent + FVector(1.f));
	FailTrigger->SetRelativeLocation(FVector(0.f, 0.f, Height * 0.5f));

	Chairs->ClearInstances();
	Computers->ClearInstances();
	UStaticMesh* ChairMesh = UMJArtLibrary::ResolveMesh(EMJArt::OfficeChair, bAllowTransient);
	UStaticMesh* ComputerMesh = UMJArtLibrary::ResolveMesh(EMJArt::OfficeComputer, bAllowTransient);
	Chairs->SetStaticMesh(ChairMesh);
	Computers->SetStaticMesh(ComputerMesh);
	UMJArtLibrary::ApplyArtMaterials(Chairs, EMJArt::OfficeChair);
	UMJArtLibrary::ApplyArtMaterials(Computers, EMJArt::OfficeComputer);

	// Hand-placed pile tuned for the default 220 x 400 x 165 footprint; positions scale with the box if it is edited.
	const float SX = Depth / MJLayout::ObstacleDepth;
	const float SY = HalfWidth / MJLayout::CorridorHalfWidth;
	const float SZ = Height / MJLayout::ObstacleHeight;
	FRandomStream Stream(PileSeed);
	auto Jitter = [&Stream](float Amount) { return Stream.FRandRange(-Amount, Amount); };

	// Layer 1: chairs on the floor, 2 rows x 5.
	for (const float X : { -62.f, 55.f })
	{
		for (const float Y : { -150.f, -75.f, 0.f, 75.f, 150.f })
		{
			const FVector Location(X * SX + Jitter(5.f), Y * SY + Jitter(6.f), 0.f);
			Chairs->AddInstance(FTransform(FRotator(0.f, Stream.FRandRange(0.f, 360.f), 0.f), Location));
		}
	}

	// Layer 2: computers dumped on the seats.
	for (const float X : { -58.f, 48.f })
	{
		for (const float Y : { -140.f, -47.f, 47.f, 140.f })
		{
			const float Yaw = (Stream.FRand() < 0.5f ? 0.f : 180.f) + Jitter(50.f);
			const FVector Location(X * SX + Jitter(4.f), Y * SY + Jitter(5.f), 57.f * SZ);
			Computers->AddInstance(FTransform(FRotator(Jitter(5.f), Yaw, Jitter(6.f)), Location));
		}
	}

	// Layer 3: chairs upside-down on top; their wheels define the pile's top (= Height).
	const FVector2D TopChairs[] = { FVector2D(-30.f, -115.f), FVector2D(25.f, 5.f), FVector2D(-15.f, 120.f) };
	for (const FVector2D& XY : TopChairs)
	{
		const FVector Location(XY.X * SX, XY.Y * SY, Height);
		Chairs->AddInstance(FTransform(FRotator(0.f, Stream.FRandRange(0.f, 360.f), 180.f), Location, FVector(1.f, 1.f, SZ)));
	}
}

void AMJObstacleStack::HandleFailOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor->IsA<AMJPlayer>())
	{
		if (AMJLevelController* Controller = AMJLevelController::Get(this))
		{
			Controller->ReportFail(EMJFailReason::HitObstacle);
		}
	}
}
