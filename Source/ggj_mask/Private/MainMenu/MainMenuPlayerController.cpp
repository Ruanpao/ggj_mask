#include "MainMenu/MainMenuPlayerController.h"
#include "Camera/CameraActor.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

void AMainMenuPlayerController::BeginPlay()
{
    Super::BeginPlay();
    
    // 通过标签查找主菜单摄像机
    MainMenuCamera = FindMainMenuCameraByTag();
    
    // 设置视图目标为主菜单摄像机
    if (MainMenuCamera)
    {
        SetViewTarget(MainMenuCamera);
        UE_LOG(LogTemp, Warning, TEXT("Main menu camera set as view target"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Main menu camera not found with tag: %s"), *MainMenuCameraTag.ToString());
    }
    
    // 显示鼠标光标
    bShowMouseCursor = true;
    
    // 设置UI输入模式
    FInputModeUIOnly InputMode;
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(InputMode);
}

AActor* AMainMenuPlayerController::FindMainMenuCameraByTag()
{
    if (MainMenuCameraTag.IsNone())
    {
        UE_LOG(LogTemp, Warning, TEXT("MainMenuCameraTag is not set"));
        return nullptr;
    }
    
    TArray<AActor*> FoundCameras;
    UGameplayStatics::GetAllActorsWithTag(GetWorld(), MainMenuCameraTag, FoundCameras);
    
    if (FoundCameras.Num() > 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("Found %d cameras with tag: %s"), FoundCameras.Num(), *MainMenuCameraTag.ToString());
        return FoundCameras[0]; // 返回第一个找到的摄像机
    }
    
    // 如果没有找到标签匹配的摄像机，尝试查找任何摄像机
    TArray<AActor*> AllCameras;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACameraActor::StaticClass(), AllCameras);
    
    if (AllCameras.Num() > 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("Using first available camera actor"));
        return AllCameras[0];
    }
    
    UE_LOG(LogTemp, Error, TEXT("No cameras found in the level"));
    return nullptr;
}

void AMainMenuPlayerController::StartGame()
{
    UE_LOG(LogTemp, Warning, TEXT("Starting game... Loading level: %s"), *GameLevelName.ToString());
    
    // 加载游戏主地图
    UGameplayStatics::OpenLevel(GetWorld(), GameLevelName);
    
    // 恢复游戏输入模式
    bShowMouseCursor = false;
    FInputModeGameOnly InputMode;
    SetInputMode(InputMode);
}

void AMainMenuPlayerController::QuitGame()
{
    UE_LOG(LogTemp, Warning, TEXT("Quitting game..."));
    
    // 退出游戏
    UKismetSystemLibrary::QuitGame(GetWorld(), this, EQuitPreference::Quit, false);
}