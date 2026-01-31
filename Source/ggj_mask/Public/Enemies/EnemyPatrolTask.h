// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "EnemyPatrolTask.generated.h"

/**
 * Patrol task: continuously moves the owning Enemy through its PatrolPoints.
 * The task stays InProgress and will be aborted when a higher priority branch (e.g. chase) triggers.
 */
UCLASS()
class GGJ_MASK_API UEnemyPatrolTask : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UEnemyPatrolTask();

	// ExecuteTask starts moving to the current patrol point and keeps the task in progress
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	// TickTask polls movement status and starts the next move when the previous completes
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	// Abort: stop movement when the task is aborted
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	// Node memory for per-instance state
	struct FPatrolTaskMemory
	{
		float TimeSinceLastRequest = 0.0f;
		uint8 bMoveRequested = 0; // whether a move is currently requested
	};

	virtual uint16 GetInstanceMemorySize() const override { return sizeof(FPatrolTaskMemory); }
};
