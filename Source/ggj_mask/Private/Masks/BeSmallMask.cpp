// Fill out your copyright notice in the Description page of Project Settings.


#include "Masks/BeSmallMask.h"

ABeSmallMask::ABeSmallMask()
{
	PrimaryActorTick.bCanEverTick = true;

	// 创建根组件
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	
	// 创建静态网格组件
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
}

void ABeSmallMask::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ABeSmallMask::OnInteract_Implementation(AActor* Interactor)
{
	IInteractInterface::OnInteract_Implementation(Interactor);
	Destroy();
}

void ABeSmallMask::BeginPlay()
{
	Super::BeginPlay();
	
}
