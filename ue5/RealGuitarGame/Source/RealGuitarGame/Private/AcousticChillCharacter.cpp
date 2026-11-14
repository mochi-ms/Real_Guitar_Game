#include "AcousticChillCharacter.h"

#include "Blueprint/UserWidget.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GuitarGameSettings.h"
#include "GuitarPerformanceHUDWidget.h"
#include "GuitarPerformerActor.h"
#include "GuitarScoreSubsystem.h"
#include "GuitarSongLibrarySubsystem.h"
#include "InputCoreTypes.h"
#include "InteractableSeatActor.h"
#include "RealGuitarMenuWidget.h"

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

    if (UGameInstance* GI = GetGameInstance())
    {
        if (UGuitarGameSettingsSubsystem* SettingsSub = GI->GetSubsystem<UGuitarGameSettingsSubsystem>())
        {
            SettingsSub->OnSettingsChanged.AddDynamic(this, &AAcousticChillCharacter::HandleSettingsChanged);
            ApplyControlSettings(SettingsSub->GetSettings());
        }
    }

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

        UClass* ResolvedHUDClass = PerformanceHUDClass ? PerformanceHUDClass.Get() : UGuitarPerformanceHUDWidget::StaticClass();
        HUDWidget = CreateWidget<UGuitarPerformanceHUDWidget>(PC, ResolvedHUDClass);
        if (HUDWidget)
        {
            HUDWidget->AddToViewport(1);
            HUDWidget->OnRequestPauseMenu.AddDynamic(this, &AAcousticChillCharacter::HandleOpenPauseFromHUD);
        }

        UClass* ResolvedMenuClass = MenuWidgetClass ? MenuWidgetClass.Get() : URealGuitarMenuWidget::StaticClass();
        MenuWidget = CreateWidget<URealGuitarMenuWidget>(PC, ResolvedMenuClass);
        if (MenuWidget)
        {
            MenuWidget->AddToViewport(10);
            MenuWidget->OnMenuClosed.AddDynamic(this, &AAcousticChillCharacter::HandleMenuClosed);
            if (bShowTitleScreenOnBeginPlay)
            {
                if (HUDWidget)
                {
                    HUDWidget->SetVisibility(ESlateVisibility::Collapsed);
                }
                MenuWidget->OpenMenu(ERealGuitarMenuMode::TitleScreen, 0);
            }
            else
            {
                MenuWidget->SetVisibility(ESlateVisibility::Collapsed);
            }
        }
    }
}

void AAcousticChillCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UGuitarGameSettingsSubsystem* SettingsSub = GI->GetSubsystem<UGuitarGameSettingsSubsystem>())
        {
            SettingsSub->OnSettingsChanged.RemoveDynamic(this, &AAcousticChillCharacter::HandleSettingsChanged);
        }
    }
    Super::EndPlay(EndPlayReason);
}

void AAcousticChillCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!HUDWidget || (MenuWidget && MenuWidget->IsMenuOpen()))
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
        if (PauseMenuAction)
        {
            EnhancedInput->BindAction(PauseMenuAction, ETriggerEvent::Started, this, &AAcousticChillCharacter::HandlePauseMenu);
        }
        if (ToggleHudAction)
        {
            EnhancedInput->BindAction(ToggleHudAction, ETriggerEvent::Started, this, &AAcousticChillCharacter::HandleToggleHud);
        }
        if (CycleMoodAction)
        {
            EnhancedInput->BindAction(CycleMoodAction, ETriggerEvent::Started, this, &AAcousticChillCharacter::HandleCycleMood);
        }
    }

    // Fallback direct bindings so core controls work immediately before InputAction assets are wired
    if (PlayerInputComponent)
    {
        if (!PauseMenuAction)
        {
            PlayerInputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AAcousticChillCharacter::HandlePauseMenu);
        }
        if (!ToggleHudAction)
        {
            PlayerInputComponent->BindKey(EKeys::H, IE_Pressed, this, &AAcousticChillCharacter::HandleToggleHud);
        }
        if (!CycleMoodAction)
        {
            PlayerInputComponent->BindKey(EKeys::T, IE_Pressed, this, &AAcousticChillCharacter::HandleCycleMood);
        }
        if (!InteractAction)
        {
            PlayerInputComponent->BindKey(EKeys::E, IE_Pressed, this, &AAcousticChillCharacter::HandleInteract);
        }
        if (!TogglePlayAction)
        {
            PlayerInputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &AAcousticChillCharacter::HandleTogglePlay);
        }
        if (!CycleCameraAction)
        {
            PlayerInputComponent->BindKey(EKeys::C, IE_Pressed, this, &AAcousticChillCharacter::HandleCycleCamera);
        }
        if (!NextSongAction)
        {
            PlayerInputComponent->BindKey(EKeys::N, IE_Pressed, this, &AAcousticChillCharacter::HandleNextSong);
        }
    }
}

