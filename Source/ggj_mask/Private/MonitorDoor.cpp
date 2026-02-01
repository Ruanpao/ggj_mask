// Fill out your copyright notice in the Description page of Project Settings.


#include "MonitorDoor.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "ggj_mask/ggj_maskCharacter.h"
#include "Engine/Engine.h"
#include "Math/UnrealMathUtility.h" 
#include "Enemies/Enemy.h" // allow checking enemy properties

AMonitorDoor::AMonitorDoor()
{
	PrimaryActorTick.bCanEverTick = true;

	// 创建根组件
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	
	// 创建碰撞盒组件
	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	TriggerBox->SetupAttachment(RootComponent);
	TriggerBox->SetBoxExtent(FVector(100.0f, 100.0f, 100.0f));

	DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
	DoorMesh->SetupAttachment(RootComponent);

	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AMonitorDoor::OnTriggerBeginOverlap);
}

void AMonitorDoor::BeginPlay()
{
	Super::BeginPlay();
	
	OriginalDoorHeight = DoorMesh->GetRelativeLocation().Z;
	TargetHeight = OriginalDoorHeight;
}

void AMonitorDoor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	UpdateDoorMovement(DeltaTime);
}

void AMonitorDoor::OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Player opening door via mask
	if(Aggj_maskCharacter* PlayerCharacter = Cast<Aggj_maskCharacter>(OtherActor))
	{
		if(PlayerCharacter->IsWearOpenDoorMask())
		{
			OpenDoor();
			
		}
		return; // handled
	}

	// Enemy opening door if permitted
	if (AEnemy* EnemyActor = Cast<AEnemy>(OtherActor))
	{
		if (EnemyActor->CanOpenDoor)
		{
			OpenDoor();
		}
	}
}

void AMonitorDoor::OpenDoor()
{
	if(bIsClosing || bIsOpening)
	{
		return;
	}
	TargetHeight = OpenHeight;
	bIsOpening = true;
	bIsClosing = false;

	GetWorld()->GetTimerManager().SetTimer(DoorTimerHandle, this, &AMonitorDoor::CloseDoor, OpenDuration, false);
}

void AMonitorDoor::CloseDoor()
{
	if(bIsClosing||bIsOpening)
	{
		return;
	}

	TargetHeight = OriginalDoorHeight;
	bIsClosing = true;
	bIsOpening = false;
}

void AMonitorDoor::UpdateDoorMovement(float DeltaTime)
{
	if(!bIsClosing && !bIsOpening)
	{
		return;
	}

	FVector CurrentLocation = DoorMesh->GetRelativeLocation();
	float CurrentHeight = CurrentLocation.Z;

	// 计算移动方向和距离
	float MoveDirection = (TargetHeight > CurrentHeight) ? 1.0f : -1.0f;
	float MoveAmount = DoorSpeed * DeltaTime * MoveDirection;
	float NewHeight = CurrentHeight + MoveAmount;

	// 检查是否到达目标高度
	bool bReachedTarget = false;
	
	if (MoveDirection > 0 && NewHeight >= TargetHeight) // 向上移动到达目标
	{
		NewHeight = TargetHeight;
		bReachedTarget = true;
	}
	else if (MoveDirection < 0 && NewHeight <= TargetHeight) // 向下移动到达目标
	{
		NewHeight = TargetHeight;
		bReachedTarget = true;
	}
	
	DoorMesh->SetRelativeLocation(FVector(CurrentLocation.X, CurrentLocation.Y, NewHeight));
	
	// 如果到达目标高度
	if (bReachedTarget)
	{
		if (bIsOpening) // 刚完成开门
		{
			bIsOpening = false;
			// 确保计时器已经设置，如果没有则设置
			if (!GetWorld()->GetTimerManager().IsTimerActive(DoorTimerHandle))
			{
				GetWorld()->GetTimerManager().SetTimer(DoorTimerHandle, this, &AMonitorDoor::CloseDoor, OpenDuration, false);
			}
		}
		else if (bIsClosing) // 刚完成关门
		{
			bIsClosing = false;
			// 清除计时器
			GetWorld()->GetTimerManager().ClearTimer(DoorTimerHandle);
		}
	}
}

