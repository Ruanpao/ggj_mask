// Copyright Epic Games, Inc. All Rights Reserved.

#include "ggj_maskGameMode.h"
#include "ggj_maskCharacter.h"
#include "UObject/ConstructorHelpers.h"

Aggj_maskGameMode::Aggj_maskGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
