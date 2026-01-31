#include "Enemies/PatrolSpline.h"
#include "Components/SplineComponent.h"

APatrolSpline::APatrolSpline()
{
	PrimaryActorTick.bCanEverTick = false;
	SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));
	RootComponent = SplineComponent;
	SplineComponent->bShouldVisualizeScale = true;
}

int32 APatrolSpline::GetNumPoints() const
{
	if (!SplineComponent) return 0;
	return SplineComponent->GetNumberOfSplinePoints();
}

FVector APatrolSpline::GetPointLocation(int32 Index) const
{
	if (!SplineComponent) return FVector::ZeroVector;
	int32 Num = SplineComponent->GetNumberOfSplinePoints();
	if (Num == 0) return FVector::ZeroVector;
	int32 Clamped = FMath::Clamp(Index, 0, Num - 1);
	return SplineComponent->GetLocationAtSplinePoint(Clamped, ESplineCoordinateSpace::World);
}

int32 APatrolSpline::GetClosestPointIndex(const FVector& WorldLocation) const
{
	if (!SplineComponent) return INDEX_NONE;
	int32 Num = SplineComponent->GetNumberOfSplinePoints();
	if (Num == 0) return INDEX_NONE;
	float InputKey = SplineComponent->FindInputKeyClosestToWorldLocation(WorldLocation);
	int32 PointIndex = FMath::RoundToInt(InputKey);
	return (PointIndex % Num + Num) % Num;
}

