// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemies/Enemy.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Sight.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Components/SphereComponent.h"
#include "MonitorDoor.h" // include MonitorDoor so we can detect class in traces
#include "Engine/World.h"
#include "ggj_mask/ggj_maskCharacter.h" // player character class
#include "DrawDebugHelpers.h"
#include "Enemies/PatrolSpline.h"
#include "Navigation/PathFollowingComponent.h"
#include "Engine/Engine.h"
#include "NavigationSystem.h"
#include "NavigationSystemTypes.h"

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
		SightConfig->SightRadius = 500.0f;
		SightConfig->LoseSightRadius = 500.0f;
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

	// Initialize patrol: set current index to nearest patrol point if any
	if (PatrolPoints.Num() > 0)
	{
		float BestDistSq = FLT_MAX;
		int32 BestIndex = 0;
		FVector MyLoc = GetActorLocation();
		for (int32 i = 0; i < PatrolPoints.Num(); ++i)
		{
			AActor* P = PatrolPoints[i];
			if (!P) continue;
			float DistSq = FVector::DistSquared(MyLoc, P->GetActorLocation());
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				BestIndex = i;
			}
		}
		CurrentPatrolIndex = BestIndex;
	}

	// If spline patrol is enabled, start it and log state
	if (bUseSplinePatrol)
	{
		UE_LOG(LogTemp, Log, TEXT("BeginPlay: %s bUseSplinePatrol=true PatrolSpline=%s"), *GetNameSafe(this), *GetNameSafe(PatrolSpline));
		if (PatrolSpline)
		{
			UE_LOG(LogTemp, Log, TEXT("BeginPlay: Spline has %d points"), PatrolSpline->GetNumPoints());
			if (PatrolSpline->GetNumPoints() > 0)
			{
				FVector P0 = PatrolSpline->GetPointLocation(0);
				UE_LOG(LogTemp, Log, TEXT("BeginPlay: Spline point[0]=%s"), *P0.ToCompactString());
			}
			StartSplinePatrol();
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("BeginPlay: bUseSplinePatrol is true but PatrolSpline is null on %s"), *GetNameSafe(this));
		}
	}
}

// Helper: perform a line trace from this enemy to Target and check if a MonitorDoor blocks vision
bool AEnemy::IsMonitorDoorBetween(AActor* Target, FHitResult& OutHit) const
{
	if (!Target) return false;

	UWorld* World = GetWorld();
	if (!World) return false;

	FVector Start = GetActorLocation();
	FVector End = Target->GetActorLocation();

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(Target);

	// Trace only against WorldStatic and WorldDynamic so we can hit doors (which are actors)
	bool bHit = World->LineTraceSingleByChannel(OutHit, Start, End, ECC_Visibility, Params);
	if (!bHit) return false;

	// If the hit actor is a MonitorDoor (or contains a MonitorDoor component), treat as blocking
	if (OutHit.GetActor() && OutHit.GetActor()->IsA(AMonitorDoor::StaticClass()))
	{
		return true;
	}

	return false;
}

