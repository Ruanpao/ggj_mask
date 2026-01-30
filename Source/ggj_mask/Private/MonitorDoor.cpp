// Fill out your copyright notice in the Description page of Project Settings.


#include "MonitorDoor.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "ggj_mask/ggj_maskCharacter.h"
#include "Engine/Engine.h"
#include "Math/UnrealMathUtility.h" 

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
	if(Aggj_maskCharacter* PlayerCharacter = Cast<Aggj_maskCharacter>(OtherActor))
	{
		if(PlayerCharacter->IsWearOpenDoorMask())
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

	float MoveDirection;
	if (TargetHeight > CurrentHeight)
	{
		MoveDirection = 1.0f;
	}
	else if (TargetHeight < CurrentHeight)
	{
		MoveDirection = -1.0f;
	}
	else
	{
		MoveDirection = 0.0f; 
		return;
	}
	float MoveAmount = DoorSpeed * DeltaTime * MoveDirection;
	float NewHeight = CurrentHeight + MoveAmount;

	if(MoveDirection> 0 && NewHeight >= TargetHeight)
	{
		NewHeight = TargetHeight;
		bIsOpening = false;
	}
	else if(MoveDirection < 0 && NewHeight <= TargetHeight)
	{
		NewHeight = TargetHeight;
		bIsClosing = false;
	}
	DoorMesh->SetRelativeLocation(FVector(CurrentLocation.X, CurrentLocation.Y, NewHeight));
}

