// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemies/EnemyPatrolTask2.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Enemies/Enemy.h"
#include "DrawDebugHelpers.h"
#include "Navigation/PathFollowingComponent.h"
#include "Engine/Engine.h"

UEnemyPatrolTask2::UEnemyPatrolTask2()
{
	bNotifyTick = true;
}

EBTNodeResult::Type UEnemyPatrolTask2::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	FPatrolMoveMemory* Mem = reinterpret_cast<FPatrolMoveMemory*>(NodeMemory);
	Mem->bMoveRequested = 0;
	Mem->TimeSinceRequest = 0.0f;

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return EBTNodeResult::Failed;

	UObject* Obj = BB->GetValueAsObject(TEXT("NextPatrolPoint"));
	AActor* PatrolActor = Obj ? Cast<AActor>(Obj) : nullptr;
	if (!PatrolActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("EnemyPatrolTask2::ExecuteTask - NextPatrolPoint not set on blackboard"));
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, TEXT("PatrolTask2: NextPatrolPoint not set"));
		return EBTNodeResult::Failed;
	}

	int32 NextIndex = BB->GetValueAsInt(TEXT("NextPatrolIndex"));

	AAIController* AICon = Cast<AAIController>(OwnerComp.GetAIOwner());
	if (!AICon) return EBTNodeResult::Failed;

	AEnemy* EnemyPawn = Cast<AEnemy>(AICon->GetPawn());
	if (!EnemyPawn) return EBTNodeResult::Failed;

	UE_LOG(LogTemp, Log, TEXT("EnemyPatrolTask2::ExecuteTask - %s attempting MoveTo NextPatrolIndex=%d (%s)"), *GetNameSafe(EnemyPawn), NextIndex, *GetNameSafe(PatrolActor));

	// request move
	EPathFollowingRequestResult::Type MoveRes = AICon->MoveToActor(PatrolActor, EnemyPawn->PatrolAcceptanceRadius);
	bool bStarted = (MoveRes == EPathFollowingRequestResult::RequestSuccessful || MoveRes == EPathFollowingRequestResult::AlreadyAtGoal);
	if (bStarted)
	{
		Mem->bMoveRequested = 1;
		Mem->TimeSinceRequest = 0.0f;
		DrawDebugSphere(EnemyPawn->GetWorld(), PatrolActor->GetActorLocation(), 30.0f, 8, FColor::Green, false, 1.0f);
		UE_LOG(LogTemp, Log, TEXT("EnemyPatrolTask2: %s started moving to NextPatrolIndex=%d (%s)"), *GetNameSafe(EnemyPawn), NextIndex, *GetNameSafe(PatrolActor));
		return EBTNodeResult::InProgress;
	}

	// couldn't start move
	UE_LOG(LogTemp, Warning, TEXT("EnemyPatrolTask2::ExecuteTask - MoveToActor returned %d for %s (index %d)."), (int)MoveRes, *GetNameSafe(PatrolActor), NextIndex);
	return EBTNodeResult::Failed;
}

void UEnemyPatrolTask2::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	FPatrolMoveMemory* Mem = reinterpret_cast<FPatrolMoveMemory*>(NodeMemory);
	Mem->TimeSinceRequest += DeltaSeconds;

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// If detects a player, abort so chase can take over
	if (BB->GetValueAsBool(TEXT("HasTarget")))
	{
		UE_LOG(LogTemp, Log, TEXT("EnemyPatrolTask2::TickTask - HasTarget detected, aborting patrol task."));
		FinishLatentTask(OwnerComp, EBTNodeResult::Aborted);
		return;
	}

	UObject* Obj = BB->GetValueAsObject(TEXT("NextPatrolPoint"));
	AActor* PatrolActor = Obj ? Cast<AActor>(Obj) : nullptr;
	if (!PatrolActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("EnemyPatrolTask2::TickTask - NextPatrolPoint missing from blackboard during tick."));
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	int32 NextIndex = BB->GetValueAsInt(TEXT("NextPatrolIndex"));

	AAIController* AICon = Cast<AAIController>(OwnerComp.GetAIOwner());
	if (!AICon)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	AEnemy* EnemyPawn = Cast<AEnemy>(AICon->GetPawn());
	if (!EnemyPawn)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	float DistSq = FVector::DistSquared(EnemyPawn->GetActorLocation(), PatrolActor->GetActorLocation());
	UE_LOG(LogTemp, Log, TEXT("EnemyPatrolTask2::TickTask - %s distance to NextPatrolPoint(index %d): %f (acceptance %f)"), *GetNameSafe(EnemyPawn), NextIndex, DistSq, EnemyPawn->PatrolAcceptanceRadius);
	// Also show on-screen debug string for easier visibility
	if (EnemyPawn->GetWorld() && PatrolActor)
	{
		FVector TextLoc = PatrolActor->GetActorLocation() + FVector(0.f, 0.f, 100.f);
		FString Msg = FString::Printf(TEXT("PatrolIdx=%d Dist=%0.1f"), NextIndex, FMath::Sqrt(DistSq));
		DrawDebugString(EnemyPawn->GetWorld(), TextLoc, Msg, nullptr, FColor::White, 0.1f, true);
	}
	if (DistSq <= FMath::Square(EnemyPawn->PatrolAcceptanceRadius))
	{
		// Arrived: set current index on the pawn to NextIndex, then succeed
		EnemyPawn->SetCurrentPatrolIndex(NextIndex);
		UE_LOG(LogTemp, Log, TEXT("EnemyPatrolTask2: %s arrived at NextPatrolIndex=%d (%s). Updated CurrentPatrolIndex."), *GetNameSafe(EnemyPawn), NextIndex, *GetNameSafe(PatrolActor));
		if (EnemyPawn->GetWorld())
		{
			DrawDebugString(EnemyPawn->GetWorld(), PatrolActor->GetActorLocation() + FVector(0.f,0.f,120.f), TEXT("ARRIVED"), nullptr, FColor::Green, 2.0f, true);
		}
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	// Retry move request if needed
	if (!Mem->bMoveRequested || Mem->TimeSinceRequest > 1.0f)
	{
		Mem->TimeSinceRequest = 0.0f;
		EPathFollowingRequestResult::Type MoveRes = AICon->MoveToActor(PatrolActor, EnemyPawn->PatrolAcceptanceRadius);
		bool bNowRequested = (MoveRes == EPathFollowingRequestResult::RequestSuccessful || MoveRes == EPathFollowingRequestResult::AlreadyAtGoal);
		if (bNowRequested != (bool)Mem->bMoveRequested)
		{
			UE_LOG(LogTemp, Log, TEXT("EnemyPatrolTask2::TickTask - Move request state changed: was %d, now %d (res=%d) for %s index=%d"), Mem->bMoveRequested, (int)bNowRequested, (int)MoveRes, *GetNameSafe(PatrolActor), NextIndex);
			if (EnemyPawn->GetWorld())
			{
				FString Msg = FString::Printf(TEXT("MoveState: %d -> %d"), Mem->bMoveRequested, (int)bNowRequested);
				DrawDebugString(EnemyPawn->GetWorld(), PatrolActor->GetActorLocation()+FVector(0.f,0.f,80.f), Msg, nullptr, FColor::Yellow, 1.5f, true);
			}
		}
		Mem->bMoveRequested = bNowRequested;
	}
}

EBTNodeResult::Type UEnemyPatrolTask2::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AICon = Cast<AAIController>(OwnerComp.GetAIOwner());
	if (AICon)
	{
		AICon->StopMovement();
	}
	return EBTNodeResult::Aborted;
}