// Called every frame
void AEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UWorld* World = GetWorld();
	if (!World) return;

	FVector Eye = GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);
	float SightRadius = 2000.0f;
	float PeripheralAngle = 45.0f;
	if (SightConfig)
	{
		SightRadius = SightConfig->SightRadius;
		PeripheralAngle = SightConfig->PeripheralVisionAngleDegrees;
	}

	FVector Forward = GetActorForwardVector();
	DrawDebugLine(World, Eye, Eye + Forward * SightRadius, FColor::Green, false, 0.0f, 0, 1.0f);
	FVector LeftDir = Forward.RotateAngleAxis(-PeripheralAngle, FVector::UpVector).GetSafeNormal();
	FVector RightDir = Forward.RotateAngleAxis(PeripheralAngle, FVector::UpVector).GetSafeNormal();
	DrawDebugLine(World, Eye, Eye + LeftDir * SightRadius, FColor::Yellow, false, 0.0f, 0, 1.0f);
	DrawDebugLine(World, Eye, Eye + RightDir * SightRadius, FColor::Yellow, false, 0.0f, 0, 1.0f);
	DrawDebugSphere(World, Eye, SightRadius, 24, FColor::Blue, false, 0.0f, 0, 0.5f);

	// Acquire controller and blackboard
	AAIController* AICon = Cast<AAIController>(GetController());
	if (!AICon) return;

	UBlackboardComponent* BBComp = AICon->GetBlackboardComponent();
	if (!BBComp) return;

	// Cache path following component for logging and checks
	auto PF = AICon->GetPathFollowingComponent();

	// If spline patrol is enabled and we DO NOT have a target, perform simple Tick-driven patrol
	if (bUseSplinePatrol && PatrolSpline)
	{
		bool bHasTargetBB = BBComp->GetValueAsBool(TEXT("HasTarget"));
		int32 NumPoints = PatrolSpline->GetNumPoints();
		UE_LOG(LogTemp, Verbose, TEXT("SplineTick: Enemy=%s bUseSpline=%d NumPoints=%d CurrIdx=%d HasTarget=%d"), *GetNameSafe(this), (int)bUseSplinePatrol, NumPoints, SplineCurrentIndex, (int)bHasTargetBB);

		if (!bHasTargetBB && NumPoints > 0)
		{
			// Ensure current index is valid
			SplineCurrentIndex = FMath::Clamp(SplineCurrentIndex, 0, NumPoints - 1);

			FVector TargetLoc = GetSplinePointLocation(SplineCurrentIndex);
			FVector ActorLoc = GetActorLocation();
			// Use world-space spline point as MoveTarget (no projection)
			FVector MoveTarget = TargetLoc;

			// Cache path end for debug (do not use for arrival decision)
			FVector PathEnd = MoveTarget;
			if (PF && PF->GetPath())
			{
				const auto& Points = PF->GetPath()->GetPathPoints();
				if (Points.Num() > 0)
				{
					PathEnd = Points.Last().Location;
				}
			}

			// Determine whether the controller currently has an active path request (moving)
			bool bMoving = false;
			if (PF)
			{
				bMoving = (PF->GetStatus() == EPathFollowingStatus::Moving);
			}

			// If we're moving (and not chasing a target), face the movement direction
			if (bMoving && !bHasTargetBB)
			{
				FVector MoveVel = FVector::ZeroVector;
				if (MovementComp)
				{
					MoveVel = MovementComp->Velocity;
				}
				// fallback: if MovementComp has no velocity, derive from next path point
				if (MoveVel.IsNearlyZero() && PF && PF->GetPath() && PF->GetPath()->GetPathPoints().Num() > 0)
				{
					const auto& Pts = PF->GetPath()->GetPathPoints();
					// look for first point that differs from actor location
					for (int32 i = 0; i < Pts.Num(); ++i)
					{
						FVector DirCandidate = (Pts[i].Location - ActorLoc);
						DirCandidate.Z = 0.0f;
						if (!DirCandidate.IsNearlyZero()) { MoveVel = DirCandidate.GetSafeNormal() * 1.0f; break; }
					}
				}
				// Apply facing if we have movement direction
				FVector MoveDir2D = FVector(MoveVel.X, MoveVel.Y, 0.0f);
				if (!MoveDir2D.IsNearlyZero())
				{
					FRotator CurrentRot = GetActorRotation();
					FRotator DesiredRot = MoveDir2D.Rotation();
					float YawDiff = FRotator::NormalizeAxis(DesiredRot.Yaw - CurrentRot.Yaw);
					float MaxDelta = RotationSpeed * DeltaTime; // degrees allowed this frame
					float Clamped = FMath::Clamp(YawDiff, -MaxDelta, MaxDelta);
					float NewYaw = CurrentRot.Yaw + Clamped;
					SetActorRotation(FRotator(0.0f, NewYaw, 0.0f));
				}
			}

			// Arrival decision uses straight-line distance to the MoveTarget (world-space spline point)
			float Dist = FVector::Dist(ActorLoc, MoveTarget); // linear distance in cm
			float Accept = SplinePatrolAcceptanceRadius; // linear acceptance in cm

			UE_LOG(LogTemp, Log, TEXT("SplineTick: Enemy=%s idx=%d splineTarget=%s actor=%s moveTarget=%s pathEnd=%s distToMoveTarget=%.2fcm accept=%.2fcm"),
				*GetNameSafe(this), SplineCurrentIndex, *TargetLoc.ToCompactString(), *ActorLoc.ToCompactString(), *MoveTarget.ToCompactString(), *PathEnd.ToCompactString(), Dist, Accept);
			DrawDebugSphere(World, TargetLoc, 32.0f, 8, FColor::Purple, false, 0.1f);
			DrawDebugSphere(World, MoveTarget, 20.0f, 6, FColor::Orange, false, 0.1f);
			DrawDebugSphere(World, PathEnd, 18.0f, 6, FColor::Blue, false, 0.1f);

			if (Dist <= Accept)
			{
				// Arrived -> advance and immediately request next MoveTo
				int32 Old = SplineCurrentIndex;
				int32 NewIdx = AdvanceSplineIndex();
				FVector NextLoc = GetSplinePointLocation(SplineCurrentIndex);
				FVector UseNext = NextLoc; // world-space next point
				UE_LOG(LogTemp, Log, TEXT("SplineTick: Arrived at MoveTarget. Advancing %d -> %d actor=%s splineTarget=%s nextTarget=%s dist=%.2fcm accept=%.2fcm"),
					Old, NewIdx, *ActorLoc.ToCompactString(), *TargetLoc.ToCompactString(), *NextLoc.ToCompactString(), Dist, Accept);
				EPathFollowingRequestResult::Type MoveRes = AICon->MoveToLocation(UseNext, SplinePatrolAcceptanceRadius-100);
				UE_LOG(LogTemp, Log, TEXT("SplineTick: MoveTo requested nextTarget=%s result=%d"), *UseNext.ToCompactString(), (int)MoveRes);
				if (PF && PF->GetPath())
				{
					const auto& Points = PF->GetPath()->GetPathPoints();
					if (Points.Num() > 0)
					{
						FVector PathEnd2 = Points.Last().Location;
						UE_LOG(LogTemp, Verbose, TEXT("SplineTick: Path end point = %s (dist to actor=%.2fcm)"), *PathEnd2.ToCompactString(), FVector::Dist(ActorLoc, PathEnd2));
						DrawDebugSphere(World, PathEnd2, 20.0f, 6, FColor::Blue, false, 1.0f);
					}
				}
				if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Green, FString::Printf(TEXT("Spline: advanced to %d"), NewIdx));
				return; // patrol action taken this tick
			}
			else
			{
				// Ensure we have an active move request toward MoveTarget
				bMoving = false;
				if (PF)
				{
					bMoving = (PF->GetStatus() == EPathFollowingStatus::Moving);
				}
                if (!bMoving)
                {
                    UE_LOG(LogTemp, Warning, TEXT("SplineTick: Re-request MoveTo. Actor=%s splineTarget=%s moveTarget=%s pathEnd=%s distToMoveTarget=%.2fcm accept=%.2fcm"), *ActorLoc.ToCompactString(), *TargetLoc.ToCompactString(), *MoveTarget.ToCompactString(), *PathEnd.ToCompactString(), Dist, Accept);
                    EPathFollowingRequestResult::Type MoveRes = AICon->MoveToLocation(MoveTarget, SplinePatrolAcceptanceRadius-100);
                    UE_LOG(LogTemp, Warning, TEXT("SplineTick: MoveTo requested target=%s res=%d"), *MoveTarget.ToCompactString(), (int)MoveRes);
                    if (PF && PF->GetPath())
                    {
                        const auto& Points = PF->GetPath()->GetPathPoints();
                        if (Points.Num() > 0)
                        {
                            FVector PathEnd3 = Points.Last().Location;
                            UE_LOG(LogTemp, Verbose, TEXT("SplineTick: Path end point = %s (dist to actor=%.2fcm)"), *PathEnd3.ToCompactString(), FVector::Dist(ActorLoc, PathEnd3));
                            DrawDebugSphere(World, PathEnd3, 20.0f, 6, FColor::Blue, false, 1.0f);
                        }
                    }
                    if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Yellow, FString::Printf(TEXT("Spline: MoveTo idx=%d res=%d"), SplineCurrentIndex, (int)MoveRes));
                }
				return; // patrol ensured this tick
			}
		}
	}

	// If we reach here, either not using spline patrol or BehaviorTree has a target — do facing logic below
	const FName TargetActorKey = TEXT("TargetActor");
	const FName HasTargetKey = TEXT("HasTarget");
	const FName IsBlockedKey = TEXT("IsBlockedByDoor");

	AActor* Target = Cast<AActor>(BBComp->GetValueAsObject(TargetActorKey));
	bool bHasTarget = BBComp->GetValueAsBool(HasTargetKey);
	bool bBlocked = BBComp->GetValueAsBool(IsBlockedKey);

	if (bHasTarget && Target && !bBlocked)
	{
		FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
		FVector ToTargetXY = FVector(ToTarget.X, ToTarget.Y, 0.0f);
		if (!ToTargetXY.IsNearlyZero())
		{
			FRotator CurrentRot = GetActorRotation();
			FRotator DesiredRot = ToTargetXY.Rotation();
			float YawDiff = FRotator::NormalizeAxis(DesiredRot.Yaw - CurrentRot.Yaw);
			float MaxDelta = RotationSpeed * DeltaTime; // degrees allowed this frame
			float Clamped = FMath::Clamp(YawDiff, -MaxDelta, MaxDelta);
			float NewYaw = CurrentRot.Yaw + Clamped;
			SetActorRotation(FRotator(0.0f, NewYaw, 0.0f));
		}

		DrawDebugLine(World, Eye, Target->GetActorLocation(), FColor::Red, false, 0.0f, 0, 2.0f);
	}
}

