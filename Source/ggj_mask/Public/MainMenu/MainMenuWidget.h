#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Kismet/GameplayStatics.h"
#include "MainMenuWidget.generated.h"

UCLASS()
class GGJ_MASK_API UMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category = "Main Menu")
	void StartGame();

	UFUNCTION(BlueprintCallable, Category = "Main Menu")
	void QuitGame();

protected:
	// 主菜单界面组件
	UPROPERTY(meta = (BindWidget))
	class UButton* StartGameButton;

	UPROPERTY(meta = (BindWidget))
	class UButton* QuitGameButton;

	UPROPERTY(meta = (BindWidget))
	class UImage* BackgroundImage;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* TitleText;

	// 按钮点击声音
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	class USoundBase* ButtonClickSound;


	UFUNCTION()
	void OnStartGameClicked();

	UFUNCTION()
	void OnQuitGameClicked();

	void PlayButtonSound();

	// === 新增代码开始 ===
	// 动画相关变量
	UPROPERTY()
	AActor* RotatingActor;
    
	UPROPERTY()
	AActor* CameraActor;
    
	bool bIsAnimating;
	float AnimationTime;
	float TotalAnimationTime;
	float CameraStartDistance;
	float CameraTargetDistance;
    
	void StartAnimation();
	void UpdateAnimation(float DeltaTime);
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;
	// === 新增代码结束 ===
};