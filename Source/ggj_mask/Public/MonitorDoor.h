// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MonitorDoor.generated.h"

UCLASS()
class GGJ_MASK_API AMonitorDoor : public AActor
{
	GENERATED_BODY()
	
public:	
	AMonitorDoor();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Components")
	class UStaticMeshComponent* DoorMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UBoxComponent* TriggerBox;
	
	FTimerHandle DoorTimerHandle;
	
	float OriginalDoorHeight;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door Settings")
	float OpenHeight = 500.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door Settings")
	float OpenDuration = 5.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door Settings")
	float DoorSpeed = 200.0f;
	
	bool bIsOpening = false;
	
	bool bIsClosing = false;
	
	float TargetHeight;

private:
	UFUNCTION()
	void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);

	void OpenDoor();

	void CloseDoor();

	void UpdateDoorMovement(float DeltaTime);
};
