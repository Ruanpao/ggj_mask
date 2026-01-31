// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DraggableCube.generated.h"

UCLASS()
class GGJ_MASK_API ADraggableCube : public AActor
{
	GENERATED_BODY()
	
public:	
	ADraggableCube();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	void StartDragging(AActor* Dragger);
	void StopDragging();
	void UpdateDragging(const FVector& TargetPosition);

	//设置轨道
	UFUNCTION(BlueprintCallable,Category = "Track")
	void SetTrackPoints(const TArray<FVector>& Points);

	//自动链接轨道
	UFUNCTION(BlueprintCallable,Category = "Track")
	void ConnectToNearestTrack();
	
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category = "Components")
	class UStaticMeshComponent* CubeMesh;

	UPROPERTY(BlueprintReadOnly,Category = "Dragging")
	AActor* DraggingActor;

	UPROPERTY(BlueprintReadOnly,Category = "Dragging")
	bool bIsBeingDragged = false;

	//轨道点
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Track")
	TArray<FVector> TrackPoints;

	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Track")
	bool bUseTrack = true;

	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Track")
	float TrackRadius = 50.f;
private:
	FVector DragStartPosition;
	float DragStartHeight;

	//将位置投影到最近的轨道上
	FVector ProjectToTrack(const FVector& Position);
	
	//找到轨道上最近的点
	FVector FindNearestPointOnTrack(const FVector& Position);
	
	//找到最近两个轨道点直接的线段
	FVector FindNearestPointOnSegment(const FVector& Position);

	//找到最近的轨道线段索引
	int32 FindNearestSegmentIndex(const FVector& Position);
	
	//当前轨道线段索引
	int32 CurrentTrackSegmentIndex = -1;
};
