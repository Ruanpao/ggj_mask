#include "Mainmenu/MainMenuWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "MainMenu/MainMenuPlayerController.h"
#include "EngineUtils.h" // 新增include
#include "Camera/CameraActor.h"
#include "GameFramework/PlayerController.h" // for SetPause

void UMainMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();

    UE_LOG(LogTemp, Warning, TEXT("MainMenuWidget NativeConstruct called"));

    // Pause the game when this menu appears
    if (APlayerController* PC = GetOwningPlayer())
    {
        PC->SetPause(true);
    }

    // 绑定按钮点击事件
    if (StartGameButton)
    {
        StartGameButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnStartGameClicked);
        UE_LOG(LogTemp, Warning, TEXT("StartGameButton bound successfully"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("StartGameButton is null!"));
    }

    if (QuitGameButton)
    {
        QuitGameButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnQuitGameClicked);
        UE_LOG(LogTemp, Warning, TEXT("QuitGameButton bound successfully"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("QuitGameButton is null!"));
    }


    bIsAnimating = false;
    AnimationTime = 0.0f;
    TotalAnimationTime = 3.0f; // 3秒动画
    CameraStartDistance = 0.0f;
    CameraTargetDistance = 2500.0f; // 摄像机前飞500单位
    
}

void UMainMenuWidget::NativeDestruct()
{
    Super::NativeDestruct();

    // Unpause the game when this menu is removed
    if (APlayerController* PC = GetOwningPlayer())
    {
        PC->SetPause(false);
    }
}

void UMainMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (bIsAnimating)
    {
        UpdateAnimation(InDeltaTime);
    }
}

void UMainMenuWidget::StartAnimation()
{
    if (bIsAnimating) return;

    UWorld* World = GetWorld();
    if (!World) return;
}



void UMainMenuWidget::UpdateAnimation(float DeltaTime)
{

}
// === 新增代码结束 ===

void UMainMenuWidget::StartGame()
{
    UE_LOG(LogTemp, Warning, TEXT("Start Game called via function"));
    OnStartGameClicked();
}

void UMainMenuWidget::QuitGame()
{
    UE_LOG(LogTemp, Warning, TEXT("Quit Game called via function"));
    OnQuitGameClicked();
}

void UMainMenuWidget::OnStartGameClicked()
{
    PlayButtonSound();
    UE_LOG(LogTemp, Warning, TEXT("Start Game button clicked"));
    SetColorAndOpacity(FLinearColor(1, 1, 1, 0));

    SetColorAndOpacity(FLinearColor(1, 1, 1, 0));
    
    StartAnimation(); 

    // Ensure menu is removed and game is unpaused before loading level
    if (APlayerController* PC = GetOwningPlayer())
    {
        // Unpause the game if paused
        if (PC->IsPaused())
        {
            PC->SetPause(false);
        }
    }

    // Remove this widget so NativeDestruct runs (which also unpauses as a safety)
    RemoveFromParent();

    // Load the ThirdPersonMap level
    UE_LOG(LogTemp, Warning, TEXT("MainMenu: Loading level 'ThirdPersonMap'"));
    UGameplayStatics::OpenLevel(this, FName(TEXT("ThirdPersonMap")));
}

void UMainMenuWidget::OnQuitGameClicked()
{
    PlayButtonSound();
    UE_LOG(LogTemp, Warning, TEXT("Quit Game button clicked"));

    // 获取PlayerController并调用退出游戏
    APlayerController* PlayerController = GetOwningPlayer();
    if (AMainMenuPlayerController* MenuPC = Cast<AMainMenuPlayerController>(PlayerController))
    {
        MenuPC->QuitGame();
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to cast to MainMenuPlayerController"));
        // 备用方案：直接退出游戏
        UKismetSystemLibrary::QuitGame(GetWorld(), PlayerController, EQuitPreference::Quit, false);
    }
}

void UMainMenuWidget::PlayButtonSound()
{
    if (ButtonClickSound)
    {
        UGameplayStatics::PlaySound2D(GetWorld(), ButtonClickSound);
    }
}