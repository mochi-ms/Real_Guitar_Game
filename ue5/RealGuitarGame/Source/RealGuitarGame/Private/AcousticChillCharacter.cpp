#include "AcousticChillCharacter.h"

#include "Blueprint/UserWidget.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GuitarPerformanceHUDWidget.h"
#include "GuitarPerformerActor.h"
#include "GuitarScoreSubsystem.h"
#include "GuitarSongLibrarySubsystem.h"
#include "InteractableSeatActor.h"

AAcousticChillCharacter::AAcousticChillCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
    FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
    FirstPersonCamera->SetRelativeLocation(FVector(0.0f, 0.0f, 64.0f));
    FirstPersonCamera->bUsePawnControlRotation = true;
}

void AAcousticChillCharacter::BeginPlay()
{
    Super::BeginPlay();

    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Sub =
                ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
        {
            if (DefaultMappingContext)
            {
                Sub->AddMappingContext(DefaultMappingContext, 0);
            }
        }
        if (PerformanceHUDClass)
        {
            HUDWidget = CreateWidget<UGuitarPerformanceHUDWidget>(PC, PerformanceHUDClass);
            if (HUDWidget)
            {
                HUDWidget->AddToViewport();
            }
        }
    }
}

void AAcousticChillCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!HUDWidget)
    {
        return;
    }

    if (ActiveSeat)
    {
        HUDWidget->SetInteractionPrompt(true, NSLOCTEXT("GuitarHUD", "StandPrompt", "[E] 자리에서 일어나기 · [C] 카메라 전환"));
    }
    else if (const AInteractableSeatActor* LookedSeat = FindLookedAtSeat())
    {
        HUDWidget->SetInteractionPrompt(true, LookedSeat->SitPromptText);
    }
    else
    {
        HUDWidget->SetInteractionPrompt(false, FText::GetEmpty());
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
        if (CycleCameraAction)
        {
            EnhancedInput->BindAction(CycleCameraAction, ETriggerEvent::Started, this, &AAcousticChillCharacter::HandleCycleCamera);
        }
        if (NextSongAction)
        {
            EnhancedInput->BindAction(NextSongAction, ETriggerEvent::Started, this, &AAcousticChillCharacter::HandleNextSong);
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

void AAcousticChillCharacter::HandleCycleCamera()
{
    if (!ActiveSeat || !ActiveSeat->LinkedPerformer)
    {
        return;
    }
    ActiveSeat->LinkedPerformer->CycleCameraAngle();
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        PC->SetViewTargetWithBlend(ActiveSeat->LinkedPerformer, 0.55f, EViewTargetBlendFunction::VTBlend_Cubic);
    }
}

void AAcousticChillCharacter::HandleNextSong()
{
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UGuitarSongLibrarySubsystem* Lib = GI->GetSubsystem<UGuitarSongLibrarySubsystem>())
        {
            Lib->PlayNextSong();
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
        if (HUDWidget)
        {
            HUDWidget->SetSeatedMode(false, nullptr);
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
        if (HUDWidget)
        {
            HUDWidget->SetSeatedMode(true, Seat->LinkedPerformer);
        }
    }
}
