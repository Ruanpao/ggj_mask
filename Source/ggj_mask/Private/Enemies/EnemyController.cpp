// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemies/EnemyController.h"
#include "Enemies/Enemy.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "ggj_mask/ggj_maskCharacter.h"
#include "MonitorDoor.h"
#include "DrawDebugHelpers.h"
#include "NavigationSystem.h"
#include "NavigationData.h"
#include "TimerManager.h"

static bool IsNavReadyForWorld(UWorld* World)
{
	if (!World) return false;
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (!NavSys) return false;
	ANavigationData* NavData = NavSys->GetDefaultNavDataInstance(FNavigationSystem::DontCreate);
	bool bReady = (NavData != nullptr && NavData->IsRegistered());
	UE_LOG(LogTemp, Verbose, TEXT("IsNavReadyForWorld: World=%s NavSys=%s NavData=%s Registered=%d"), *GetNameSafe(World), *GetNameSafe(NavSys), *GetNameSafe(NavData), (int)bReady);
	return bReady;
}

AEnemyController::AEnemyController()
{
	// Enable ticking if needed or set defaults here
	bAttachToPawn = true;

	// Create perception component on controller
	PerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComp"));
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	if (SightConfig)
	{
		SightConfig->SightRadius = 2000.0f;
		SightConfig->LoseSightRadius = 2200.0f;
		SightConfig->PeripheralVisionAngleDegrees = 90.0f;
		SightConfig->SetMaxAge(5.0f);
		SightConfig->DetectionByAffiliation.bDetectEnemies = true;
		SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
		SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	}

	if (PerceptionComponent && SightConfig)
	{
		PerceptionComponent->ConfigureSense(*SightConfig);
		PerceptionComponent->SetDominantSense(UAISense_Sight::StaticClass());
		PerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AEnemyController::OnPerceptionUpdated);
	}
}

void AEnemyController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	AEnemy* EnemyPawn = Cast<AEnemy>(InPawn);
	if (!EnemyPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("AEnemyController::OnPossess - possessed pawn is not AEnemy: %s"), *GetNameSafe(InPawn));
		return;
	}

	ControlledEnemy = EnemyPawn;
	UE_LOG(LogTemp, Log, TEXT("AEnemyController::OnPossess - Possessed %s (Enemy=%s). World=%s"), *GetNameSafe(InPawn), *GetNameSafe(ControlledEnemy), *GetNameSafe(GetWorld()));

	// Ensure perception component on controller is registered with world
	if (UAIPerceptionComponent* PC = FindComponentByClass<UAIPerceptionComponent>())
	{
		UE_LOG(LogTemp, Verbose, TEXT("AEnemyController::OnPossess - Found PerceptionComponent on Controller: Registered=%d"), (int)PC->IsRegistered());
		if (!PC->IsRegistered())
		{
			PC->RegisterComponent();
			UE_LOG(LogTemp, Verbose, TEXT("AEnemyController::OnPossess - Registered PerceptionComponent for controller %s"), *GetNameSafe(this));
		}
	}
	else
	{
		UE_LOG(LogTemp, Verbose, TEXT("AEnemyController::OnPossess - No PerceptionComponent found on Controller %s"), *GetNameSafe(this));
	}

	// Start a timer to check for nav readiness and start BT when ready
	StartBehaviorTreeWhenNavReady();
}

