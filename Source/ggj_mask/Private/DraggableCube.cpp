// Fill out your copyright notice in the Description page of Project Settings.


#include "DraggableCube.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

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

	if (!bUseTrack || TrackPoints.Num() < 2)
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

		if(!MoveSafely(ProjectedLocation))
		{
			PushPlayerAway();
			return;
		}
		
		// 添加位置平滑过渡
		FVector CurrentLocation = GetActorLocation();
		float MaxMoveDistance = 30.0f;

		if(FVector::Dist(CurrentLocation, ProjectedLocation) > MaxMoveDistance)
		{
			// 如果距离过大，使用插值平滑移动
			FVector Direction = (ProjectedLocation - CurrentLocation).GetSafeNormal();
			NewLocation = CurrentLocation + Direction * MaxMoveDistance;
		}
		else
		{
			NewLocation = ProjectedLocation;

			if (!MoveSafely(NewLocation))
			{
				PushPlayerAway();
				return;
			}
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

void ADraggableCube::PushPlayerAway()
{
	// 冷却检查
	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastPushTime < PushCooldown)
	{
		return;
	}
    
	LastPushTime = CurrentTime;
    
	if (!DraggingActor) return;
    
	// 获取玩家角色
	ACharacter* PlayerCharacter = Cast<ACharacter>(DraggingActor);
	if (!PlayerCharacter) return;
    
	// 计算推动方向（从方块指向玩家）
	FVector CubeLocation = GetActorLocation();
	FVector PlayerLocation = PlayerCharacter->GetActorLocation();
	FVector PushDirection = (PlayerLocation - CubeLocation).GetSafeNormal();
    
	// 向上稍微推一点，避免玩家被压在地面上
	PushDirection.Z += 0.00001f;
	PushDirection.Normalize();
    
	// 应用推动力
	FVector PushForceVector = PushDirection * PushForce;
    
	// 使用AddImpulse或AddForce推动玩家
	UCharacterMovementComponent* MovementComp = PlayerCharacter->GetCharacterMovement();
	if (MovementComp)
	{
		// 先清除当前速度
		MovementComp->Velocity = FVector::ZeroVector;
        
		// 应用推动力
		MovementComp->AddImpulse(PushForceVector, true);
        
		// 限制最大速度
		MovementComp->MaxWalkSpeed = 3000.0f;
        
		// 0.5秒后恢复原速度
		FTimerHandle TimerHandle;
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, [MovementComp]()
		{
			if (MovementComp)
			{
				MovementComp->MaxWalkSpeed = 700.0f; // 恢复默认速度
			}
		}, 0.5f, false);
	}
	
    
	UE_LOG(LogTemp, Warning, TEXT("推动玩家，方向: %s"), *PushDirection.ToString());
}

bool ADraggableCube::CheckPlayerCollision(const FVector& NewLocation)
{
	if(!DraggingActor)
	{
		return false;
	}

	FVector PlayerLocation = DraggingActor->GetActorLocation();

	FVector CubeLocation = GetActorLocation();
	FVector MoveDirection = (NewLocation - CubeLocation).GetSafeNormal();

	// 计算方块前进的方向上是否会碰到玩家
	FVector ToPlayer = (PlayerLocation - CubeLocation).GetSafeNormal();
	float DotProduct = FVector::DotProduct(MoveDirection, ToPlayer);
    
	// 如果玩家在方块移动方向上
	if (DotProduct > 0.7f) // 0.7表示大约45度角内
	{
		// 计算距离
		float DistanceToPlayer = FVector::Dist(CubeLocation, PlayerLocation);
        
		// 如果距离足够近，认为会碰撞
		if (DistanceToPlayer < PlayerCheckRadius)
		{
			// 绘制调试信息
			DrawDebugSphere(GetWorld(), PlayerLocation, 50.0f, 12, FColor::Red, false, 0.1f);
			DrawDebugLine(GetWorld(), CubeLocation, PlayerLocation, FColor::Red, false, 0.1f, 0, 2.0f);
            
			return true;
		}
	}
    
	return false;
}

bool ADraggableCube::MoveSafely(const FVector& TargetLocation)
{
	// 首先检查是否会碰到玩家
	if (CheckPlayerCollision(TargetLocation))
	{
		return false;
	}
	return true;
}
