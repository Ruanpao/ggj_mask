// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemies/EnemyController.h"
#include "Enemies/Enemy.h"
#include "BehaviorTree/BlackboardComponent.h"

AEnemyController::AEnemyController()
{
	// Enable ticking if needed or set defaults here
	bAttachToPawn = true;
}

void AEnemyController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	AEnemy* EnemyPawn = Cast<AEnemy>(InPawn);
	if (!EnemyPawn) return;

	if (EnemyPawn->BehaviorTree)
	{
		// UBlackboardComponent* BBComp = nullptr;
		// if (EnemyPawn->BehaviorTree->BlackboardAsset)
		// {
		// 	UseBlackboard(EnemyPawn->BehaviorTree->BlackboardAsset, BBComp);
		// }
		RunBehaviorTree(EnemyPawn->BehaviorTree);
	}
}
