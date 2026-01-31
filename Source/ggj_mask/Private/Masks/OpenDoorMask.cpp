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

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Script/Engine.StaticMesh'/Game/model/water_mask/blue.blue'"));
	if (CubeMesh.Succeeded())
	{
		UE_LOG(LogTemp, Warning, TEXT("成功加载水面具模型: %s"), *CubeMesh.Object->GetName());
		MeshComponent->SetStaticMesh(CubeMesh.Object);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("无法加载水面具模型"));
	}
	
	// 确保组件可见
	MeshComponent->SetVisibility(true);
	MeshComponent->SetHiddenInGame(false);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
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
