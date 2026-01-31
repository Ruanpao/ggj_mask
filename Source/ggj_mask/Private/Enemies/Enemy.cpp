// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemies/Enemy.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "MonitorDoor.h" // include MonitorDoor so we can detect class in traces
#include "Engine/World.h"
#include "ggj_mask/ggj_maskCharacter.h" // player character class
#include "DrawDebugHelpers.h"
#include "Enemies/PatrolSpline.h"
#include "Navigation/PathFollowingComponent.h"
#include "Engine/Engine.h"
#include "NavigationSystem.h"
#include "NavigationSystemTypes.h"
#include "BehaviorTree/BehaviorTree.h"
// include masks and draggable cube for spawning/overlap handling
#include "Masks/BeSmallMask.h"
#include "Masks/OpenDoorMask.h"
#include "Masks/DragMask.h"
#include "DraggableCube.h"
#include "UObject/UnrealType.h" // for FProperty/FBoolProperty

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

	// Create movement component and attach
	MovementComp = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("MovementComp"));
	if (MovementComp)
	{
		MovementComp->UpdatedComponent = CollisionComp;
		MovementComp->MaxSpeed = 600.0f;
	}

	// Interaction box for player interaction (will trigger defeat UI when player overlaps)
	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	if (InteractionBox)
	{
		InteractionBox->SetupAttachment(RootComponent);
		InteractionBox->SetBoxExtent(FVector(120.f, 120.f, 120.f));
		InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		InteractionBox->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
		InteractionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		InteractionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
		InteractionBox->SetGenerateOverlapEvents(true);
	}

	// Create blackboard/behavior components removed — controller will manage them
}

