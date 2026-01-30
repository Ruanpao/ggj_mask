// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemies/EnemyBeSmallPoint.h"
#include "Enemies/Enemy.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/Engine.h"
#include "TimerManager.h" // Added to delay restore of scale

// Sets default values
AEnemyBeSmallPoint::AEnemyBeSmallPoint()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	OverlapBox = CreateDefaultSubobject<UBoxComponent>(TEXT("OverlapBox"));
	OverlapBox->InitBoxExtent(FVector(200.f, 200.f, 100.f));
	OverlapBox->SetCollisionProfileName(TEXT("OverlapAll"));
	OverlapBox->SetGenerateOverlapEvents(true);
	OverlapBox->SetMobility(EComponentMobility::Static);
	// Use the custom channel selected in editor (BeSmallCollisionChannel)
	OverlapBox->SetCollisionObjectType(BeSmallCollisionChannel);
	// Only overlap, don't block - don't affect navigation
	OverlapBox->SetCollisionResponseToAllChannels(ECR_Overlap);

	RootComponent = OverlapBox;

	OverlapBox->OnComponentBeginOverlap.AddDynamic(this, &AEnemyBeSmallPoint::OnOverlapBegin);
	OverlapBox->OnComponentEndOverlap.AddDynamic(this, &AEnemyBeSmallPoint::OnOverlapEnd);

}

// Called when the game starts or when spawned
void AEnemyBeSmallPoint::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AEnemyBeSmallPoint::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AEnemyBeSmallPoint::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	if (!OtherActor) return;

	// Only affect AEnemy actors
	AEnemy* Enemy = Cast<AEnemy>(OtherActor);
	if (!Enemy) return;

	TWeakObjectPtr<AActor> Key = Enemy;

	int32& Count = OverlapCounts.FindOrAdd(Key);
	Count++;

	// First time entering any BeSmallPoint: store original scale and set to 50%
	if (Count == 1)
	{
		OriginalScales.Add(Key, Enemy->GetActorScale3D());
		Enemy->SetActorScale3D(Enemy->GetActorScale3D() * 0.5f);
		// Optional debug
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("Enemy entered BeSmallPoint: scaled to 50%"));
		}
	}
}

void AEnemyBeSmallPoint::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!OtherActor) return;

	AEnemy* Enemy = Cast<AEnemy>(OtherActor);
	if (!Enemy) return;

	TWeakObjectPtr<AActor> Key = Enemy;

	int32* CountPtr = OverlapCounts.Find(Key);
	if (!CountPtr) return;

	(*CountPtr)--;
	if (*CountPtr <= 0)
	{
		// Remove the overlap count mapping first
		OverlapCounts.Remove(Key);

		// Find original scale if stored
		FVector* OrigScalePtr = OriginalScales.Find(Key);
		if (OrigScalePtr)
		{
			// Copy value and remove stored scale immediately to avoid dangling pointers
			FVector OrigScale = *OrigScalePtr;
			OriginalScales.Remove(Key);

			// Use a weak pointer to the enemy and defer the SetActorScale3D call to avoid modifying
			// actor scale while the engine is in the middle of updating overlaps (prevents re-entrancy)
			TWeakObjectPtr<AEnemy> WeakEnemy = Enemy;

			FTimerDelegate RestoreDelegate = FTimerDelegate::CreateLambda([WeakEnemy, OrigScale]() {
				if (AEnemy* E = WeakEnemy.Get())
				{
					E->SetActorScale3D(OrigScale);
					if (GEngine)
					{
						GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, TEXT("Enemy left BeSmallPoint: scale restored"));
					}
				}
			});

			FTimerHandle Handle;
			if (GetWorld())
			{
				// Small delay to ensure we're out of the overlap update callstack
				GetWorld()->GetTimerManager().SetTimer(Handle, RestoreDelegate, 0.01f, false);
			}
		}
	}
}