// Called to bind functionality to input
void AEnemy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void AEnemy::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	// Filter out MonitorDoor actors from being treated as sensed targets
	if (Actor && Actor->IsA(AMonitorDoor::StaticClass()))
	{
		// Ignore MonitorDoor perception updates entirely
		return;
	}

	// Only treat player character as a valid TargetActor
	if (!Actor || !Actor->IsA(Aggj_maskCharacter::StaticClass()))
	{
		// Not the player -> ignore for target selection
		return;
	}

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

// Patrol helpers
AActor* AEnemy::GetNearestPatrolPoint() const
{
	if (PatrolPoints.Num() == 0) return nullptr;
	float BestDistSq = FLT_MAX;
	AActor* Best = nullptr;
	FVector MyLoc = GetActorLocation();
	for (AActor* P : PatrolPoints)
	{
		if (!P) continue;
		float DistSq = FVector::DistSquared(MyLoc, P->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Best = P;
		}
	}
	return Best;
}

AActor* AEnemy::GetCurrentPatrolPoint() const
{
	if (PatrolPoints.IsValidIndex(CurrentPatrolIndex))
	{
		return PatrolPoints[CurrentPatrolIndex];
	}
	return nullptr;
}

AActor* AEnemy::GetNextPatrolPoint()
{
	if (PatrolPoints.Num() == 0) return nullptr;
	CurrentPatrolIndex = (CurrentPatrolIndex + 1) % PatrolPoints.Num();
	return GetCurrentPatrolPoint();
}