// Called when the game starts or when spawned
void AEnemy::BeginPlay()
{
	Super::BeginPlay();

	// Bind interaction overlap
	if (InteractionBox)
	{
		InteractionBox->OnComponentBeginOverlap.AddDynamic(this, &AEnemy::OnInteractionOverlapBegin);
	}

	// Defensive: if a perception component was accidentally added to the Pawn (via BP), remove it.


	// Initialize BehaviorTree and Blackboard using the Pawn's Controller (AAIController)
	if (BehaviorTree)
	{
		AAIController* AICon = Cast<AAIController>(GetController());
		if (!AICon)
		{
			// If controller not yet possessed, try to spawn logic later or log
			// We'll rely on PossessedBy to start the tree when controller arrives
		}
		else
		{
			if (!bBehaviorTreeStarted)
			{
				if (BehaviorTree->BlackboardAsset)
				{
					UBlackboardComponent* BBComp = AICon->GetBlackboardComponent();
					AICon->UseBlackboard(BehaviorTree->BlackboardAsset, BBComp);
				}

				AICon->RunBehaviorTree(BehaviorTree);
				bBehaviorTreeStarted = true;
			}
		}
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
	float SightRadius = 2000.0f; // keep local defaults for debug drawing
	float PeripheralAngle = 45.0f;
	
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

	// If spline patrol is enabled, and we DO NOT have a target, perform simple Tick-driven patrol
	if (bUseSplinePatrol && PatrolSpline)
	{
		bool bHasTargetBB = BBComp->GetValueAsBool(TEXT("HasTarget"));
		int32 NumPoints = PatrolSpline->GetNumPoints();
		//UE_LOG(LogTemp, Verbose, TEXT("SplineTick: Enemy=%s bUseSpline=%d NumPoints=%d CurrIdx=%d HasTarget=%d"), *GetNameSafe(this), (int)bUseSplinePatrol, NumPoints, SplineCurrentIndex, (int)bHasTargetBB);

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

			// UE_LOG(LogTemp, Log, TEXT("SplineTick: Enemy=%s idx=%d splineTarget=%s actor=%s moveTarget=%s pathEnd=%s distToMoveTarget=%.2fcm accept=%.2fcm"),
			// 	*GetNameSafe(this), SplineCurrentIndex, *TargetLoc.ToCompactString(), *ActorLoc.ToCompactString(), *MoveTarget.ToCompactString(), *PathEnd.ToCompactString(), Dist, Accept);
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
				//UE_LOG(LogTemp, Log, TEXT("SplineTick: Arrived at MoveTarget. Advancing %d -> %d actor=%s splineTarget=%s nextTarget=%s dist=%.2fcm accept=%.2fcm"),
				//	Old, NewIdx, *ActorLoc.ToCompactString(), *TargetLoc.ToCompactString(), *NextLoc.ToCompactString(), Dist, Accept);
				EPathFollowingRequestResult::Type MoveRes = AICon->MoveToLocation(UseNext, 10);
				//UE_LOG(LogTemp, Log, TEXT("SplineTick: MoveTo requested nextTarget=%s result=%d"), *UseNext.ToCompactString(), (int)MoveRes);
				if (PF && PF->GetPath())
				{
					const auto& Points = PF->GetPath()->GetPathPoints();
					if (Points.Num() > 0)
					{
						FVector PathEnd2 = Points.Last().Location;
						//UE_LOG(LogTemp, Verbose, TEXT("SplineTick: Path end point = %s (dist to actor=%.2fcm)"), *PathEnd2.ToCompactString(), FVector::Dist(ActorLoc, PathEnd2));
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
                    //UE_LOG(LogTemp, Warning, TEXT("SplineTick: Re-request MoveTo. Actor=%s splineTarget=%s moveTarget=%s pathEnd=%s distToMoveTarget=%.2fcm accept=%.2fcm"), *ActorLoc.ToCompactString(), *TargetLoc.ToCompactString(), *MoveTarget.ToCompactString(), *PathEnd.ToCompactString(), Dist, Accept);
                    EPathFollowingRequestResult::Type MoveRes = AICon->MoveToLocation(MoveTarget, 10);
                    //UE_LOG(LogTemp, Warning, TEXT("SplineTick: MoveTo requested target=%s res=%d"), *MoveTarget.ToCompactString(), (int)MoveRes);
                    if (PF && PF->GetPath())
                    {
                        const auto& Points = PF->GetPath()->GetPathPoints();
                        if (Points.Num() > 0)
                        {
                            FVector PathEnd3 = Points.Last().Location;
                            //UE_LOG(LogTemp, Verbose, TEXT("SplineTick: Path end point = %s (dist to actor=%.2fcm)"), *PathEnd3.ToCompactString(), FVector::Dist(ActorLoc, PathEnd3));
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
//
// void AEnemy::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
// {
// 	// Filter out MonitorDoor actors from being treated as sensed targets
// 	if (Actor && Actor->IsA(AMonitorDoor::StaticClass()))
// 	{
// 		// Ignore MonitorDoor perception updates entirely
// 		return;
// 	}
//
// 	// Only treat player character as a valid TargetActor
// 	if (!Actor || !Actor->IsA(Aggj_maskCharacter::StaticClass()))
// 	{
// 		// Not the player -> ignore for target selection
// 		return;
// 	}
//
// 	AAIController* AICon = Cast<AAIController>(GetController());
// 	if (!AICon) return;
//
// 	UBlackboardComponent* BBComp = AICon->GetBlackboardComponent();
// 	if (!BBComp) return;
//
// 	// Keys used in blackboard. Assumption: Blackboard has these keys defined.
// 	const FName HasTargetKey = TEXT("HasTarget");
// 	const FName TargetActorKey = TEXT("TargetActor"); // change this name if your blackboard uses a different key
//
// 	if (Stimulus.WasSuccessfullySensed())
// 	{
// 		// Set boolean and actor reference when sensed
// 		BBComp->SetValueAsBool(HasTargetKey, true);
// 		BBComp->SetValueAsObject(TargetActorKey, Actor);
// 	}
// 	else
// 	{
// 		// Only clear the actor key if the blackboard currently references this actor
// 		UObject* CurrentTarget = BBComp->GetValueAsObject(TargetActorKey);
// 		if (CurrentTarget == Actor)
// 		{
// 			BBComp->ClearValue(TargetActorKey);
// 			BBComp->SetValueAsBool(HasTargetKey, false);
// 		}
// 		// If CurrentTarget != Actor, another target is already set; don't overwrite it here.
// 	}
// }

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

	AICon->MoveToActor(Target, PatrolAcceptanceRadius);
	return true;
}

bool AEnemy::MoveToNextPatrolPoint()
{
	AActor* Next = GetNextPatrolPoint();
	if (!Next) return false;

	AAIController* AICon = Cast<AAIController>(GetController());
	if (!AICon) return false;

	AICon->MoveToActor(Next, PatrolAcceptanceRadius);
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

	EPathFollowingRequestResult::Type MoveRes = AICon->MoveToLocation(UseTarget, 10);
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
		EPathFollowingRequestResult::Type MoveRes = AICon->MoveToLocation(NextLoc, 10);
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

// Called when possessed by controller
void AEnemy::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);

    if (bBehaviorTreeStarted) return;

    AAIController* AICon = Cast<AAIController>(NewController);
    if (!AICon) return;

    if (BehaviorTree)
    {
        if (BehaviorTree->BlackboardAsset)
        {
            UBlackboardComponent* BBComp = AICon->GetBlackboardComponent();
            AICon->UseBlackboard(BehaviorTree->BlackboardAsset, BBComp);
        }

        AICon->RunBehaviorTree(BehaviorTree);
        bBehaviorTreeStarted = true;
        UE_LOG(LogTemp, Log, TEXT("AEnemy::PossessedBy - BehaviorTree started for %s"), *GetNameSafe(this));
    }
}

void AEnemy::OnInteractionOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                                      UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
    if (!OtherActor) return;

    // Check if the overlapping actor is the player character
    Aggj_maskCharacter* PlayerCharacter = Cast<Aggj_maskCharacter>(OtherActor);
    if (PlayerCharacter)
    {
        // Prevent multiple triggers
        if (PlayerCharacter->bIsDefeated) return;

        // Mark player defeated and show defeat UI
        PlayerCharacter->bIsDefeated = true;
        PlayerCharacter->ShowDefeatUI();

        UE_LOG(LogTemp, Log, TEXT("AEnemy::OnInteractionOverlapBegin - Player %s defeated by Enemy %s"), *GetNameSafe(PlayerCharacter), *GetNameSafe(this));
        // Optionally disable enemy movement or other effects here
        // e.g., StopSplinePatrol();
    }

    // If overlapped a draggable cube, spawn mask pickups according to flags then destroy this enemy
    ADraggableCube* DC = Cast<ADraggableCube>(OtherActor);
    if (DC)
    {
        UE_LOG(LogTemp, Log, TEXT("AEnemy::OnInteractionOverlapBegin - overlapped DraggableCube %s, spawning masks for %s"), *GetNameSafe(DC), *GetNameSafe(this));

        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
        SpawnParams.Owner = this;

        FVector SpawnLoc = GetActorLocation();
        FRotator SpawnRot = GetActorRotation();

        UWorld* World = GetWorld();
        if (!World)
        {
            UE_LOG(LogTemp, Warning, TEXT("AEnemy::OnInteractionOverlapBegin - no World to spawn masks"));
            return;
        }

        // Lambda: try to read a bool property from a class default object using candidate names
        auto ReadBoolFromCDO = [](UClass* InClass, const TArray<FName>& Candidates, bool& OutVal)->bool
        {
            if (!InClass) return false;
            UObject* CDO = InClass->GetDefaultObject();
            if (!CDO) return false;
            for (const FName& Name : Candidates)
            {
                FProperty* Prop = InClass->FindPropertyByName(Name);
                if (!Prop) continue;
                if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Prop))
                {
                    OutVal = BoolProp->GetPropertyValue_InContainer(CDO);
                    return true;
                }
            }
            return false;
        };

        // Small mask: prefer blueprint flag if BP assigned, otherwise use enemy fallback or spawn C++ class
        if (SmallMaskBP)
        {
            TArray<FName> Candidates = { FName(TEXT("CanDropSmallMask")), FName(TEXT("bCanDropSmallMask")), FName(TEXT("bShouldDropSmall")), FName(TEXT("bShouldDrop")), FName(TEXT("bCanDrop")) };
            bool bVal = false;
            bool bHasFlag = ReadBoolFromCDO(SmallMaskBP.Get(), Candidates, bVal);
            if (bHasFlag ? bVal : CanDropSmallMask)
            {
                AActor* Spawned = World->SpawnActor<AActor>(SmallMaskBP.Get(), SpawnLoc, SpawnRot, SpawnParams);
                UE_LOG(LogTemp, Log, TEXT("AEnemy: spawned SmallMask via BP class %s actor=%s (bpFlag=%s, enemyFallback=%s)"), *GetNameSafe(SmallMaskBP.Get()), *GetNameSafe(Spawned), bHasFlag ? (bVal ? TEXT("true") : TEXT("false")) : TEXT("n/a"), CanDropSmallMask?TEXT("true"):TEXT("false"));
            }
            else
            {
                UE_LOG(LogTemp, Log, TEXT("AEnemy: SmallMaskBP %s indicated no spawn (bpFlag=%s, enemyFallback=%s)"), *GetNameSafe(SmallMaskBP.Get()), bHasFlag?(bVal?TEXT("true"):TEXT("false")):TEXT("n/a"), CanDropSmallMask?TEXT("true"):TEXT("false"));
            }
        }
        else if (CanDropSmallMask)
        {
            ABeSmallMask* Small = World->SpawnActor<ABeSmallMask>(ABeSmallMask::StaticClass(), SpawnLoc, SpawnRot, SpawnParams);
            UE_LOG(LogTemp, Log, TEXT("AEnemy: spawned SmallMask (C++) actor=%s via enemy fallback"), *GetNameSafe(Small));
        }

        // Open door mask
        if (OpenDoorMaskBP)
        {
            TArray<FName> Candidates = { FName(TEXT("CanDropOpenDoorMask")), FName(TEXT("bCanDropOpenDoorMask")), FName(TEXT("bShouldDropOpenDoor")), FName(TEXT("bShouldDrop")), FName(TEXT("bCanDrop")) };
            bool bVal = false;
            bool bHasFlag = ReadBoolFromCDO(OpenDoorMaskBP.Get(), Candidates, bVal);
            if (bHasFlag ? bVal : CanDropOpenDoorMask)
            {
                AActor* Spawned = World->SpawnActor<AActor>(OpenDoorMaskBP.Get(), SpawnLoc, SpawnRot, SpawnParams);
                UE_LOG(LogTemp, Log, TEXT("AEnemy: spawned OpenDoorMask via BP class %s actor=%s (bpFlag=%s, enemyFallback=%s)"), *GetNameSafe(OpenDoorMaskBP.Get()), *GetNameSafe(Spawned), bHasFlag ? (bVal ? TEXT("true") : TEXT("false")) : TEXT("n/a"), CanDropOpenDoorMask?TEXT("true"):TEXT("false"));
            }
            else
            {
                UE_LOG(LogTemp, Log, TEXT("AEnemy: OpenDoorMaskBP %s indicated no spawn (bpFlag=%s, enemyFallback=%s)"), *GetNameSafe(OpenDoorMaskBP.Get()), bHasFlag?(bVal?TEXT("true"):TEXT("false")):TEXT("n/a"), CanDropOpenDoorMask?TEXT("true"):TEXT("false"));
            }
        }
        else if (CanDropOpenDoorMask)
        {
            AOpenDoorMask* Open = World->SpawnActor<AOpenDoorMask>(AOpenDoorMask::StaticClass(), SpawnLoc, SpawnRot, SpawnParams);
            UE_LOG(LogTemp, Log, TEXT("AEnemy: spawned OpenDoorMask (C++) actor=%s via enemy fallback"), *GetNameSafe(Open));
        }

        // Drag mask
        if (DragMaskBP)
        {
            TArray<FName> Candidates = { FName(TEXT("CanDropDragMask")), FName(TEXT("bCanDropDragMask")), FName(TEXT("bShouldDropDrag")), FName(TEXT("bShouldDrop")), FName(TEXT("bCanDrop")) };
            bool bVal = false;
            bool bHasFlag = ReadBoolFromCDO(DragMaskBP.Get(), Candidates, bVal);
            if (bHasFlag ? bVal : CanDropDragMask)
            {
                AActor* Spawned = World->SpawnActor<AActor>(DragMaskBP.Get(), SpawnLoc, SpawnRot, SpawnParams);
                UE_LOG(LogTemp, Log, TEXT("AEnemy: spawned DragMask via BP class %s actor=%s (bpFlag=%s, enemyFallback=%s)"), *GetNameSafe(DragMaskBP.Get()), *GetNameSafe(Spawned), bHasFlag ? (bVal ? TEXT("true") : TEXT("false")) : TEXT("n/a"), CanDropDragMask?TEXT("true"):TEXT("false"));
            }
            else
            {
                UE_LOG(LogTemp, Log, TEXT("AEnemy: DragMaskBP %s indicated no spawn (bpFlag=%s, enemyFallback=%s)"), *GetNameSafe(DragMaskBP.Get()), bHasFlag?(bVal?TEXT("true"):TEXT("false")):TEXT("n/a"), CanDropDragMask?TEXT("true"):TEXT("false"));
            }
        }
        else if (CanDropDragMask)
        {
            ADragMask* Drag = World->SpawnActor<ADragMask>(ADragMask::StaticClass(), SpawnLoc, SpawnRot, SpawnParams);
            UE_LOG(LogTemp, Log, TEXT("AEnemy: spawned DragMask (C++) actor=%s via enemy fallback"), *GetNameSafe(Drag));
        }

        // Destroy this enemy after spawning
        UE_LOG(LogTemp, Log, TEXT("AEnemy: %s destroyed after dropping masks"), *GetNameSafe(this));
        Destroy();
        return;
    }

}
