// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_UpdateTargetInfo.generated.h"

/**
 * Service that updates blackboard keys related to seeing/chasing the player and door blocking.
 * Writes: HasTarget (bool), TargetActor (object), LastSeenLocation (vector), IsBlockedByDoor (bool), DoorImpactLocation (vector)
 */
UCLASS(DisplayName = "UpdateTargetInfo")
class GGJ_MASK_API UBTService_UpdateTargetInfo : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_UpdateTargetInfo();

	// Provide a small instance memory so we can debounce noisy door-blocked state
	virtual uint16 GetInstanceMemorySize() const override;

protected:
	// Tick called by behavior tree
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	// Backoff distance from the exact door impact point so AI doesn't try to path into the door collision
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float BackOffDistance = 50.0f;

	// Debounce time before committing a change to IsBlockedByDoor to avoid flicker
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float BlockedDebounceTime = 0.2f;

	// The names of blackboard keys used (kept as constants here for clarity)
	static const FName HasTargetKey;
	static const FName TargetActorKey;
	static const FName LastSeenLocationKey;
	static const FName IsBlockedByDoorKey;
	static const FName DoorImpactLocationKey;

	// Per-instance memory for this service (node memory)
	struct FInstanceMemory
	{
		float BlockedAccum = 0.0f; // accumulated time the candidate blocked state has been observed
		bool bLastBlocked = false; // last stable blocked state written to blackboard
	};
};