void AEnemyController::StartBehaviorTreeWhenNavReady()
{
	if (bBehaviorTreeStarted) { UE_LOG(LogTemp, Verbose, TEXT("StartBehaviorTreeWhenNavReady: already started for %s"), *GetNameSafe(ControlledEnemy)); return; }

	UE_LOG(LogTemp, Log, TEXT("StartBehaviorTreeWhenNavReady: Checking nav readiness for %s (World=%s)"), *GetNameSafe(ControlledEnemy), *GetNameSafe(GetWorld()));

	// Immediate check
	if (IsNavReadyForWorld(GetWorld()))
	{
		UE_LOG(LogTemp, Log, TEXT("StartBehaviorTreeWhenNavReady: Nav already ready for %s"), *GetNameSafe(ControlledEnemy));
		if (ControlledEnemy && ControlledEnemy->BehaviorTree)
		{
			RunBehaviorTree(ControlledEnemy->BehaviorTree);
			bBehaviorTreeStarted = true;
			UE_LOG(LogTemp, Log, TEXT("StartBehaviorTreeWhenNavReady: BehaviorTree started immediately for %s"), *GetNameSafe(ControlledEnemy));
		}
		return;
	}

	// Schedule periodic checks
	UE_LOG(LogTemp, Log, TEXT("StartBehaviorTreeWhenNavReady: Nav not ready yet, scheduling checks every %.2f seconds"), NavCheckInterval);
	GetWorldTimerManager().ClearTimer(NavCheckTimer);
	GetWorldTimerManager().SetTimer(NavCheckTimer, this, &AEnemyController::CheckNavAndStart, NavCheckInterval, true);
}

void AEnemyController::CheckNavAndStart()
{
	UE_LOG(LogTemp, Warning, TEXT("CheckNavAndStart: Checking nav for %s..."), *GetNameSafe(ControlledEnemy));
	if (bBehaviorTreeStarted) { GetWorldTimerManager().ClearTimer(NavCheckTimer); UE_LOG(LogTemp, Verbose, TEXT("CheckNavAndStart: already started, clearing timer")); return; }
	if (!GetWorld()) { UE_LOG(LogTemp, Warning, TEXT("CheckNavAndStart: No World available")); return; }

	if (IsNavReadyForWorld(GetWorld()))
	{
		UE_LOG(LogTemp, Log, TEXT("CheckNavAndStart: Nav is ready for %s, starting BT"), *GetNameSafe(ControlledEnemy));
		if (ControlledEnemy && ControlledEnemy->BehaviorTree)
		{
			RunBehaviorTree(ControlledEnemy->BehaviorTree);
			bBehaviorTreeStarted = true;
			UE_LOG(LogTemp, Log, TEXT("CheckNavAndStart: BehaviorTree started after nav ready for %s"), *GetNameSafe(ControlledEnemy));
		}
		GetWorldTimerManager().ClearTimer(NavCheckTimer);
	}
}

