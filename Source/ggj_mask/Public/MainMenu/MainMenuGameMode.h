#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MainMenuGameMode.generated.h"

UCLASS()
class GGJ_MASK_API AMainMenuGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AMainMenuGameMode();

protected:
    virtual void BeginPlay() override;
    
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Main Menu")
    TSubclassOf<class UMainMenuWidget> MainMenuWidgetClass;
    
    UPROPERTY()
    class UMainMenuWidget* MainMenuWidget;

    UPROPERTY(EditAnywhere, Category = "Audio")
    USoundBase* BackgroundMusic;
};