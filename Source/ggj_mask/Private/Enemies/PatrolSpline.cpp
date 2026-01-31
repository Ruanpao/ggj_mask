#include "Enemies/PatrolSpline.h"
#include "Components/SplineComponent.h"

APatrolSpline::APatrolSpline()
{
	PrimaryActorTick.bCanEverTick = false;
	SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));
	RootComponent = SplineComponent;
}

int32 APatrolSpline::GetNumPoints() const
{
	return SplineComponent ? SplineComponent->GetNumberOfSplinePoints() : 0;
}

FVector APatrolSpline::GetPointLocation(int32 Index) const
{
	if (!SplineComponent) return FVector::ZeroVector;
	int32 Num = SplineComponent->GetNumberOfSplinePoints();
	if (Num == 0 || Index < 0 || Index >= Num) return FVector::ZeroVector;
	return SplineComponent->GetLocationAtSplinePoint(Index, ESplineCoordinateSpace::World);
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
