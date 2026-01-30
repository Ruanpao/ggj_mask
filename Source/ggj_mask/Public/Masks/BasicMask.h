// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ggj_mask/Public/InteractInterface.h"

#include "BasicMask.generated.h"

UCLASS()
class GGJ_MASK_API ABasicMask : public AActor, public IInteractInterface
{
	GENERATED_BODY()
	
public:
	// Sets default values for this actor's properties
	ABasicMask();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	virtual void OnInteract_Implementation(AActor* Interactor) override;

};
