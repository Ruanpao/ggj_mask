// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Components/BoxComponent.h"

// Forward declare AI path-following types to avoid heavy includes in header
struct FAIRequestID;
struct FPathFollowingResult;

// Forward declarations to avoid requiring include paths in the header
//class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UBehaviorTree;
class USphereComponent;
class UFloatingPawnMovement;
class AMonitorDoor; // forward-declare MonitorDoor actor

#include "Enemy.generated.h"

UCLASS()
class GGJ_MASK_API AEnemy : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AEnemy();
	
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called when this pawn is possessed by a controller (start behavior tree when AIController arrives)
	virtual void PossessedBy(AController* NewController) override;

	// Collision component - provides a root collision volume so navigation and physics work
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision")
	USphereComponent* CollisionComp;

	// Movement component so Pawn can be moved by AIController::MoveTo
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	UFloatingPawnMovement* MovementComp;


	// Behavior tree asset slot (assignable in Editor)
	UPROPERTY(EditAnywhere, Category = "AI")
	UBehaviorTree* BehaviorTree;

	// // Perception callback to update blackboard key "HasTarget"
	// UFUNCTION()
	// void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	UPROPERTY(EditAnywhere, Category = "Mask")
	bool CanBeSmall;
	UPROPERTY(EditAnywhere, Category = "Mask")
	bool CanOpenDoor;
	// UPROPERTY(EditAnywhere, Category = "Mask")
	// bool CanDisableTraps;
	//

	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// Acceptance radius used for MoveTo calls
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float AcceptanceRadius = 100.0f;

	// Rotation speed (degrees per second) when turning to face the target
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float RotationSpeed = 720.0f;

public:
	// Patrol points that the enemy will visit when patrolling. Assign actors in the level (e.g. empty actors as waypoints).
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Patrol")
	TArray<AActor*> PatrolPoints;

	// Acceptance radius for reaching a patrol point
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	float PatrolAcceptanceRadius = 100.0f;

	// Returns the nearest patrol point actor to the enemy (or nullptr if none)
	UFUNCTION(BlueprintCallable, Category = "Patrol")
	AActor* GetNearestPatrolPoint() const;

	// Returns the current patrol point (based on CurrentPatrolIndex) or nullptr
	UFUNCTION(BlueprintCallable, Category = "Patrol")
	AActor* GetCurrentPatrolPoint() const;

	// Advance to the next patrol point and return it (wraps around). If no points, returns nullptr
	UFUNCTION(BlueprintCallable, Category = "Patrol")
	AActor* GetNextPatrolPoint();

	// Move to current patrol point using the owning AIController. Returns true if move requested.
	UFUNCTION(BlueprintCallable, Category = "Patrol")
	bool MoveToCurrentPatrolPoint();

	// Advance to next patrol point and start moving to it. Returns true if move requested.
	UFUNCTION(BlueprintCallable, Category = "Patrol")
	bool MoveToNextPatrolPoint();

	// Returns the nearest patrol point index to the enemy, or INDEX_NONE if none
	int32 GetNearestPatrolIndex() const;

	// Set the current patrol index (clamped). Use to resume patrol from a specific point.
	void SetCurrentPatrolIndex(int32 NewIndex);

	// Returns the current patrol index
	int32 GetCurrentPatrolIndex() const;

	// Returns the index that follows the current patrol index (wraps). Returns INDEX_NONE if no points.
	int32 GetIndexAfterCurrent() const;

	// Store the last patrol index (used by patrol task to remember where it came from)
	void SetLastPatrolIndex(int32 Index);
	int32 GetLastPatrolIndex() const;

public:
	// Optional spline to use for patrol paths. Assign an APatrolSpline in the level.
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Patrol")
	class APatrolSpline* PatrolSpline;

	// Enable using spline-based patrol
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	bool bUseSplinePatrol = false;

	// Patrol acceptance radius for spline points
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	float SplinePatrolAcceptanceRadius = 100.0f;

	// Current index along the spline
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Patrol")
	int32 SplineCurrentIndex = 0;

	// Start/stop spline patrol
	UFUNCTION(BlueprintCallable, Category = "Patrol")
	void StartSplinePatrol();
	UFUNCTION(BlueprintCallable, Category = "Patrol")
	void StopSplinePatrol();

	// Advance spline index (wraps)
	int32 AdvanceSplineIndex();

	// Get location of spline point
	FVector GetSplinePointLocation(int32 Index) const;

	// Index of the current patrol point in PatrolPoints
	UPROPERTY(VisibleInstanceOnly, Category = "Patrol")
	int32 CurrentPatrolIndex = 0;

	// Last patrol index visited (stored across tasks)
	UPROPERTY(VisibleInstanceOnly, Category = "Patrol")
	int32 LastPatrolIndex = INDEX_NONE;

	// Optional cached last move-to location when moving to a door intersection
	FVector LastMoveToLocation;

	// Spline patrol retry state
	float SplineIdleTimeAccum = 0.0f;
	int32 SplineMoveRetryCount = 0;

	// Handler for path following completion when using spline patrol
	void OnPathFollowingRequestFinished(FAIRequestID RequestID, const FPathFollowingResult& Result);

	// Helper: check whether a MonitorDoor actor is between this enemy and Target.
	// If so, returns true and fills OutHit with the first blocking hit.
	bool IsMonitorDoorBetween(AActor* Target, FHitResult& OutHit) const;

	// Fallback: when MoveTo repeatedly fails, use manual movement along spline
	bool SplineManualMoving = false;

	// Whether the behavior tree has been started (prevents double-start)
	bool bBehaviorTreeStarted = false;

	// Interaction box: when the player overlaps this box the enemy will interact (e.g., defeat the player)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	UBoxComponent* InteractionBox;

	// Overlap handler for the interaction box
	UFUNCTION()
	void OnInteractionOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);
};
