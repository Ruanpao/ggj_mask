// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemyBeSmallPoint.generated.h"

class UBoxComponent;
class AEnemy;

UCLASS()
class GGJ_MASK_API AEnemyBeSmallPoint : public AActor
{
	GENERATED_BODY()
	
public:
	// Sets default values for this actor's properties
	AEnemyBeSmallPoint();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Overlap box used for the "BeSmall" area
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BeSmall")
	UBoxComponent* OverlapBox;

	// Collision channel to use for this box (set to your custom BeSmall channel in editor if needed)
	UPROPERTY(EditAnywhere, Category = "BeSmall")
	TEnumAsByte<ECollisionChannel> BeSmallCollisionChannel = ECC_GameTraceChannel1;

	// Track original scales for actors when they first enter
	TMap<TWeakObjectPtr<AActor>, FVector> OriginalScales;

	// Track overlap counts so multiple points don't prematurely restore scale
	TMap<TWeakObjectPtr<AActor>, int32> OverlapCounts;

	// Overlap handlers
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

public: 	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