void AAcousticChillCharacter::HandleMove(const FInputActionValue& Value)
{
    if (ActiveSeat != nullptr || (MenuWidget && MenuWidget->IsMenuOpen()))
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
    if (ActiveSeat != nullptr || (MenuWidget && MenuWidget->IsMenuOpen()))
    {
        return;
    }
    const FVector2D Look = Value.Get<FVector2D>();
    AddControllerYawInput(Look.X * LookSensitivity);
    AddControllerPitchInput(Look.Y * LookSensitivity * (bInvertLookY ? -1.0f : 1.0f));
}

void AAcousticChillCharacter::HandleInteract()
{
    if (MenuWidget && MenuWidget->IsMenuOpen()) return;
    TryToggleSeat();
}

void AAcousticChillCharacter::HandleTogglePlay()
{
    if (MenuWidget && MenuWidget->IsMenuOpen()) return;
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
    if (MenuWidget && MenuWidget->IsMenuOpen()) return;
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
    if (MenuWidget && MenuWidget->IsMenuOpen()) return;
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UGuitarSongLibrarySubsystem* Lib = GI->GetSubsystem<UGuitarSongLibrarySubsystem>())
        {
            Lib->PlayNextSong();
        }
    }
}

void AAcousticChillCharacter::HandlePauseMenu()
{
    if (!MenuWidget)
    {
        return;
    }
    if (MenuWidget->IsMenuOpen())
    {
        if (MenuWidget->GetCurrentMode() == ERealGuitarMenuMode::PauseMenu)
        {
            MenuWidget->CloseMenu();
        }
        return;
    }

    if (HUDWidget)
    {
        HUDWidget->SetVisibility(ESlateVisibility::Collapsed);
    }
    MenuWidget->OpenMenu(ERealGuitarMenuMode::PauseMenu, 0);
}

void AAcousticChillCharacter::HandleOpenPauseFromHUD()
{
    HandlePauseMenu();
}

void AAcousticChillCharacter::HandleMenuClosed()
{
    if (HUDWidget)
    {
        HUDWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        HUDWidget->SetSeatedMode(ActiveSeat != nullptr, ActiveSeat ? ActiveSeat->LinkedPerformer.Get() : nullptr);
    }
    else if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        PC->SetShowMouseCursor(ActiveSeat != nullptr);
        PC->SetInputMode(FInputModeGameOnly());
    }
}

void AAcousticChillCharacter::HandleToggleHud()
{
    if (HUDWidget && !(MenuWidget && MenuWidget->IsMenuOpen()))
    {
        HUDWidget->ToggleCinematicHud();
    }
}

void AAcousticChillCharacter::HandleCycleMood()
{
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UGuitarGameSettingsSubsystem* SettingsSub = GI->GetSubsystem<UGuitarGameSettingsSubsystem>())
        {
            SettingsSub->CycleAtmosphereMood();
        }
    }
}

void AAcousticChillCharacter::HandleSettingsChanged(const FGuitarSettingsData& NewSettings)
{
    ApplyControlSettings(NewSettings);
}

void AAcousticChillCharacter::ApplyControlSettings(const FGuitarSettingsData& Settings)
{
    LookSensitivity = Settings.MouseSensitivity;
    bInvertLookY = Settings.bInvertY;
    if (FirstPersonCamera)
    {
        FirstPersonCamera->SetFieldOfView(Settings.FieldOfView);
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
