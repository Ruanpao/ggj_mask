// Fill out your copyright notice in the Description page of Project Settings.


#include "DraggableCube.h"
#include "Components/StaticMeshComponent.h"

ADraggableCube::ADraggableCube()
{

	PrimaryActorTick.bCanEverTick = true;

	// 创建根组件
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// 创建方块网格
	CubeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CubeMesh"));
	CubeMesh->SetupAttachment(RootComponent);
}


void ADraggableCube::BeginPlay()
{
	Super::BeginPlay();

}


void ADraggableCube::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 调试绘制轨道
	if (bUseTrack && TrackPoints.Num() >= 2)
	{
		for (int32 i = 0; i < TrackPoints.Num() - 1; i++)
		{
			DrawDebugLine(
				GetWorld(),
				TrackPoints[i],
				TrackPoints[i + 1],
				FColor::Green,
				false,
				-1.0f,
				0,
				2.0f
			);

			// 绘制轨道点
			DrawDebugSphere(
				GetWorld(),
				TrackPoints[i],
				10.0f,
				8,
				FColor::Red,
				false,
				-1.0f,
				0,
				2.0f
			);
		}

		// 绘制最后一个点
		if (TrackPoints.Num() > 0)
		{
			DrawDebugSphere(
				GetWorld(),
				TrackPoints.Last(),
				10.0f,
				8,
				FColor::Red,
				false,
				-1.0f,
				0,
				2.0f
			);
		}
	}
}

void ADraggableCube::StartDragging(AActor* Dragger)
{
	if(!Dragger || bIsBeingDragged)
	{
		return;
	}

	bIsBeingDragged = true;
	DraggingActor = Dragger;

	DragStartPosition = GetActorLocation();
	DragStartHeight = GetActorLocation().Z;

	if(bUseTrack && TrackPoints.Num() >= 2)
	{
		CurrentTrackSegmentIndex = FindNearestSegmentIndex(DragStartPosition);
	}
}

void ADraggableCube::StopDragging()
{
	if(!bIsBeingDragged)
	{
		return;
	}

	bIsBeingDragged = false;
	DraggingActor = nullptr;

	CubeMesh->SetRenderCustomDepth(false);
}

void ADraggableCube::UpdateDragging(const FVector& TargetPosition)
{
	if(!bIsBeingDragged || !DraggingActor)
	{
		return;
	}

	FVector NewLocation = FVector(
		TargetPosition.X,
		TargetPosition.Y,
		DragStartHeight
	);

	if (bUseTrack && TrackPoints.Num() >= 2)
	{
		// 先计算投影位置
		FVector ProjectedLocation = ProjectToTrack(NewLocation);

		// 添加位置平滑过渡
		FVector CurrentLocation = GetActorLocation();
		float MaxMoveDistance = 500.0f;

		if(FVector::Dist(CurrentLocation, ProjectedLocation) > MaxMoveDistance)
		{
			// 如果距离过大，使用插值平滑移动
			FVector Direction = (ProjectedLocation - CurrentLocation).GetSafeNormal();
			NewLocation = CurrentLocation + Direction * MaxMoveDistance;
		}
		else
		{
			NewLocation = ProjectedLocation;
		}
	}
	SetActorLocation(NewLocation);
}

void ADraggableCube::SetTrackPoints(const TArray<FVector>& Points)
{
	TrackPoints = Points;
	bUseTrack = (Points.Num() >= 2);
	CurrentTrackSegmentIndex = -1;
}

void ADraggableCube::ConnectToNearestTrack()
{
}

FVector ADraggableCube::ProjectToTrack(const FVector& Position)
{
	if(!bUseTrack || TrackPoints.Num() < 2)
	{
		return Position;
	}

	FVector NearestPoint = FindNearestPointOnSegment(Position);

	float DistanceToTrack = FVector::Dist(Position,NearestPoint);

	if(DistanceToTrack > TrackRadius)
	{
		return FVector(Position.X,Position.Y,DragStartHeight);
	}
	return FVector(NearestPoint.X,NearestPoint.Y,DragStartHeight);
}

// 找到轨道上最近的点
FVector ADraggableCube::FindNearestPointOnTrack(const FVector& Position)
{
	if(TrackPoints.Num() <2)
	{
		return Position;
	}

	FVector NearestPoint = TrackPoints[0];
	float MinDistance = FLT_MAX;
	int32 NearestIndex = 0;

	for(int32 i =0;i < TrackPoints.Num() -1;i++)
	{
		FVector PointOnSegment = FMath::ClosestPointOnSegment(Position,TrackPoints[i],TrackPoints[i+1]);
		float Distance = FVector::Dist(Position,PointOnSegment);
		if(Distance < MinDistance)
		{
			MinDistance = Distance;
			NearestPoint = PointOnSegment;
			NearestIndex = i;
		}
	}

	CurrentTrackSegmentIndex = NearestIndex;
	return NearestPoint;
}

// 找到线段上最近的点
FVector ADraggableCube::FindNearestPointOnSegment(const FVector& Position)
{
	FVector NearestPoint;
	float MinDistance = FLT_MAX;
	int32 NearestSegmentIndex = -1;

	for(int32 i = 0; i < TrackPoints.Num() - 1; i++)
	{
		FVector PointOnSegment = FMath::ClosestPointOnSegment(Position, TrackPoints[i], TrackPoints[i + 1]);
		float Distance = FVector::Dist(Position, PointOnSegment);
		if(Distance < MinDistance)
		{
			MinDistance = Distance;
			NearestPoint = PointOnSegment;
			NearestSegmentIndex = i;
		}
	}

	if(NearestSegmentIndex != -1)
	{
		CurrentTrackSegmentIndex = NearestSegmentIndex;
	}

	return NearestPoint;
}

// 找到最近的轨道段索引
int32 ADraggableCube::FindNearestSegmentIndex(const FVector& Position)
{
	if(TrackPoints.Num() < 2)
	{
		return -1;
	}

	int32 NearestSegmentIndex = 0;
	float MinDistance = FLT_MAX;

	for(int32 i = 0; i < TrackPoints.Num() - 1; i++)
	{
		FVector PointOnSegment = FMath::ClosestPointOnSegment(Position,TrackPoints[i],TrackPoints[i+1]);
		float Distance = FVector::Dist(Position,PointOnSegment);
		if(Distance < MinDistance)
		{
			MinDistance = Distance;
			NearestSegmentIndex = i;
		}
	}
	return NearestSegmentIndex;
}