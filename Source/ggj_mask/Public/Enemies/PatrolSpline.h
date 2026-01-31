// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SplineComponent.h"
#include "PatrolSpline.generated.h"

class USplineComponent;

UCLASS()
class GGJ_MASK_API APatrolSpline : public AActor
{
	GENERATED_BODY()

public:
	APatrolSpline();

	// Spline component for patrol path
	UPROPERTY(VisibleAnywhere, Category = "Spline")
	USplineComponent* SplineComponent;

	int32 GetNumPoints() const;
	FVector GetPointLocation(int32 Index) const;
	int32 GetClosestPointIndex(const FVector& WorldLocation) const;
};
