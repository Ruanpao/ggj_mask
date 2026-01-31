// Fill out your copyright notice in the Description page of Project Settings.


#include "ggj_mask/Public/Masks/BeSmallMask.h"

ABeSmallMask::ABeSmallMask()
{
	PrimaryActorTick.bCanEverTick = true;

	// 创建根组件
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	
	// 创建静态网格组件
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);


	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Script/Engine.StaticMesh'/Game/model/flame_mask/flame.flame'"));
	if (CubeMesh.Succeeded())
	{
		MeshComponent->SetStaticMesh(CubeMesh.Object);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("无法加载火焰面具模型"));
	}
	
	// 确保组件可见
	MeshComponent->SetVisibility(true);
	MeshComponent->SetHiddenInGame(false);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	
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