void AEnemyController::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	UE_LOG(LogTemp, Verbose, TEXT("OnPerceptionUpdated: Actor=%s Stimulus.Success=%d"), *GetNameSafe(Actor), (int)Stimulus.WasSuccessfullySensed());

	// Only care about the player character
	if (!Actor || !Actor->IsA(Aggj_maskCharacter::StaticClass())) { UE_LOG(LogTemp, Verbose, TEXT("OnPerceptionUpdated: Ignoring actor %s (not player)"), *GetNameSafe(Actor)); return; }

	// Ensure we have a valid enemy pawn reference
	AEnemy* EnemyPawn = ControlledEnemy ? ControlledEnemy : Cast<AEnemy>(GetPawn());
	if (!EnemyPawn) { UE_LOG(LogTemp, Warning, TEXT("OnPerceptionUpdated: No EnemyPawn available for controller %s"), *GetNameSafe(this)); return; }

	UBlackboardComponent* BB = GetBlackboardComponent();
	if (!BB) { UE_LOG(LogTemp, Warning, TEXT("OnPerceptionUpdated: No Blackboard on controller %s"), *GetNameSafe(this)); return; }

	const FName HasTargetKey = TEXT("HasTarget");
	const FName TargetActorKey = TEXT("TargetActor");
	const FName LastSeenKey = TEXT("LastSeenLocation");
	const FName IsBlockedKey = TEXT("IsBlockedByDoor");
	const FName DoorImpactKey = TEXT("DoorImpactLocation");

	// If we currently sense the player
	if (Stimulus.WasSuccessfullySensed())
	{
		UE_LOG(LogTemp, Log, TEXT("OnPerceptionUpdated: Sensed player %s for enemy %s"), *GetNameSafe(Actor), *GetNameSafe(EnemyPawn));
		// Write target info
		BB->SetValueAsBool(HasTargetKey, true);
		BB->SetValueAsObject(TargetActorKey, Actor);
		BB->SetValueAsVector(LastSeenKey, Actor->GetActorLocation());
		UE_LOG(LogTemp, Verbose, TEXT("OnPerceptionUpdated: Wrote HasTarget=true, TargetActor=%s, LastSeen=%s"), *GetNameSafe(Actor), *Actor->GetActorLocation().ToCompactString());

		// Line trace from enemy to player to see if a MonitorDoor blocks visibility
		FHitResult Hit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(EnemyPawn);
		Params.AddIgnoredActor(Actor);
		bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, EnemyPawn->GetActorLocation(), Actor->GetActorLocation(), ECC_Visibility, Params);
		if (bHit)
		{
			UE_LOG(LogTemp, Verbose, TEXT("OnPerceptionUpdated: Trace hit actor %s at %s"), *GetNameSafe(Hit.GetActor()), *Hit.ImpactPoint.ToCompactString());
			if (Hit.GetActor() && Hit.GetActor()->IsA(AMonitorDoor::StaticClass()))
			{
				BB->SetValueAsBool(IsBlockedKey, true);
				BB->SetValueAsVector(DoorImpactKey, Hit.ImpactPoint);
				UE_LOG(LogTemp, Log, TEXT("OnPerceptionUpdated: Player blocked by MonitorDoor, DoorImpact=%s"), *Hit.ImpactPoint.ToCompactString());
			}
			else
			{
				BB->SetValueAsBool(IsBlockedKey, false);
				BB->ClearValue(DoorImpactKey);
				UE_LOG(LogTemp, Verbose, TEXT("OnPerceptionUpdated: Player not blocked by door"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Verbose, TEXT("OnPerceptionUpdated: Trace did not hit anything between enemy and player"));
			BB->SetValueAsBool(IsBlockedKey, false);
			BB->ClearValue(DoorImpactKey);
		}
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("OnPerceptionUpdated: Lost sight of player for enemy %s"), *GetNameSafe(EnemyPawn));
		// Lost sight of player. If we previously had a target, trace to LastSeenLocation to see if blocked by door
		bool bHadTarget = BB->GetValueAsBool(HasTargetKey);
		if (!bHadTarget) { UE_LOG(LogTemp, Verbose, TEXT("OnPerceptionUpdated: Previously had no target")); return; }

		FVector LastSeen = BB->GetValueAsVector(LastSeenKey);
		FHitResult Hit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(EnemyPawn);
		bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, EnemyPawn->GetActorLocation(), LastSeen, ECC_Visibility, Params);
		if (bHit)
		{
			UE_LOG(LogTemp, Verbose, TEXT("OnPerceptionUpdated: Lost sight trace hit actor %s at %s"), *GetNameSafe(Hit.GetActor()), *Hit.ImpactPoint.ToCompactString());
			if (Hit.GetActor() && Hit.GetActor()->IsA(AMonitorDoor::StaticClass()))
			{
				BB->SetValueAsBool(IsBlockedKey, true);
				BB->SetValueAsVector(DoorImpactKey, Hit.ImpactPoint);
				UE_LOG(LogTemp, Log, TEXT("OnPerceptionUpdated: Lost sight - blocked by MonitorDoor, DoorImpact=%s"), *Hit.ImpactPoint.ToCompactString());
			}
			else
			{
				// No door blocking - clear target and related keys
				BB->SetValueAsBool(HasTargetKey, false);
				BB->ClearValue(TargetActorKey);
				BB->SetValueAsBool(IsBlockedKey, false);
				BB->ClearValue(DoorImpactKey);
				UE_LOG(LogTemp, Log, TEXT("OnPerceptionUpdated: Lost sight - cleared target for %s"), *GetNameSafe(EnemyPawn));
			}
		}
		else
		{
			BB->SetValueAsBool(HasTargetKey, false);
			BB->ClearValue(TargetActorKey);
			BB->SetValueAsBool(IsBlockedKey, false);
			BB->ClearValue(DoorImpactKey);
			UE_LOG(LogTemp, Log, TEXT("OnPerceptionUpdated: Lost sight - no hit, cleared target for %s"), *GetNameSafe(EnemyPawn));
		}
	}
}
