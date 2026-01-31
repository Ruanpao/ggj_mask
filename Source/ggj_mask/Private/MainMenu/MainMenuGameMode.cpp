#include "MainMenu/MainMenuGameMode.h"
#include "MainMenu/MainMenuWidget.h"
#include "MainMenu/MainMenuPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

AMainMenuGameMode::AMainMenuGameMode()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AMainMenuGameMode::BeginPlay()
{
    Super::BeginPlay();
    
    UE_LOG(LogTemp, Warning, TEXT("MainMenuGameMode BeginPlay called"));
    
    // 创建并显示主菜单UI
    if (MainMenuWidgetClass)
    {
        APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
        if (PlayerController)
        {
            MainMenuWidget = CreateWidget<UMainMenuWidget>(PlayerController, MainMenuWidgetClass);
            if (MainMenuWidget)
            {
                MainMenuWidget->AddToViewport();
                
                // 设置输入模式为UI Only
                PlayerController->bShowMouseCursor = true;
                FInputModeUIOnly InputMode;
                InputMode.SetWidgetToFocus(MainMenuWidget->TakeWidget());
                InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
                PlayerController->SetInputMode(InputMode);
                
                UE_LOG(LogTemp, Warning, TEXT("MainMenuWidget created and added to viewport"));
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to create MainMenuWidget"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("No PlayerController found"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("MainMenuWidgetClass is not set"));
    }
}