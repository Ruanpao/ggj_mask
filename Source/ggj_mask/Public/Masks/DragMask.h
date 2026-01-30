// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractInterface.h"
#include "DragMask.generated.h"

UCLASS()
class GGJ_MASK_API ADragMask : public AActor , public IInteractInterface
{
	GENERATED_BODY()
	
public:	
	ADragMask();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;
	virtual void OnInteract_Implementation(AActor* Interactor) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* MeshComponent;
};
