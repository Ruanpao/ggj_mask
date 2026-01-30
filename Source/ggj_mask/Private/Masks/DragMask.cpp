// Fill out your copyright notice in the Description page of Project Settings.


#include "Masks/DragMask.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

ADragMask::ADragMask()
{
 	
	PrimaryActorTick.bCanEverTick = true;

	// 创建根组件
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	
	// 创建网格组件
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
}

void ADragMask::BeginPlay()
{
	Super::BeginPlay();
	
}

void ADragMask::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ADragMask::OnInteract_Implementation(AActor* Interactor)
{
	IInteractInterface::OnInteract_Implementation(Interactor);
	Destroy();
}