int32 AEnemy::GetCurrentPatrolIndex() const
{
	return CurrentPatrolIndex;
}

int32 AEnemy::GetIndexAfterCurrent() const
{
	int32 Count = PatrolPoints.Num();
	if (Count == 0) return INDEX_NONE;
	if (CurrentPatrolIndex < 0) return 0;
	int32 Next = CurrentPatrolIndex + 1;
	if (Next >= Count) Next = 0;
	return Next;
}

int32 AEnemy::GetNearestPatrolIndex() const
{
	if (PatrolPoints.Num() == 0) return INDEX_NONE;
	int32 BestIndex = INDEX_NONE;
	float BestDistSq = FLT_MAX;
	FVector MyLoc = GetActorLocation();
	for (int32 i = 0; i < PatrolPoints.Num(); ++i)
	{
		AActor* P = PatrolPoints[i];
		if (!P) continue;
		float DistSq = FVector::DistSquared(MyLoc, P->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			BestIndex = i;
		}
	}
	return BestIndex;
}

void AEnemy::SetCurrentPatrolIndex(int32 NewIndex)
{
	if (PatrolPoints.Num() == 0)
	{
		CurrentPatrolIndex = 0;
		return;
	}
	if (NewIndex < 0)
		NewIndex = 0;
	if (NewIndex >= PatrolPoints.Num())
		NewIndex = NewIndex % PatrolPoints.Num();
	CurrentPatrolIndex = NewIndex;
}

void AEnemy::SetLastPatrolIndex(int32 Index)
{
	LastPatrolIndex = Index;
}

int32 AEnemy::GetLastPatrolIndex() const
{
	return LastPatrolIndex;
}

bool AEnemy::MoveToCurrentPatrolPoint()
{
	AActor* Target = GetCurrentPatrolPoint();
	if (!Target) return false;

	AAIController* AICon = Cast<AAIController>(GetController());
	if (!AICon) return false;

	AICon->MoveToActor(Target, PatrolAcceptanceRadius-100);
	return true;
}

