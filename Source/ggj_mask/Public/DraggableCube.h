// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DraggableCube.generated.h"

UCLASS()
class GGJ_MASK_API ADraggableCube : public AActor
{
	GENERATED_BODY()
	
public:	
	ADraggableCube();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	void StartDragging(AActor* Dragger);

	void StopDragging();

	void UpdateDragging(const FVector& TargetPosition);

	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category = "Components")
	class UStaticMeshComponent* CubeMesh;

	UPROPERTY(BlueprintReadOnly,Category = "Dragging")
	AActor* DraggingActor;

	UPROPERTY(BlueprintReadOnly,Category = "Dragging")
	bool bIsBeingDragged = false;
private:
	FVector DragOffset;
};
