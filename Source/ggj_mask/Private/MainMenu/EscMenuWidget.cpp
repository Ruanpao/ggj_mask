// Fill out your copyright notice in the Description page of Project Settings.


#include "MainMenu/EscMenuWidget.h"

void UEscMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetPause(true);
		FInputModeUIOnly InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock); // 关键：不锁定鼠标
		// InputMode.SetHideCursorDuringCapture(false); // 关键：捕获时不隐藏光标
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = true;
	}

	// 绑定按钮点击事件
	if (StartGameButton)
	{
		StartGameButton->OnClicked.AddDynamic(this, &UEscMenuWidget::OnStartGameClicked);
		UE_LOG(LogTemp, Warning, TEXT("StartGameButton bound successfully"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("StartGameButton is null!"));
	}

	if (QuitGameButton)
	{
		QuitGameButton->OnClicked.AddDynamic(this, &UEscMenuWidget::OnQuitGameClicked);
		UE_LOG(LogTemp, Warning, TEXT("QuitGameButton bound successfully"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("QuitGameButton is null!"));
	}

}

void UEscMenuWidget::NativeDestruct()
{
	Super::NativeDestruct();

	// Unpause the game when this menu is removed
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetPause(false);
	}
}

void UEscMenuWidget::StartGame()
{
	OnStartGameClicked();

}

void UEscMenuWidget::QuitGame()
{
	OnQuitGameClicked();

}

void UEscMenuWidget::OnStartGameClicked()
{

	PlayButtonSound();
	UE_LOG(LogTemp, Warning, TEXT("FUCK"));
	SetColorAndOpacity(FLinearColor(1, 1, 1, 0));

	SetColorAndOpacity(FLinearColor(1, 1, 1, 0));

	// Ensure menu is removed and game is unpaused before loading level
	if (APlayerController* PC = GetOwningPlayer())
	{
		// Unpause the game if paused
		if (PC->IsPaused())
		{
			PC->SetPause(false);
		}
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock); // 关键：不锁定鼠标
		InputMode.SetHideCursorDuringCapture(false); // 关键：捕获时不隐藏光标
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = true;
	}
	//RemoveFromParent();

}

void UEscMenuWidget::OnQuitGameClicked()
{
	PlayButtonSound();
	UE_LOG(LogTemp, Warning, TEXT("Start Game button clicked"));
	SetColorAndOpacity(FLinearColor(1, 1, 1, 0));

	SetColorAndOpacity(FLinearColor(1, 1, 1, 0));

	// Ensure menu is removed and game is unpaused before loading level
	if (APlayerController* PC = GetOwningPlayer())
	{
		// Unpause the game if paused
		if (PC->IsPaused())
		{
			PC->SetPause(true);
		}
	}

	// Remove this widget so NativeDestruct runs (which also unpauses as a safety)
	RemoveFromParent();

	// Load the ThirdPersonMap level
	UE_LOG(LogTemp, Warning, TEXT("MainMenu: Loading level 'MainMenu'"));
	UGameplayStatics::OpenLevel(this, FName(TEXT("MainMenu")));
}

void UEscMenuWidget::PlayButtonSound()
{
	if (ButtonClickSound)
	{
		UGameplayStatics::PlaySound2D(GetWorld(), ButtonClickSound);
	}
}
