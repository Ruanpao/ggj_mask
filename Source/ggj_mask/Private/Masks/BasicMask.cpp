// Fill out your copyright notice in the Description page of Project Settings.


#include "ggj_mask/Public/Masks/BasicMask.h"

// Sets default values
ABasicMask::ABasicMask()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// 创建根组件
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	
	// 创建静态网格组件
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
}

// Called when the game starts or when spawned
void ABasicMask::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ABasicMask::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Default native implementation of the interaction. Subclasses should override Interact_Implementation (or OnInteract_Implementation) to provide behavior.
void ABasicMask::OnInteract_Implementation(AActor* Interactor)
{
	// Default behavior: log and do nothing. Override this in subclasses to implement actual interaction logic.
	UE_LOG(LogTemp, Log, TEXT("ABasicMask::OnInteract_Implementation called by %s"), *GetNameSafe(Interactor));
}
