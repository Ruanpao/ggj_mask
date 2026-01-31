// Fill out your copyright notice in the Description page of Project Settings.


#include "MainMenu/EndDoor.h"
#include "Kismet/GameplayStatics.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "ggj_mask/ggj_maskCharacter.h"

// Sets default values
AEndDoor::AEndDoor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
 	PrimaryActorTick.bCanEverTick = true;

 	// Create root scene component
 	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

 	// Trigger box
 	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
 	TriggerBox->SetupAttachment(RootComponent);
 	TriggerBox->SetBoxExtent(FVector(200.0f, 200.0f, 200.0f));
 	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
 	TriggerBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
 	TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECollisionResponse::ECR_Overlap);
 	TriggerBox->SetGenerateOverlapEvents(true);

 	// Visual mesh
 	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
 	MeshComp->SetupAttachment(RootComponent);
 	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

}

// Called when the game starts or when spawned
void AEndDoor::BeginPlay()
{
 	Super::BeginPlay();

 	if (TriggerBox)
 	{
 		TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AEndDoor::OnOverlapBegin);
 	}
}

// Called every frame
void AEndDoor::Tick(float DeltaTime)
{
 	Super::Tick(DeltaTime);

}

void AEndDoor::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
 	class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
 	if (!OtherActor) return;

 	// Check if it's the player character
 	Aggj_maskCharacter* Player = Cast<Aggj_maskCharacter>(OtherActor);
 	if (Player)
 	{
 		// Open the level named EndMap
 		UGameplayStatics::OpenLevel(this, FName(TEXT("EndMap")));
 	}
}
