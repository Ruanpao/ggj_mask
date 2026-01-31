// Copyright Epic Games, Inc. All Rights Reserved.

#include "ggj_maskGameMode.h"
#include "ggj_maskCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "GameFramework/PlayerInput.h"
#include "InputCoreTypes.h"

Aggj_maskGameMode::Aggj_maskGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}

void Aggj_maskGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		APlayerController* PC = World->GetFirstPlayerController();
		if (PC)
		{
			// Ensure input mode is GameOnly so keyboard/mouse control is enabled for gameplay
			FInputModeGameOnly InputMode;
			PC->SetInputMode(InputMode);
			PC->bShowMouseCursor = true
			;
			UE_LOG(LogTemp, Log, TEXT("Aggj_maskGameMode::BeginPlay - input mode set to GameOnly, mouse cursor shown."));
		}
	}
}
