// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EnemyController.generated.h"

class UAISenseConfig_Sight;
struct FAIStimulus;
class AEnemy; // forward-declare AEnemy so we can store a pointer to it

/**
 * 
 */
UCLASS()
class GGJ_MASK_API AEnemyController : public AAIController
{
	GENERATED_BODY()

public:
	AEnemyController();
	virtual void OnPossess(APawn* InPawn) override;

protected:
	// Perception is provided by the base AAIController (do not redeclare PerceptionComponent here)
	// Controller-owned sight config
	UPROPERTY()
	UAISenseConfig_Sight* SightConfig;

	// Forward perception updates to the pawn
	UFUNCTION()
	void OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	// Start behavior tree once nav is ready
	void StartBehaviorTreeWhenNavReady();
	void CheckNavAndStart();

	// cached pawn (AEnemy) we control
	UPROPERTY(Transient)
	AEnemy* ControlledEnemy = nullptr;

private:
	FTimerHandle NavCheckTimer;
	bool bBehaviorTreeStarted = false;
	float NavCheckInterval = 1.0f; // seconds between nav ready checks

	// When player is lost from sight, wait this many seconds before clearing blackboard target
	FTimerHandle LostSightTimer;
	float LostSightDelay = 5.0f;

	// Clear TargetActor and LastSeenLocation on blackboard after losing sight for LostSightDelay seconds
	void ClearTargetAndLastSeen();
};
