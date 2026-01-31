#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MainMenuPlayerController.generated.h"

UCLASS()
class GGJ_MASK_API AMainMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Main Menu")
	void StartGame();

	UFUNCTION(BlueprintCallable, Category = "Main Menu")
	void QuitGame();

	// 要加载的游戏地图名称
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Settings")
	FName GameLevelName = "GameLevel";

	// 摄像机标签 - 在编辑器中设置
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	FName MainMenuCameraTag = "MainMenuCamera";

protected:
	// 通过标签查找摄像机
	AActor* FindMainMenuCameraByTag();

private:
	AActor* MainMenuCamera;
};