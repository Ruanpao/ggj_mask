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

}

void ADraggableCube::StartDragging(AActor* Dragger)
{
	if(!Dragger || bIsBeingDragged)
	{
		return;
	}

	bIsBeingDragged = true;
	DraggingActor = Dragger;

	DragOffset = GetActorLocation() - Dragger->GetActorLocation();

	CubeMesh->SetRenderCustomDepth(true);
	CubeMesh->SetCustomDepthStencilValue(1);
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

	FVector NewLocation = TargetPosition;

	NewLocation.Z = FMath::Min(NewLocation.Z, 50.0f);

	SetActorLocation(NewLocation);
}