bool AEnemy::MoveToNextPatrolPoint()
{
	AActor* Next = GetNextPatrolPoint();
	if (!Next) return false;

	AAIController* AICon = Cast<AAIController>(GetController());
	if (!AICon) return false;

	AICon->MoveToActor(Next, PatrolAcceptanceRadius-100);
	return true;
}

// Spline patrol methods

void AEnemy::StartSplinePatrol()
{
	if (!PatrolSpline) { UE_LOG(LogTemp, Warning, TEXT("StartSplinePatrol called but PatrolSpline is null on %s"), *GetNameSafe(this)); return; }
	SplineCurrentIndex = PatrolSpline->GetClosestPointIndex(GetActorLocation());
	int32 Num = PatrolSpline->GetNumPoints();
	FVector ActorLoc = GetActorLocation();
	FVector Target = GetSplinePointLocation(SplineCurrentIndex);
	UE_LOG(LogTemp, Log, TEXT("StartSplinePatrol: %s closest index=%d NumPoints=%d actor=%s splineTarget=%s"), *GetNameSafe(this), SplineCurrentIndex, Num, *ActorLoc.ToCompactString(), *Target.ToCompactString());
	// request move to current spline point
	AAIController* AICon = Cast<AAIController>(GetController());
	if (!AICon) { UE_LOG(LogTemp, Warning, TEXT("StartSplinePatrol: No AIController for %s"), *GetNameSafe(this)); return; }

	FVector UseTarget = Target; // Use world-space target

	EPathFollowingRequestResult::Type MoveRes = AICon->MoveToLocation(UseTarget, SplinePatrolAcceptanceRadius-100);
	UE_LOG(LogTemp, Log, TEXT("StartSplinePatrol: %s starting spline patrol. StartIndex=%d NumPoints=%d MoveRes=%d MoveToTarget=(%s) splineTarget=(%s)"), *GetNameSafe(this), SplineCurrentIndex, Num, (int)MoveRes, *UseTarget.ToCompactString(), *Target.ToCompactString());
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Cyan, FString::Printf(TEXT("SplinePatrol: start idx %d res=%d"), SplineCurrentIndex, (int)MoveRes));
}

void AEnemy::StopSplinePatrol()
{
	AAIController* AICon = Cast<AAIController>(GetController());
	if (AICon)
	{
		if (AICon->GetPathFollowingComponent())
		{
			AICon->GetPathFollowingComponent()->OnRequestFinished.RemoveAll(this);
		}
		AICon->StopMovement();
	}
}

void AEnemy::OnPathFollowingRequestFinished(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	// Only handle for spline patrol
	if (!bUseSplinePatrol || !PatrolSpline) return;

	AAIController* AICon = Cast<AAIController>(GetController());
	if (!AICon) return;

	UE_LOG(LogTemp, Log, TEXT("OnPathFollowingRequestFinished: Enemy=%s Request=%s Result=%s"), *GetNameSafe(this), *RequestID.ToString(), Result.IsSuccess() ? TEXT("Success") : TEXT("Failed"));

	if (Result.IsSuccess())
	{
		int32 OldIndex = SplineCurrentIndex;
		int32 NewIndex = AdvanceSplineIndex();
		FVector NextLoc = GetSplinePointLocation(SplineCurrentIndex);
		EPathFollowingRequestResult::Type MoveRes = AICon->MoveToLocation(NextLoc, SplinePatrolAcceptanceRadius-100);
		UE_LOG(LogTemp, Log, TEXT("OnPathFollowingRequestFinished: Advancing spline %d -> %d, MoveRes=%d"), OldIndex, NewIndex, (int)MoveRes);
		if (GEngine) GEngine->AddOnScreenDebugMessage((int)GetUniqueID(), 2.0f, FColor::Green, FString::Printf(TEXT("SplinePatrol: advanced to %d"), NewIndex));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("OnPathFollowingRequestFinished: Move failed for %s; will retry in Tick."), *GetNameSafe(this));
	}
}

int32 AEnemy::AdvanceSplineIndex()
{
	if (!PatrolSpline) return INDEX_NONE;
	int32 Num = PatrolSpline->GetNumPoints();
	if (Num == 0) return INDEX_NONE;
	SplineCurrentIndex = (SplineCurrentIndex + 1) % Num;
	return SplineCurrentIndex;
}

FVector AEnemy::GetSplinePointLocation(int32 Index) const
{
	if (!PatrolSpline) return FVector::ZeroVector;
	return PatrolSpline->GetPointLocation(Index);
}
