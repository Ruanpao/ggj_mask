// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemies/Enemy.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Sight.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "AIController.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Components/SphereComponent.h"

// Sets default values
AEnemy::AEnemy()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Create collision component and set as root
	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComp"));
	CollisionComp->InitSphereRadius(50.0f);
	CollisionComp->SetCollisionProfileName(TEXT("Pawn"));
	RootComponent = CollisionComp;

	// Configure collision for navigation
	CollisionComp->SetCanEverAffectNavigation(true);
	CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComp->SetCollisionObjectType(ECollisionChannel::ECC_Pawn);

	// Create perception component
	PerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComp"));

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));

	// Configure sight defaults
	if (SightConfig)
	{
		SightConfig->SightRadius = 2000.0f;
		SightConfig->LoseSightRadius = 2200.0f;
		SightConfig->PeripheralVisionAngleDegrees = 45.0f;
		SightConfig->SetMaxAge(5.0f);
		SightConfig->DetectionByAffiliation.bDetectEnemies = true;
		SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
		SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	}

	if (PerceptionComponent && SightConfig)
	{
		PerceptionComponent->ConfigureSense(*SightConfig);
		PerceptionComponent->SetDominantSense(UAISense_Sight::StaticClass());
		PerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AEnemy::OnTargetPerceptionUpdated);
	}

	// Create movement component and attach
	MovementComp = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("MovementComp"));
	if (MovementComp)
	{
		MovementComp->UpdatedComponent = CollisionComp;
		MovementComp->MaxSpeed = 600.0f;
	}

	// Create blackboard/behavior components removed — controller will manage them
}

// Called when the game starts or when spawned
void AEnemy::BeginPlay()
{
	Super::BeginPlay();
	
	// Initialize BehaviorTree and Blackboard using the Pawn's Controller (AAIController)
	if (BehaviorTree)
	{
		AAIController* AICon = Cast<AAIController>(GetController());
		if (!AICon)
		{
			// If controller not yet possessed, try to spawn logic later or log
			return;
		}

		if (BehaviorTree->BlackboardAsset)
		{
			UBlackboardComponent* BBComp = AICon->GetBlackboardComponent();
			AICon->UseBlackboard(BehaviorTree->BlackboardAsset, BBComp);
		}

		AICon->RunBehaviorTree(BehaviorTree);
	}

}

// Called every frame
void AEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Simple movement towards TargetActor from controller blackboard (fallback if MoveTo doesn't move pawn)
	AAIController* AICon = Cast<AAIController>(GetController());
	if (!AICon) return;

	UBlackboardComponent* BBComp = AICon->GetBlackboardComponent();
	if (!BBComp) return;

	const FName TargetActorKey = TEXT("TargetActor");
	AActor* Target = Cast<AActor>(BBComp->GetValueAsObject(TargetActorKey));
	if (!Target) return;

	FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
	float DistSq = ToTarget.SizeSquared();
	const float AcceptanceRadius = 100.0f; // tweak as needed
	if (DistSq > FMath::Square(AcceptanceRadius))
	{
		FVector Dir = ToTarget.GetSafeNormal2D();
		if (!Dir.IsNearlyZero())
		{
			// Use Pawn movement input which UFloatingPawnMovement will consume
			AddMovementInput(Dir, 1.0f);
			// Optional: rotate to face movement
			FRotator NewRot = Dir.Rotation();
			SetActorRotation(FRotator(0.0f, NewRot.Yaw, 0.0f));
		}
	}
}

// Called to bind functionality to input
void AEnemy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void AEnemy::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	AAIController* AICon = Cast<AAIController>(GetController());
	if (!AICon) return;

	UBlackboardComponent* BBComp = AICon->GetBlackboardComponent();
	if (!BBComp) return;

	// Keys used in blackboard. Assumption: Blackboard has these keys defined.
	const FName HasTargetKey = TEXT("HasTarget");
	const FName TargetActorKey = TEXT("TargetActor"); // change this name if your blackboard uses a different key

	if (Stimulus.WasSuccessfullySensed())
	{
		// Set boolean and actor reference when sensed
		BBComp->SetValueAsBool(HasTargetKey, true);
		BBComp->SetValueAsObject(TargetActorKey, Actor);
	}
	else
	{
		// Only clear the actor key if the blackboard currently references this actor
		UObject* CurrentTarget = BBComp->GetValueAsObject(TargetActorKey);
		if (CurrentTarget == Actor)
		{
			BBComp->ClearValue(TargetActorKey);
			BBComp->SetValueAsBool(HasTargetKey, false);
		}
		// If CurrentTarget != Actor, another target is already set; don't overwrite it here.
	}
}
