// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemies/EnemyPatrolTask.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "AIController.h"
#include "Enemies/Enemy.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

UEnemyPatrolTask::UEnemyPatrolTask()
{
	NodeName = TEXT("EnemyPatrolTask");
	bNotifyTick = false; // simple set task doesn't need ticking
}

EBTNodeResult::Type UEnemyPatrolTask::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// Resolve AI controller and pawn
	AAIController* AICon = Cast<AAIController>(OwnerComp.GetAIOwner());
	if (!AICon) return EBTNodeResult::Failed;

	AEnemy* EnemyPawn = Cast<AEnemy>(AICon->GetPawn());
	if (!EnemyPawn) return EBTNodeResult::Failed;

	// If no patrol points configured, fail
	if (EnemyPawn->PatrolPoints.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("EnemyPatrolTask: no patrol points on %s"), *GetNameSafe(EnemyPawn));
		return EBTNodeResult::Failed;
	}

	int32 NextIndex = EnemyPawn->GetIndexAfterCurrent();

	// record last index
	EnemyPawn->SetLastPatrolIndex(EnemyPawn->GetCurrentPatrolIndex());

	// resolve actor for next index
	AActor* NextActor = nullptr;
	if (EnemyPawn->PatrolPoints.IsValidIndex(NextIndex))
	{
		NextActor = EnemyPawn->PatrolPoints[NextIndex];
	}
	if (!NextActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("EnemyPatrolTask: Next patrol actor invalid for %s"), *GetNameSafe(EnemyPawn));
		return EBTNodeResult::Failed;
	}

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return EBTNodeResult::Failed;

	BB->SetValueAsInt(TEXT("NextPatrolIndex"), NextIndex);
	BB->SetValueAsObject(TEXT("NextPatrolPoint"), NextActor);

	UE_LOG(LogTemp, Log, TEXT("EnemyPatrolTask: %s set NextPatrolIndex=%d NextPatrolPoint=%s"), *GetNameSafe(EnemyPawn), NextIndex, *GetNameSafe(NextActor));
	DrawDebugSphere(EnemyPawn->GetWorld(), NextActor->GetActorLocation(), 60.0f, 12, FColor::Cyan, false, 2.0f);

	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, TEXT("EnemyPatrolTask: stubbed out"));
	return EBTNodeResult::Succeeded;
}

void UEnemyPatrolTask::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	// This task doesn't need tick behavior; left empty intentionally
}

EBTNodeResult::Type UEnemyPatrolTask::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AICon = Cast<AAIController>(OwnerComp.GetAIOwner());
	if (AICon)
	{
		AICon->StopMovement();
	}
	return EBTNodeResult::Aborted;
}
