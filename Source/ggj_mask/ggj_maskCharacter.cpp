// Copyright Epic Games, Inc. All Rights Reserved.

#include "ggj_maskCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "InteractInterface.h"
#include "ggj_mask/Public/Masks/BasicMask.h"
#include "ggj_mask/Public/Masks/BeSmallMask.h"
#include "ggj_mask/Public/Masks/OpenDoorMask.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// Aggj_maskCharacter

Aggj_maskCharacter::Aggj_maskCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->bUsePawnControlRotation = false;
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	CameraBoom->TargetArmLength = 1000.0f; 
	//CameraBoom->SetUsingAbsoluteRotation(true); 
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->bUsePawnControlRotation = false;
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	
	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

void Aggj_maskCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	
}

//////////////////////////////////////////////////////////////////////////
// Input

void Aggj_maskCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// // Jumping
		// EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		// EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &Aggj_maskCharacter::Move);

		EnhancedInputComponent->BindAction(WearMask1, ETriggerEvent::Started, this, &Aggj_maskCharacter::WearBasicMask);

		EnhancedInputComponent->BindAction(WearMask2, ETriggerEvent::Started, this, &Aggj_maskCharacter::WearSmallMask);

		EnhancedInputComponent->BindAction(WearMask3, ETriggerEvent::Started, this, &Aggj_maskCharacter::WearOpenDoorMask);

		EnhancedInputComponent->BindAction(ApplySkillAction, ETriggerEvent::Started, this, &Aggj_maskCharacter::ApplySkill);

		EnhancedInputComponent->BindAction(PickUpAction, ETriggerEvent::Started, this, &Aggj_maskCharacter::PickUp);
		// Looking
		// EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &Aggj_maskCharacter::Look);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void Aggj_maskCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void Aggj_maskCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void Aggj_maskCharacter::WearBasicMask(const FInputActionValue& Value)
{
	UE_LOG(LogTemplateCharacter, Error, TEXT("Wear Basic Mask"));
	bWearBasicMask = true;
	bWearSmallMask = false;
	bWearOpenDoorMask = false;
}

void Aggj_maskCharacter::WearSmallMask(const FInputActionValue& Value)
{
	if(!bGetSmallMask)
	{
		return;
	}
	UE_LOG(LogTemplateCharacter, Error, TEXT("Wear Small Mask"));
	bWearBasicMask = false;
	bWearSmallMask = true;
	bWearOpenDoorMask = false;
}

void Aggj_maskCharacter::WearOpenDoorMask(const FInputActionValue& Value)
{
	if(!bGetOpenDoorMask)
	{
		return;
	}
	UE_LOG(LogTemplateCharacter, Error, TEXT("Wear OpenDoor Mask"));
	bWearBasicMask = false;
	bWearSmallMask = false;
	bWearOpenDoorMask = true;
}

void Aggj_maskCharacter::ApplySkill(const FInputActionValue& Value)
{
	
	if (bWearBasicMask)
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("Apply Basic Mask Skill"));
	}
	else if (bWearSmallMask)
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("Apply Small Mask Skill"));
		
		if(!bSmall)
		{
			SetActorScale3D(FVector(0.1f, 0.1f, 0.1f));
			bSmall = true;
		}
		else
		{
			SetActorScale3D(FVector(1.f, 1.f, 1.f));
			bSmall = false;
		}
	}
	else if (bWearOpenDoorMask)
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("Apply OpenDoor Mask Skill"));
	}
}

void Aggj_maskCharacter::PickUp(const FInputActionValue& Value)
{
	AActor* InteractableActor = nullptr;
	if(!Controller)
	{
		return;
	}
	
	FVector SphereCenter = GetActorLocation()  + (GetActorForwardVector() * 100.f);
	float SphereRadius = 100.0f;

	
	DrawDebugSphere(GetWorld(), SphereCenter, SphereRadius, 16, FColor::Green, false, 2.f);
	
	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	
	if(GetWorld()->SweepSingleByChannel(HitResult, SphereCenter, SphereCenter, FQuat::Identity, 
		ECC_Visibility, FCollisionShape::MakeSphere(SphereRadius), Params))
	{
		AActor* HitActor = HitResult.GetActor();

		if(HitActor && HitActor->GetClass()->ImplementsInterface(UInteractInterface::StaticClass()))
		{
			InteractableActor = HitActor;
			
			DrawDebugPoint(GetWorld(), HitResult.Location, 10.0f, FColor::Red, false, 2.f);
		}
	}

	if(InteractableActor)
	{
		if(InteractableActor->GetClass()->ImplementsInterface(UInteractInterface::StaticClass()))
		{
			IInteractInterface::Execute_OnInteract(InteractableActor, this);
			
			if (ABeSmallMask* SmallMask = Cast<ABeSmallMask>(InteractableActor))
			{
				// 拾取到变小面具
				bGetSmallMask = true;
				UE_LOG(LogTemplateCharacter, Warning, TEXT("获得变小面具"));
			}
			else if (AOpenDoorMask* OpenDoorMask = Cast<AOpenDoorMask>(InteractableActor))
			{
				// 拾取到开门面具
				bGetOpenDoorMask = true;
				UE_LOG(LogTemplateCharacter, Warning, TEXT("获得开门面具"));
			}
			else
			{
				UE_LOG(LogTemplateCharacter, Warning, TEXT("未拾取到"));
			}
		}
	}
}
