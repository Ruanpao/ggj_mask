// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "EnemyPatrolTask2.generated.h"

/**
 * Patrol Move Task: reads `PatrolActor` from blackboard and moves to it. Stays InProgress while moving.
 * If HasTarget becomes true, this task aborts so chase takes over.
 */
UCLASS()
class GGJ_MASK_API UEnemyPatrolTask2 : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UEnemyPatrolTask2();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	struct FPatrolMoveMemory
	{
		uint8 bMoveRequested = 0;
		float TimeSinceRequest = 0.0f;
	};

	virtual uint16 GetInstanceMemorySize() const override { return sizeof(FPatrolMoveMemory); }
};
