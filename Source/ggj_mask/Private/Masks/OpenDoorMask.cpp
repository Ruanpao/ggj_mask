// Fill out your copyright notice in the Description page of Project Settings.


#include "Masks/OpenDoorMask.h"

AOpenDoorMask::AOpenDoorMask()
{
	PrimaryActorTick.bCanEverTick = true;

	// 创建根组件
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	
	// 创建静态网格组件
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
}

void AOpenDoorMask::BeginPlay()
{
	Super::BeginPlay();
}

void AOpenDoorMask::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AOpenDoorMask::OnInteract_Implementation(AActor* Interactor)
{
	IInteractInterface::OnInteract_Implementation(Interactor);
	Destroy();
}
