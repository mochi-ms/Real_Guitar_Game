#include "AcousticChillCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GuitarScoreSubsystem.h"
#include "InteractableSeatActor.h"

AAcousticChillCharacter::AAcousticChillCharacter()
{
    PrimaryActorTick.bCanEverTick = false;

    FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
    FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
    FirstPersonCamera->SetRelativeLocation(FVector(0.0f, 0.0f, 64.0f));
    FirstPersonCamera->bUsePawnControlRotation = true;
}

void AAcousticChillCharacter::BeginPlay()
{
    Super::BeginPlay();

    if (const APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Sub =
                ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
        {
            if (DefaultMappingContext)
            {
                Sub->AddMappingContext(DefaultMappingContext, 0);
            }
        }
    }
}

void AAcousticChillCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        if (MoveAction)
        {
            EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AAcousticChillCharacter::HandleMove);
        }
        if (LookAction)
        {
            EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &AAcousticChillCharacter::HandleLook);
        }
        if (InteractAction)
        {
            EnhancedInput->BindAction(InteractAction, ETriggerEvent::Started, this, &AAcousticChillCharacter::HandleInteract);
        }
        if (TogglePlayAction)
        {
            EnhancedInput->BindAction(TogglePlayAction, ETriggerEvent::Started, this, &AAcousticChillCharacter::HandleTogglePlay);
        }
    }
}

void AAcousticChillCharacter::HandleMove(const FInputActionValue& Value)
{
    if (ActiveSeat != nullptr)
    {
        return;
    }
    const FVector2D Axis = Value.Get<FVector2D>();
    if (Controller && !Axis.IsNearlyZero())
    {
        AddMovementInput(GetActorForwardVector(), Axis.Y);
        AddMovementInput(GetActorRightVector(), Axis.X);
    }
}

void AAcousticChillCharacter::HandleLook(const FInputActionValue& Value)
{
    if (ActiveSeat != nullptr)
    {
        return;
    }
    const FVector2D Look = Value.Get<FVector2D>();
    AddControllerYawInput(Look.X);
    AddControllerPitchInput(Look.Y);
}

void AAcousticChillCharacter::HandleInteract()
{
    TryToggleSeat();
}

void AAcousticChillCharacter::HandleTogglePlay()
{
    if (UWorld* World = GetWorld())
    {
        if (UGuitarScoreSubsystem* ScoreSub = World->GetSubsystem<UGuitarScoreSubsystem>())
        {
            if (ScoreSub->IsPlaying())
            {
                ScoreSub->Pause();
            }
            else
            {
                ScoreSub->Play();
            }
        }
    }
}

AInteractableSeatActor* AAcousticChillCharacter::FindLookedAtSeat() const
{
    const UWorld* World = GetWorld();
    if (!World || !FirstPersonCamera)
    {
        return nullptr;
    }

    FHitResult Hit;
    const FVector Start = FirstPersonCamera->GetComponentLocation();
    const FVector End = Start + FirstPersonCamera->GetForwardVector() * InteractTraceDistance;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(SeatTrace), false, this);

    if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
    {
        if (AInteractableSeatActor* Seat = Cast<AInteractableSeatActor>(Hit.GetActor()))
        {
            return Seat;
        }
    }

    TArray<AActor*> Overlapping;
    GetOverlappingActors(Overlapping, AInteractableSeatActor::StaticClass());
    return Overlapping.Num() > 0 ? Cast<AInteractableSeatActor>(Overlapping[0]) : nullptr;
}

void AAcousticChillCharacter::TryToggleSeat()
{
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC)
    {
        return;
    }

    if (ActiveSeat != nullptr)
    {
        ActiveSeat->ExitSeat(PC, this);
        ActiveSeat = nullptr;
        if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
        {
            MoveComp->SetMovementMode(MOVE_Walking);
        }
        return;
    }

    if (AInteractableSeatActor* Seat = FindLookedAtSeat())
    {
        ActiveSeat = Seat;
        if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
        {
            MoveComp->StopMovementImmediately();
            MoveComp->DisableMovement();
        }
        Seat->EnterSeat(PC);
    }
}
