// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EndDoor.generated.h"

class Aggj_maskCharacter;

UCLASS()
class GGJ_MASK_API AEndDoor : public AActor
{
	GENERATED_BODY()
	
public: 	
	// Sets default values for this actor's properties
	AEndDoor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public: 
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Trigger box that detects player overlap
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trigger")
	UBoxComponent* TriggerBox;

	// Visual mesh for the door
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visual")
	UStaticMeshComponent* MeshComp;

	// Overlap handler
	UFUNCTION()
	void OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor,
		class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);

};
