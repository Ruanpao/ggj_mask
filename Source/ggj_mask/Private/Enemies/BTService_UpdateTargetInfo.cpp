#include "Enemies/BTService_UpdateTargetInfo.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "AIController.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"
#include "Engine/World.h"
#include "ggj_mask/ggj_maskCharacter.h"
#include "MonitorDoor.h"
#include "GameFramework/Pawn.h"

// Define static keys
const FName UBTService_UpdateTargetInfo::HasTargetKey = TEXT("HasTarget");
const FName UBTService_UpdateTargetInfo::TargetActorKey = TEXT("TargetActor");
const FName UBTService_UpdateTargetInfo::LastSeenLocationKey = TEXT("LastSeenLocation");
const FName UBTService_UpdateTargetInfo::IsBlockedByDoorKey = TEXT("IsBlockedByDoor");
const FName UBTService_UpdateTargetInfo::DoorImpactLocationKey = TEXT("DoorImpactLocation");

UBTService_UpdateTargetInfo::UBTService_UpdateTargetInfo()
{
    NodeName = TEXT("Update Target Info");
}

uint16 UBTService_UpdateTargetInfo::GetInstanceMemorySize() const
{
    return sizeof(FInstanceMemory);
}

void UBTService_UpdateTargetInfo::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

    FInstanceMemory* Mem = reinterpret_cast<FInstanceMemory*>(NodeMemory);
    if (!Mem) return;

    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    if (!BB) return;

    AAIController* AICon = Cast<AAIController>(OwnerComp.GetAIOwner());
    if (!AICon) return;

    APawn* Pawn = AICon->GetPawn();
    if (!Pawn) return;

    UWorld* World = Pawn->GetWorld();
    if (!World) return;

    // helpers to avoid unnecessary blackboard writes (which can interrupt MoveTo)
    auto SetBoolIfChanged = [&](const FName& Key, bool NewVal)
    {
        bool Cur = BB->GetValueAsBool(Key);
        if (Cur != NewVal)
        {
            BB->SetValueAsBool(Key, NewVal);
        }
    };
    auto SetObjectIfChanged = [&](const FName& Key, UObject* NewObj)
    {
        UObject* Cur = BB->GetValueAsObject(Key);
        if (Cur != NewObj)
        {
            if (NewObj)
                BB->SetValueAsObject(Key, NewObj);
            else
                BB->ClearValue(Key);
        }
    };
    auto SetVectorIfChanged = [&](const FName& Key, const FVector& NewVec, float Tolerance = 10.0f)
    {
        FVector Cur = BB->GetValueAsVector(Key);
        if (!Cur.Equals(NewVec, Tolerance))
        {
            BB->SetValueAsVector(Key, NewVec);
        }
    };

    UAIPerceptionComponent* Perc = Pawn->FindComponentByClass<UAIPerceptionComponent>();
    TArray<AActor*> PerceivedActors;
    if (Perc)
    {
        Perc->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), PerceivedActors);
    }

    Aggj_maskCharacter* SeenPlayer = nullptr;
    for (AActor* A : PerceivedActors)
    {
        if (A && A->IsA(Aggj_maskCharacter::StaticClass()))
        {
            SeenPlayer = Cast<Aggj_maskCharacter>(A);
            break;
        }
    }

    FVector PawnLoc = Pawn->GetActorLocation();

    bool bCandidateBlocked = false;
    FVector CandidateDoorPoint = FVector::ZeroVector;

    if (SeenPlayer)
    {
        // Only write when changed to avoid interrupting MoveTo
        SetBoolIfChanged(HasTargetKey, true);
        SetObjectIfChanged(TargetActorKey, SeenPlayer);
        FVector PlayerLoc = SeenPlayer->GetActorLocation();
        SetVectorIfChanged(LastSeenLocationKey, PlayerLoc, 5.0f);

        // Raycast to player
        FHitResult Hit;
        FCollisionQueryParams Params;
        Params.AddIgnoredActor(Pawn);
        Params.AddIgnoredActor(SeenPlayer);

        bool bHit = World->LineTraceSingleByChannel(Hit, PawnLoc, PlayerLoc, ECC_Visibility, Params);
        if (bHit)
        {
            AActor* HitActor = Hit.GetActor();
            if (HitActor && HitActor->IsA(AMonitorDoor::StaticClass()))
            {
                FVector Impact = Hit.ImpactPoint;
                FVector Dir = (PawnLoc - Impact).GetSafeNormal();
                CandidateDoorPoint = Impact + Dir * BackOffDistance;
                bCandidateBlocked = true;
            }
        }
    }
    else
    {
        // Player not currently visible
        bool bPrevHasTarget = BB->GetValueAsBool(HasTargetKey);
        if (bPrevHasTarget)
        {
            FVector LastSeen = BB->GetValueAsVector(LastSeenLocationKey);
            if (!LastSeen.IsNearlyZero())
            {
                FHitResult Hit;
                FCollisionQueryParams Params;
                Params.AddIgnoredActor(Pawn);

                bool bHit = World->LineTraceSingleByChannel(Hit, PawnLoc, LastSeen, ECC_Visibility, Params);
                if (bHit)
                {
                    AActor* HitActor = Hit.GetActor();
                    if (HitActor && HitActor->IsA(AMonitorDoor::StaticClass()))
                    {
                        FVector Impact = Hit.ImpactPoint;
                        FVector Dir = (PawnLoc - Impact).GetSafeNormal();
                        CandidateDoorPoint = Impact + Dir * BackOffDistance;
                        bCandidateBlocked = true;
                    }
                    else
                    {
                        // Lost player, clear target and blocked
                        SetBoolIfChanged(HasTargetKey, false);
                        SetObjectIfChanged(TargetActorKey, nullptr);
                        // reset memory
                        Mem->BlockedAccum = 0.0f;
                        Mem->bLastBlocked = false;
                        SetBoolIfChanged(IsBlockedByDoorKey, false);
                        return;
                    }
                }
                else
                {
                    SetBoolIfChanged(HasTargetKey, false);
                    SetObjectIfChanged(TargetActorKey, nullptr);
                    Mem->BlockedAccum = 0.0f;
                    Mem->bLastBlocked = false;
                    SetBoolIfChanged(IsBlockedByDoorKey, false);
                    return;
                }
            }
            else
            {
                SetBoolIfChanged(HasTargetKey, false);
                SetObjectIfChanged(TargetActorKey, nullptr);
                Mem->BlockedAccum = 0.0f;
                Mem->bLastBlocked = false;
                SetBoolIfChanged(IsBlockedByDoorKey, false);
                return;
            }
        }
    }

    // Debounce blocked state changes to avoid flicker
    if (bCandidateBlocked)
    {
        Mem->BlockedAccum += DeltaSeconds;
        if (!Mem->bLastBlocked && Mem->BlockedAccum >= BlockedDebounceTime)
        {
            Mem->bLastBlocked = true;
            SetBoolIfChanged(IsBlockedByDoorKey, true);
            SetVectorIfChanged(DoorImpactLocationKey, CandidateDoorPoint, 10.0f);
        }
        else if (Mem->bLastBlocked)
        {
            // already blocked, keep updating door point if it drifts a bit
            SetVectorIfChanged(DoorImpactLocationKey, CandidateDoorPoint, 10.0f);
        }
    }
    else
    {
        // candidate not blocked -> decay accum and when below threshold, clear blocked
        Mem->BlockedAccum -= DeltaSeconds;
        if (Mem->BlockedAccum <= 0.0f)
        {
            Mem->BlockedAccum = 0.0f;
            if (Mem->bLastBlocked)
            {
                Mem->bLastBlocked = false;
                SetBoolIfChanged(IsBlockedByDoorKey, false);
            }
        }
    }
}
