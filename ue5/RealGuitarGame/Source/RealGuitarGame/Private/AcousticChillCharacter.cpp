#include "AcousticChillCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EngineUtils.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "GuitarCharacterAnimInstance.h"
#include "GuitarFretboardComponent.h"
#include "GuitarGameSettings.h"
#include "GuitarPerformerActor.h"
#include "GuitarPerformanceHUDWidget.h"
#include "GuitarScoreSubsystem.h"
#include "GuitarSongLibrarySubsystem.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "InteractableSeatActor.h"
#include "RealGuitarMenuWidget.h"

AAcousticChillCharacter::AAcousticChillCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    GetCapsuleComponent()->InitCapsuleSize(38.0f, 88.0f);

    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 480.0f, 0.0f);
    GetCharacterMovement()->MaxWalkSpeed = 240.0f;
    GetCharacterMovement()->JumpZVelocity = 420.0f;
    GetCharacterMovement()->AirControl = 0.35f;

    // Load UE5 Mannequins & Locomotion AnimBP from Content/Characters/Mannequins
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> QuinnFinder(
        TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
    if (QuinnFinder.Succeeded())
    {
        CachedQuinnMesh = QuinnFinder.Object;
    }

    static ConstructorHelpers::FObjectFinder<USkeletalMesh> MannyFinder(
        TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
    if (MannyFinder.Succeeded())
    {
        CachedMannyMesh = MannyFinder.Object;
    }

    static ConstructorHelpers::FClassFinder<UAnimInstance> UnarmedAnimFinder(
        TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"));
    if (UnarmedAnimFinder.Succeeded())
    {
        CachedUnarmedAnimClass = UnarmedAnimFinder.Class;
    }

    USkeletalMesh* InitialMesh = CachedQuinnMesh ? CachedQuinnMesh.Get() : CachedMannyMesh.Get();

    // Hidden locomotion driver evaluates ABP_Unarmed smoothly even behind IK overlay
    LocomotionDriverMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("LocomotionDriverMesh"));
    LocomotionDriverMesh->SetupAttachment(GetCapsuleComponent());
    LocomotionDriverMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -88.0f));
    LocomotionDriverMesh->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
    LocomotionDriverMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    LocomotionDriverMesh->SetRenderInMainPass(false);
    LocomotionDriverMesh->SetRenderInDepthPass(false);
    LocomotionDriverMesh->SetCastShadow(false);
    LocomotionDriverMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    if (InitialMesh)
    {
        LocomotionDriverMesh->SetSkeletalMeshAsset(InitialMesh);
    }
    if (CachedUnarmedAnimClass)
    {
        LocomotionDriverMesh->SetAnimInstanceClass(CachedUnarmedAnimClass);
    }

    // Visible skeletal mesh uses our custom Two-Bone IK guitarist AnimInstance
    if (USkeletalMeshComponent* VisibleMesh = GetMesh())
    {
        VisibleMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -88.0f));
        VisibleMesh->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
        VisibleMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
        VisibleMesh->SetCastShadow(true);
        if (InitialMesh)
        {
            VisibleMesh->SetSkeletalMeshAsset(InitialMesh);
        }
        VisibleMesh->SetAnimInstanceClass(UGuitarCharacterAnimInstance::StaticClass());
    }

    // 3rd-person orbit camera (default)
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(GetCapsuleComponent());
    CameraBoom->TargetArmLength = 260.0f;
    CameraBoom->SocketOffset = FVector(0.0f, 34.0f, 52.0f);
    CameraBoom->bUsePawnControlRotation = true;
    CameraBoom->bDoCollisionTest = true;
    CameraBoom->bEnableCameraLag = true;
    CameraBoom->CameraLagSpeed = 9.0f;

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;
    FollowCamera->SetActive(true);

    // Over-right-shoulder cinematic performance camera
    ShoulderCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ShoulderCamera"));
    ShoulderCamera->SetupAttachment(GetCapsuleComponent());
    ShoulderCamera->SetRelativeLocation(FVector(-98.0f, 54.0f, 50.0f));
    ShoulderCamera->SetRelativeRotation(FRotator(-12.0f, -18.0f, 0.0f));
    ShoulderCamera->bUsePawnControlRotation = false;
    ShoulderCamera->SetActive(false);

    // Warm 3/4 front artist portrait camera facing the guitarist & lake horizon
    FrontPortraitCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FrontPortraitCamera"));
    FrontPortraitCamera->SetupAttachment(GetCapsuleComponent());
    FrontPortraitCamera->SetRelativeLocation(FVector(168.0f, 76.0f, 28.0f));
    FrontPortraitCamera->SetRelativeRotation(FRotator(-6.0f, -155.0f, 0.0f));
    FrontPortraitCamera->bUsePawnControlRotation = false;
    FrontPortraitCamera->SetActive(false);

    // First-person camera set forward of face mesh so hands & guitar remain clearly visible when looking down
    FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
    FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
    FirstPersonCamera->SetRelativeLocation(FVector(22.0f, 0.0f, 68.0f));
    FirstPersonCamera->bUsePawnControlRotation = true;
    FirstPersonCamera->SetActive(false);

    EnsureDefaultEnhancedInputSetup();
}

void AAcousticChillCharacter::BeginPlay()
{
    Super::BeginPlay();

    // Link custom IK AnimInstance with hidden ABP_Unarmed driver mesh
    if (USkeletalMeshComponent* VisibleMesh = GetMesh())
    {
        if (UGuitarCharacterAnimInstance* CharAnim = Cast<UGuitarCharacterAnimInstance>(VisibleMesh->GetAnimInstance()))
        {
            CharAnim->SetLocomotionDriverMesh(LocomotionDriverMesh);
        }
    }

    SpawnOrAdoptEquippedGuitar();
    bHoldingGuitar = true;
    GuitarHoldBlendAlpha = 1.0f;
    UpdateEquippedGuitarTransform(SitBlendAlpha, GuitarHoldBlendAlpha, 0.0f);

    if (UWorld* World = GetWorld())
    {
        if (UGuitarScoreSubsystem* ScoreSub = World->GetSubsystem<UGuitarScoreSubsystem>())
        {
            ScoreSub->OnGuitarEventTriggered.AddDynamic(this, &AAcousticChillCharacter::HandleGuitarEventTriggered);
        }
    }

    if (UGameInstance* GI = GetGameInstance())
    {
        if (UGuitarGameSettingsSubsystem* SettingsSub = GI->GetSubsystem<UGuitarGameSettingsSubsystem>())
        {
            ApplyControlSettings(SettingsSub->GetSettings());
            SettingsSub->OnSettingsChanged.AddDynamic(this, &AAcousticChillCharacter::HandleSettingsChanged);

            if (UWorld* World = GetWorld())
            {
                if (UGuitarScoreSubsystem* ScoreSub = World->GetSubsystem<UGuitarScoreSubsystem>())
                {
                    ScoreSub->SetChordBankIndex(SettingsSub->GetSettings().ChordBankIndex);
                }
            }
        }
    }

    if (APlayerController* PC = Cast<APlayerController>(Controller))
    {
        if (ULocalPlayer* LP = PC->GetLocalPlayer())
        {
            if (UEnhancedInputLocalPlayerSubsystem* InputSys = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
            {
                if (DefaultMappingContext)
                {
                    InputSys->AddMappingContext(DefaultMappingContext, 0);
                }
            }
        }

        UClass* HUDClassToSpawn = PerformanceHUDClass ? PerformanceHUDClass.Get() : UGuitarPerformanceHUDWidget::StaticClass();
        HUDWidget = CreateWidget<UGuitarPerformanceHUDWidget>(PC, HUDClassToSpawn);
        if (HUDWidget)
        {
            HUDWidget->AddToViewport(10);
            HUDWidget->OnRequestPauseMenu.AddDynamic(this, &AAcousticChillCharacter::HandleOpenPauseFromHUD);
            HUDWidget->SetGuitarState(IsHoldingGuitar(), IsSeated(), EquippedGuitar);
            HUDWidget->SetCameraLabel(GetCameraViewDisplayName());
        }

        UClass* MenuClassToSpawn = MenuWidgetClass ? MenuWidgetClass.Get() : URealGuitarMenuWidget::StaticClass();
        MenuWidget = CreateWidget<URealGuitarMenuWidget>(PC, MenuClassToSpawn);
        if (MenuWidget)
        {
            MenuWidget->AddToViewport(50);
            MenuWidget->OnMenuClosed.AddDynamic(this, &AAcousticChillCharacter::HandleMenuClosed);
            if (bShowTitleScreenOnBeginPlay)
            {
                // Frame character gracefully in front portrait while Title Screen sits open
                SetCameraViewMode(EGuitarCharacterCameraView::PerformanceFront);
                MenuWidget->OpenMenu(ERealGuitarMenuMode::TitleScreen, 0);
                if (HUDWidget)
                {
                    HUDWidget->SetVisibility(ESlateVisibility::Collapsed);
                }
            }
            else
            {
                MenuWidget->SetVisibility(ESlateVisibility::Collapsed);
            }
        }

        RefreshInputMode();
    }
}

void AAcousticChillCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld())
    {
        if (UGuitarScoreSubsystem* ScoreSub = World->GetSubsystem<UGuitarScoreSubsystem>())
        {
            ScoreSub->OnGuitarEventTriggered.RemoveDynamic(this, &AAcousticChillCharacter::HandleGuitarEventTriggered);
        }
    }

    if (UGameInstance* GI = GetGameInstance())
    {
        if (UGuitarGameSettingsSubsystem* SettingsSub = GI->GetSubsystem<UGuitarGameSettingsSubsystem>())
        {
            SettingsSub->OnSettingsChanged.RemoveDynamic(this, &AAcousticChillCharacter::HandleSettingsChanged);
        }
    }

    if (EquippedGuitar && EquippedGuitar->GetAttachParentActor() == this)
    {
        EquippedGuitar->Destroy();
        EquippedGuitar = nullptr;
    }

    Super::EndPlay(EndPlayReason);
}

void AAcousticChillCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    BreathTime += DeltaTime;

    const float TargetSit = IsSeated() ? 1.0f : 0.0f;
    const float TargetHold = IsHoldingGuitar() ? 1.0f : 0.0f;
    SitBlendAlpha = FMath::FInterpTo(SitBlendAlpha, TargetSit, DeltaTime, 7.5f);
    GuitarHoldBlendAlpha = FMath::FInterpTo(GuitarHoldBlendAlpha, TargetHold, DeltaTime, 6.5f);

    TargetStrumSwipe = FMath::FInterpTo(TargetStrumSwipe, 0.0f, DeltaTime, 10.0f);
    StrumSwipeOffset = FMath::FInterpTo(StrumSwipeOffset, TargetStrumSwipe, DeltaTime, 22.0f);

    if (IsSeated() && ActiveSeat && ActiveSeat->SitAnchor)
    {
        const FTransform TargetTM = ActiveSeat->SitAnchor->GetComponentTransform();
        const FVector NewLoc = FMath::VInterpTo(GetActorLocation(), TargetTM.GetLocation(), DeltaTime, 10.0f);
        const FRotator NewRot = FMath::RInterpTo(GetActorRotation(), TargetTM.GetRotation().Rotator(), DeltaTime, 10.0f);
        SetActorLocationAndRotation(NewLoc, NewRot);
    }

    // Orient Shoulder / Portrait cameras cleanly with seated torso drop
    const float ZDrop = -38.0f * SitBlendAlpha;
    if (ShoulderCamera)
    {
        ShoulderCamera->SetRelativeLocation(FVector(-98.0f, 54.0f, 50.0f + ZDrop));
    }
    if (FrontPortraitCamera)
    {
        FrontPortraitCamera->SetRelativeLocation(FVector(168.0f, 76.0f, 28.0f + ZDrop * 0.75f));
    }

    UpdateEquippedGuitarTransform(SitBlendAlpha, GuitarHoldBlendAlpha, BreathTime);

    if (USkeletalMeshComponent* VisibleMesh = GetMesh())
    {
        if (UGuitarCharacterAnimInstance* CharAnim = Cast<UGuitarCharacterAnimInstance>(VisibleMesh->GetAnimInstance()))
        {
            CharAnim->SetSitAlpha(SitBlendAlpha);
            CharAnim->SetGuitarHoldAlpha(GuitarHoldBlendAlpha);
            if (EquippedGuitar && EquippedGuitar->Fretboard)
            {
                CharAnim->SetGuitarIKTargets(
                    EquippedGuitar->Fretboard->GetSmoothedHandIKTargets(),
                    true,
                    StrumSwipeOffset);
            }
            else
            {
                CharAnim->SetGuitarIKTargets(FGuitarHandIKTargets(), false, 0.0f);
            }
        }
    }

    if (!HUDWidget)
    {
        return;
    }

    HUDWidget->SetGuitarState(IsHoldingGuitar(), IsSeated(), EquippedGuitar);
    HUDWidget->SetCameraLabel(GetCameraViewDisplayName());

    if (IsSeated())
    {
        HUDWidget->SetInteractionPrompt(false, FText::GetEmpty());
        return;
    }

    if (AInteractableSeatActor* Seat = FindNearestSeat())
    {
        HUDWidget->SetInteractionPrompt(true, Seat->SitPromptText);
    }
    else
    {
        HUDWidget->SetInteractionPrompt(false, FText::GetEmpty());
    }
}

void AAcousticChillCharacter::SpawnOrAdoptEquippedGuitar()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    TArray<AGuitarPerformerActor*> FoundPerformers;
    for (TActorIterator<AGuitarPerformerActor> It(World); It; ++It)
    {
        if (IsValid(*It))
        {
            FoundPerformers.Add(*It);
        }
    }

    if (!FoundPerformers.IsEmpty())
    {
        EquippedGuitar = FoundPerformers[0];
        for (int32 i = 1; i < FoundPerformers.Num(); ++i)
        {
            FoundPerformers[i]->Destroy();
        }
    }
    else
    {
        FActorSpawnParameters Params;
        Params.Owner = this;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        EquippedGuitar = World->SpawnActor<AGuitarPerformerActor>(
            AGuitarPerformerActor::StaticClass(),
            GetActorTransform(),
            Params);
    }

    if (EquippedGuitar)
    {
        EquippedGuitar->bShowVirtualHands = false;
        EquippedGuitar->SetActorEnableCollision(false);
        EquippedGuitar->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
    }
}

FTransform AAcousticChillCharacter::BuildCradleTransform(float SitAlpha) const
{
    // AcousticGuitar_Full local axes: +X = Nut->Body, +Y = String 1->6 (Low E at +Y), +Z = Soundboard front normal.
    const FVector AxisX = FVector(-0.18f, 0.94f, -0.28f).GetSafeNormal();
    const FVector RawZ = FVector(0.94f, 0.16f, 0.30f).GetSafeNormal();
    const FVector AxisY = FVector::CrossProduct(RawZ, AxisX).GetSafeNormal();
    const FVector AxisZ = FVector::CrossProduct(AxisX, AxisY).GetSafeNormal();

    const FMatrix RotMat(AxisX, AxisY, AxisZ, FVector::ZeroVector);
    const FVector NutOrigin = FVector(34.0f, -31.0f, 27.5f - 36.0f * SitAlpha);
    return FTransform(RotMat.ToQuat(), NutOrigin, FVector::OneVector);
}

FTransform AAcousticChillCharacter::BuildBackSlingTransform() const
{
    // Diagonal carry behind shoulder blades when exploring freely
    const FVector AxisX = FVector(0.04f, 0.56f, -0.83f).GetSafeNormal();
    const FVector RawZ = FVector(-1.0f, 0.0f, 0.0f);
    const FVector AxisY = FVector::CrossProduct(RawZ, AxisX).GetSafeNormal();
    const FVector AxisZ = FVector::CrossProduct(AxisX, AxisY).GetSafeNormal();

    const FMatrix RotMat(AxisX, AxisY, AxisZ, FVector::ZeroVector);
    return FTransform(RotMat.ToQuat(), FVector(-20.0f, -21.0f, 64.0f), FVector::OneVector);
}

void AAcousticChillCharacter::UpdateEquippedGuitarTransform(float SitAlpha, float HoldAlpha, float BreathPhase) const
{
    if (!EquippedGuitar)
    {
        return;
    }

    const FTransform BackTM = BuildBackSlingTransform();
    const FTransform CradleTM = BuildCradleTransform(SitAlpha);

    FTransform Blended;
    Blended.Blend(BackTM, CradleTM, FMath::Clamp(HoldAlpha, 0.0f, 1.0f));
    const FVector BobOffset(0.0f, 0.0f, FMath::Sin(BreathPhase * 1.45f) * 0.65f * HoldAlpha);
    EquippedGuitar->SetActorRelativeLocation(Blended.GetLocation() + BobOffset);
    EquippedGuitar->SetActorRelativeRotation(Blended.GetRotation());
}

void AAcousticChillCharacter::EnsureDefaultEnhancedInputSetup()
{
    auto MakeAction = [this](TObjectPtr<UInputAction>& Target, const TCHAR* Name, EInputActionValueType Type)
    {
        if (!Target)
        {
            Target = NewObject<UInputAction>(this, Name);
            Target->ValueType = Type;
        }
    };

    MakeAction(MoveAction, TEXT("Runtime_IA_Move"), EInputActionValueType::Axis2D);
    MakeAction(LookAction, TEXT("Runtime_IA_Look"), EInputActionValueType::Axis2D);
    MakeAction(InteractAction, TEXT("Runtime_IA_Interact"), EInputActionValueType::Boolean);
    MakeAction(TogglePlayAction, TEXT("Runtime_IA_TogglePlay"), EInputActionValueType::Boolean);
    MakeAction(CycleCameraAction, TEXT("Runtime_IA_CycleCamera"), EInputActionValueType::Boolean);
    MakeAction(NextSongAction, TEXT("Runtime_IA_NextSong"), EInputActionValueType::Boolean);
    MakeAction(PauseMenuAction, TEXT("Runtime_IA_PauseMenu"), EInputActionValueType::Boolean);
    MakeAction(ToggleHudAction, TEXT("Runtime_IA_ToggleHud"), EInputActionValueType::Boolean);
    MakeAction(CycleMoodAction, TEXT("Runtime_IA_CycleMood"), EInputActionValueType::Boolean);

    if (!DefaultMappingContext)
    {
        UInputMappingContext* RuntimeIMC = NewObject<UInputMappingContext>(this, TEXT("Runtime_IMC_Default"));
        auto MapMoveKey = [RuntimeIMC, this](const FKey& Key, bool bSwizzleYX, bool bNegate)
        {
            FEnhancedActionKeyMapping& Mapping = RuntimeIMC->MapKey(MoveAction, Key);
            if (bSwizzleYX)
            {
                UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(RuntimeIMC);
                Swizzle->Order = EInputAxisSwizzle::YXZ;
                Mapping.Modifiers.Add(Swizzle);
            }
            if (bNegate)
            {
                UInputModifierNegate* Negate = NewObject<UInputModifierNegate>(RuntimeIMC);
                Mapping.Modifiers.Add(Negate);
            }
        };

        MapMoveKey(EKeys::W, true, false);
        MapMoveKey(EKeys::S, true, true);
        MapMoveKey(EKeys::D, false, false);
        MapMoveKey(EKeys::A, false, true);

        {
            FEnhancedActionKeyMapping& LookMapping = RuntimeIMC->MapKey(LookAction, EKeys::Mouse2D);
            UInputModifierNegate* NegateY = NewObject<UInputModifierNegate>(RuntimeIMC);
            NegateY->bX = false;
            NegateY->bY = true;
            NegateY->bZ = false;
            LookMapping.Modifiers.Add(NegateY);
        }

        RuntimeIMC->MapKey(InteractAction, EKeys::E);
        RuntimeIMC->MapKey(TogglePlayAction, EKeys::P);
        RuntimeIMC->MapKey(CycleCameraAction, EKeys::C);
        RuntimeIMC->MapKey(NextSongAction, EKeys::N);
        RuntimeIMC->MapKey(PauseMenuAction, EKeys::Escape);
        RuntimeIMC->MapKey(ToggleHudAction, EKeys::H);
        RuntimeIMC->MapKey(CycleMoodAction, EKeys::T);
        DefaultMappingContext = RuntimeIMC;
    }
}

void AAcousticChillCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    EnsureDefaultEnhancedInputSetup();

    if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        if (MoveAction)
            EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AAcousticChillCharacter::HandleMove);
        if (LookAction)
            EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &AAcousticChillCharacter::HandleLook);
        if (InteractAction)
            EnhancedInput->BindAction(InteractAction, ETriggerEvent::Started, this, &AAcousticChillCharacter::HandleInteract);
        if (TogglePlayAction)
            EnhancedInput->BindAction(TogglePlayAction, ETriggerEvent::Started, this, &AAcousticChillCharacter::HandleTogglePlay);
        if (CycleCameraAction)
            EnhancedInput->BindAction(CycleCameraAction, ETriggerEvent::Started, this, &AAcousticChillCharacter::HandleCycleCamera);
        if (NextSongAction)
            EnhancedInput->BindAction(NextSongAction, ETriggerEvent::Started, this, &AAcousticChillCharacter::HandleNextSong);
        if (PauseMenuAction)
            EnhancedInput->BindAction(PauseMenuAction, ETriggerEvent::Started, this, &AAcousticChillCharacter::HandlePauseMenu);
        if (ToggleHudAction)
            EnhancedInput->BindAction(ToggleHudAction, ETriggerEvent::Started, this, &AAcousticChillCharacter::HandleToggleHud);
        if (CycleMoodAction)
            EnhancedInput->BindAction(CycleMoodAction, ETriggerEvent::Started, this, &AAcousticChillCharacter::HandleCycleMood);
    }

    // Core mobility, mode toggles, and direct manual guitar playing bindings
    PlayerInputComponent->BindKey(EKeys::G, IE_Pressed, this, &AAcousticChillCharacter::ToggleHoldingGuitar);
    PlayerInputComponent->BindKey(EKeys::M, IE_Pressed, this, &AAcousticChillCharacter::HandleToggleControlMode);
    PlayerInputComponent->BindKey(EKeys::B, IE_Pressed, this, &AAcousticChillCharacter::HandlePrevSong);
    PlayerInputComponent->BindKey(EKeys::Q, IE_Pressed, this, &AAcousticChillCharacter::HandleToggleCursorMode);
    PlayerInputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &AAcousticChillCharacter::HandlePauseMenu);
    PlayerInputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &AAcousticChillCharacter::HandleSpaceAction);
    PlayerInputComponent->BindKey(EKeys::LeftShift, IE_Pressed, this, &AAcousticChillCharacter::HandleSprintStart);
    PlayerInputComponent->BindKey(EKeys::LeftShift, IE_Released, this, &AAcousticChillCharacter::HandleSprintStop);

    // Chords 1..8 (both number row & numpad)
    static const FKey NumRowKeys[8] = {
        EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four,
        EKeys::Five, EKeys::Six, EKeys::Seven, EKeys::Eight
    };
    static const FKey NumPadKeys[8] = {
        EKeys::NumPadOne, EKeys::NumPadTwo, EKeys::NumPadThree, EKeys::NumPadFour,
        EKeys::NumPadFive, EKeys::NumPadSix, EKeys::NumPadSeven, EKeys::NumPadEight
    };
    for (int32 Slot = 0; Slot < 8; ++Slot)
    {
        FInputKeyBinding BindRow(NumRowKeys[Slot], IE_Pressed);
        BindRow.KeyDelegate.GetDelegateForManualSet().BindLambda([this, Slot]() { HandleHoldChordSlot(Slot); });
        PlayerInputComponent->KeyBindings.Add(MoveTemp(BindRow));

        FInputKeyBinding BindPad(NumPadKeys[Slot], IE_Pressed);
        BindPad.KeyDelegate.GetDelegateForManualSet().BindLambda([this, Slot]() { HandleHoldChordSlot(Slot); });
        PlayerInputComponent->KeyBindings.Add(MoveTemp(BindPad));
    }

    // Chord banks [ / ]
    {
        FInputKeyBinding PrevBank(EKeys::LeftBracket, IE_Pressed);
        PrevBank.KeyDelegate.GetDelegateForManualSet().BindLambda([this]() { HandleCycleChordBank(-1); });
        PlayerInputComponent->KeyBindings.Add(MoveTemp(PrevBank));

        FInputKeyBinding NextBank(EKeys::RightBracket, IE_Pressed);
        NextBank.KeyDelegate.GetDelegateForManualSet().BindLambda([this]() { HandleCycleChordBank(1); });
        PlayerInputComponent->KeyBindings.Add(MoveTemp(NextBank));
    }

    // Individual string plucks: U(6=E2), I(5=A2), O(4=D3), J(3=G3), K(2=B3), L(1=E4)
    static const TPair<FKey, int32> PluckKeyPairs[6] = {
        { EKeys::U, 6 }, { EKeys::I, 5 }, { EKeys::O, 4 },
        { EKeys::J, 3 }, { EKeys::K, 2 }, { EKeys::L, 1 }
    };
    for (const TPair<FKey, int32>& Pair : PluckKeyPairs)
    {
        FInputKeyBinding BindPluck(Pair.Key, IE_Pressed);
        const int32 StrNum = Pair.Value;
        BindPluck.KeyDelegate.GetDelegateForManualSet().BindLambda([this, StrNum]() { HandlePluckStringKey(StrNum); });
        PlayerInputComponent->KeyBindings.Add(MoveTemp(BindPluck));
    }

    // Strums & Percussive slap
    PlayerInputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AAcousticChillCharacter::HandleLiveDownStrum);
    PlayerInputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &AAcousticChillCharacter::HandleLiveUpStrum);
    PlayerInputComponent->BindKey(EKeys::X, IE_Pressed, this, &AAcousticChillCharacter::HandleLivePercussive);
    PlayerInputComponent->BindKey(EKeys::MiddleMouseButton, IE_Pressed, this, &AAcousticChillCharacter::HandleLivePercussive);
}

void AAcousticChillCharacter::HandleMove(const FInputActionValue& Value)
{
    if (IsSeated() || (MenuWidget && MenuWidget->IsMenuOpen()))
    {
        return;
    }

    const FVector2D MoveVec = Value.Get<FVector2D>();
    if (Controller && !MoveVec.IsNearlyZero())
    {
        const FRotator YawRot(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
        const FVector Fwd = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
        const FVector Right = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);
        AddMovementInput(Fwd, MoveVec.Y);
        AddMovementInput(Right, MoveVec.X);
    }
}

void AAcousticChillCharacter::HandleLook(const FInputActionValue& Value)
{
    if ((MenuWidget && MenuWidget->IsMenuOpen()) || bFreeCursorMode)
    {
        return;
    }

    const FVector2D LookVec = Value.Get<FVector2D>();
    AddControllerYawInput(LookVec.X * LookSensitivity);
    AddControllerPitchInput((bInvertLookY ? -LookVec.Y : LookVec.Y) * LookSensitivity);
}

void AAcousticChillCharacter::HandleInteract()
{
    if (MenuWidget && MenuWidget->IsMenuOpen())
    {
        return;
    }
    TryToggleSeat();
}

void AAcousticChillCharacter::HandleTogglePlay()
{
    if (MenuWidget && MenuWidget->IsMenuOpen())
    {
        return;
    }
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
                SetHoldingGuitar(true);
                ScoreSub->Play();
            }
        }
    }
}

void AAcousticChillCharacter::HandleToggleControlMode()
{
    if (MenuWidget && MenuWidget->IsMenuOpen())
    {
        return;
    }
    if (UWorld* World = GetWorld())
    {
        if (UGuitarScoreSubsystem* ScoreSub = World->GetSubsystem<UGuitarScoreSubsystem>())
        {
            SetHoldingGuitar(true);
            ScoreSub->ToggleControlMode();
        }
    }
}

void AAcousticChillCharacter::HandleSpaceAction()
{
    if (MenuWidget && MenuWidget->IsMenuOpen())
    {
        return;
    }
    if (IsHoldingGuitar())
    {
        HandleLiveDownStrum();
    }
    else
    {
        Jump();
    }
}

void AAcousticChillCharacter::HandleSprintStart()
{
    GetCharacterMovement()->MaxWalkSpeed = 480.0f;
}

void AAcousticChillCharacter::HandleSprintStop()
{
    GetCharacterMovement()->MaxWalkSpeed = 240.0f;
}

void AAcousticChillCharacter::SetHoldingGuitar(bool bInHolding)
{
    bHoldingGuitar = bInHolding;
}

void AAcousticChillCharacter::HandleHoldChordSlot(int32 SlotIndex)
{
    if (MenuWidget && MenuWidget->IsMenuOpen())
    {
        return;
    }
    if (!IsHoldingGuitar())
    {
        SetHoldingGuitar(true);
    }
    if (UWorld* World = GetWorld())
    {
        if (UGuitarScoreSubsystem* ScoreSub = World->GetSubsystem<UGuitarScoreSubsystem>())
        {
            ScoreSub->SelectLiveChordSlot(SlotIndex);
        }
    }
}

void AAcousticChillCharacter::HandlePluckStringKey(int32 String1To6)
{
    if (MenuWidget && MenuWidget->IsMenuOpen())
    {
        return;
    }
    if (!IsHoldingGuitar())
    {
        SetHoldingGuitar(true);
    }
    if (UWorld* World = GetWorld())
    {
        if (UGuitarScoreSubsystem* ScoreSub = World->GetSubsystem<UGuitarScoreSubsystem>())
        {
            ScoreSub->PluckLiveString(String1To6, 0.88f);
        }
    }
}

void AAcousticChillCharacter::HandleLiveDownStrum()
{
    if ((MenuWidget && MenuWidget->IsMenuOpen()) || bFreeCursorMode)
    {
        return;
    }
    if (!IsHoldingGuitar())
    {
        SetHoldingGuitar(true);
    }
    if (UWorld* World = GetWorld())
    {
        if (UGuitarScoreSubsystem* ScoreSub = World->GetSubsystem<UGuitarScoreSubsystem>())
        {
            ScoreSub->TriggerLiveStrum(true, 0.90f);
        }
    }
}

void AAcousticChillCharacter::HandleLiveUpStrum()
{
    if ((MenuWidget && MenuWidget->IsMenuOpen()) || bFreeCursorMode)
    {
        return;
    }
    if (!IsHoldingGuitar())
    {
        SetHoldingGuitar(true);
    }
    if (UWorld* World = GetWorld())
    {
        if (UGuitarScoreSubsystem* ScoreSub = World->GetSubsystem<UGuitarScoreSubsystem>())
        {
            ScoreSub->TriggerLiveStrum(false, 0.72f);
        }
    }
}

void AAcousticChillCharacter::HandleLivePercussive()
{
    if ((MenuWidget && MenuWidget->IsMenuOpen()) || bFreeCursorMode)
    {
        return;
    }
    if (!IsHoldingGuitar())
    {
        SetHoldingGuitar(true);
    }
    if (UWorld* World = GetWorld())
    {
        if (UGuitarScoreSubsystem* ScoreSub = World->GetSubsystem<UGuitarScoreSubsystem>())
        {
            ScoreSub->TriggerLivePercussive(0.75f);
        }
    }
}

void AAcousticChillCharacter::HandleCycleChordBank(int32 Delta)
{
    if (MenuWidget && MenuWidget->IsMenuOpen())
    {
        return;
    }
    if (UWorld* World = GetWorld())
    {
        if (UGuitarScoreSubsystem* ScoreSub = World->GetSubsystem<UGuitarScoreSubsystem>())
        {
            const int32 NumBanks = FMath::Max(1, UGuitarSongLibrarySubsystem::GetNumChordBanks());
            const int32 NextBank = ((ScoreSub->GetChordBankIndex() + Delta) % NumBanks + NumBanks) % NumBanks;
            ScoreSub->SetChordBankIndex(NextBank);
        }
    }
}

void AAcousticChillCharacter::HandleGuitarEventTriggered(const FGuitarEvent& Event)
{
    switch (Event.RightAction)
    {
    case EGuitarRightAction::StrumDown:
    case EGuitarRightAction::PickDown:
        TargetStrumSwipe = -1.0f;
        break;
    case EGuitarRightAction::StrumUp:
    case EGuitarRightAction::PickUp:
        TargetStrumSwipe = 0.95f;
        break;
    case EGuitarRightAction::PercussiveSlap:
        TargetStrumSwipe = -0.65f;
        break;
    default:
        TargetStrumSwipe = (Event.StringIndex >= 4) ? -0.35f : 0.35f;
        break;
    }
}

void AAcousticChillCharacter::SetCameraViewMode(EGuitarCharacterCameraView NewView)
{
    CurrentCameraView = NewView;

    FollowCamera->SetActive(NewView == EGuitarCharacterCameraView::ExploreFollow);
    ShoulderCamera->SetActive(NewView == EGuitarCharacterCameraView::PerformanceShoulder);
    FrontPortraitCamera->SetActive(NewView == EGuitarCharacterCameraView::PerformanceFront);
    FirstPersonCamera->SetActive(NewView == EGuitarCharacterCameraView::FirstPerson);

    if (APlayerController* PC = Cast<APlayerController>(Controller))
    {
        if ((NewView == EGuitarCharacterCameraView::FretboardCloseUp || NewView == EGuitarCharacterCameraView::PickingHandCloseUp) && EquippedGuitar)
        {
            EquippedGuitar->SetActiveCameraAngle(
                NewView == EGuitarCharacterCameraView::FretboardCloseUp
                    ? EGuitarCameraAngle::FretboardCloseUp
                    : EGuitarCameraAngle::PickingHandCloseUp);
            PC->SetViewTargetWithBlend(EquippedGuitar, 0.45f, VTBlend_Cubic);
        }
        else
        {
            PC->SetViewTargetWithBlend(this, 0.45f, VTBlend_Cubic);
        }
    }

    if (HUDWidget)
    {
        HUDWidget->SetCameraLabel(GetCameraViewDisplayName());
    }
}

FText AAcousticChillCharacter::GetCameraViewDisplayName() const
{
    switch (CurrentCameraView)
    {
    case EGuitarCharacterCameraView::ExploreFollow:
        return NSLOCTEXT("GuitarCam", "Follow", "3인칭 자유 시점");
    case EGuitarCharacterCameraView::PerformanceShoulder:
        return NSLOCTEXT("GuitarCam", "Shoulder", "연주 숄더 뷰");
    case EGuitarCharacterCameraView::PerformanceFront:
        return NSLOCTEXT("GuitarCam", "Front", "정면 아티스트 뷰");
    case EGuitarCharacterCameraView::FretboardCloseUp:
        return NSLOCTEXT("GuitarCam", "Fretboard", "왼손 프렛 클로즈업");
    case EGuitarCharacterCameraView::PickingHandCloseUp:
        return NSLOCTEXT("GuitarCam", "Picking", "오른손 피킹 클로즈업");
    case EGuitarCharacterCameraView::FirstPerson:
        return NSLOCTEXT("GuitarCam", "FPS", "1인칭 시점");
    default:
        return NSLOCTEXT("GuitarCam", "Default", "3인칭 시점");
    }
}

void AAcousticChillCharacter::HandleCycleCamera()
{
    if (MenuWidget && MenuWidget->IsMenuOpen())
    {
        return;
    }
    const uint8 NextIdx = (static_cast<uint8>(CurrentCameraView) + 1) % 6;
    SetCameraViewMode(static_cast<EGuitarCharacterCameraView>(NextIdx));
}

void AAcousticChillCharacter::HandleNextSong()
{
    if (MenuWidget && MenuWidget->IsMenuOpen())
    {
        return;
    }
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UGuitarSongLibrarySubsystem* Lib = GI->GetSubsystem<UGuitarSongLibrarySubsystem>())
        {
            SetHoldingGuitar(true);
            Lib->PlayNextSong();
        }
    }
}

void AAcousticChillCharacter::HandlePrevSong()
{
    if (MenuWidget && MenuWidget->IsMenuOpen())
    {
        return;
    }
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UGuitarSongLibrarySubsystem* Lib = GI->GetSubsystem<UGuitarSongLibrarySubsystem>())
        {
            SetHoldingGuitar(true);
            Lib->PlayPreviousSong();
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
    }
    else
    {
        MenuWidget->OpenMenu(ERealGuitarMenuMode::PauseMenu, 0);
        if (HUDWidget)
        {
            HUDWidget->SetVisibility(ESlateVisibility::Collapsed);
        }
        RefreshInputMode();
    }
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
    }
    if (CurrentCameraView == EGuitarCharacterCameraView::PerformanceFront && !IsSeated())
    {
        SetCameraViewMode(EGuitarCharacterCameraView::ExploreFollow);
    }
    RefreshInputMode();
}

void AAcousticChillCharacter::HandleToggleCursorMode()
{
    if (MenuWidget && MenuWidget->IsMenuOpen())
    {
        return;
    }
    bFreeCursorMode = !bFreeCursorMode;
    RefreshInputMode();
}

void AAcousticChillCharacter::RefreshInputMode()
{
    APlayerController* PC = Cast<APlayerController>(Controller);
    if (!PC)
    {
        return;
    }

    if (MenuWidget && MenuWidget->IsMenuOpen())
    {
        FInputModeGameAndUI InputMode;
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        InputMode.SetWidgetToFocus(MenuWidget->TakeWidget());
        PC->SetInputMode(InputMode);
        PC->bShowMouseCursor = true;
    }
    else if (bFreeCursorMode)
    {
        FInputModeGameAndUI InputMode;
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        PC->SetInputMode(InputMode);
        PC->bShowMouseCursor = true;
    }
    else
    {
        FInputModeGameOnly InputMode;
        PC->SetInputMode(InputMode);
        PC->bShowMouseCursor = false;
    }
}

void AAcousticChillCharacter::HandleToggleHud()
{
    if (HUDWidget)
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
    if (CameraBoom)
    {
        CameraBoom->TargetArmLength = Settings.CameraDistance;
    }
    if (FollowCamera)
    {
        FollowCamera->SetFieldOfView(Settings.FieldOfView);
    }
    if (ShoulderCamera)
    {
        ShoulderCamera->SetFieldOfView(Settings.FieldOfView);
    }
    if (FrontPortraitCamera)
    {
        FrontPortraitCamera->SetFieldOfView(Settings.FieldOfView);
    }
    if (FirstPersonCamera)
    {
        FirstPersonCamera->SetFieldOfView(Settings.FieldOfView);
    }
    ApplyCharacterAppearance(Settings.CharacterModelIndex);
}

void AAcousticChillCharacter::ApplyCharacterAppearance(int32 CharacterModelIndex)
{
    const int32 SafeIndex = FMath::Clamp(CharacterModelIndex, 0, 1);
    if (AppliedCharacterIndex == SafeIndex)
    {
        return;
    }

    USkeletalMesh* DesiredMesh = (SafeIndex == 1 && CachedMannyMesh)
        ? CachedMannyMesh.Get()
        : (CachedQuinnMesh ? CachedQuinnMesh.Get() : CachedMannyMesh.Get());

    if (!DesiredMesh)
    {
        return;
    }

    AppliedCharacterIndex = SafeIndex;
    if (LocomotionDriverMesh)
    {
        LocomotionDriverMesh->SetSkeletalMeshAsset(DesiredMesh);
        if (CachedUnarmedAnimClass)
        {
            LocomotionDriverMesh->SetAnimInstanceClass(CachedUnarmedAnimClass);
        }
    }
    if (USkeletalMeshComponent* VisibleMesh = GetMesh())
    {
        VisibleMesh->SetSkeletalMeshAsset(DesiredMesh);
        VisibleMesh->SetAnimInstanceClass(UGuitarCharacterAnimInstance::StaticClass());
        if (UGuitarCharacterAnimInstance* CharAnim = Cast<UGuitarCharacterAnimInstance>(VisibleMesh->GetAnimInstance()))
        {
            CharAnim->SetLocomotionDriverMesh(LocomotionDriverMesh);
        }
    }
}

AInteractableSeatActor* AAcousticChillCharacter::FindNearestSeat() const
{
    const UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    AInteractableSeatActor* BestSeat = nullptr;
    float BestDistSq = InteractTraceDistance * InteractTraceDistance;
    const FVector MyLoc = GetActorLocation();

    for (TActorIterator<AInteractableSeatActor> It(World); It; ++It)
    {
        AInteractableSeatActor* Seat = *It;
        if (!IsValid(Seat))
        {
            continue;
        }
        const float DistSq = FVector::DistSquared(MyLoc, Seat->GetActorLocation());
        if (DistSq <= BestDistSq)
        {
            BestDistSq = DistSq;
            BestSeat = Seat;
        }
    }
    return BestSeat;
}

void AAcousticChillCharacter::TryToggleSeat()
{
    APlayerController* PC = Cast<APlayerController>(Controller);
    if (!PC)
    {
        return;
    }

    if (ActiveSeat)
    {
        ActiveSeat = nullptr;
        GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        SetActorLocationAndRotation(SavedStandingTransform.GetLocation(), SavedStandingTransform.GetRotation().Rotator());
        SetCameraViewMode(EGuitarCharacterCameraView::ExploreFollow);
        RefreshInputMode();
        return;
    }

    if (AInteractableSeatActor* Seat = FindNearestSeat())
    {
        SavedStandingTransform = GetActorTransform();
        ActiveSeat = Seat;
        bHoldingGuitar = true;

        GetCharacterMovement()->StopMovementImmediately();
        GetCharacterMovement()->SetMovementMode(MOVE_None);
        GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

        if (Seat->SitAnchor)
        {
            SetActorLocationAndRotation(Seat->SitAnchor->GetComponentLocation(), Seat->SitAnchor->GetComponentRotation());
        }

        if (!Seat->DefaultScoreRelativePath.IsEmpty())
        {
            if (UGuitarScoreSubsystem* ScoreSub = GetWorld()->GetSubsystem<UGuitarScoreSubsystem>())
            {
                const FString FullPath = FPaths::Combine(FPaths::ProjectContentDir(), Seat->DefaultScoreRelativePath);
                if (FPaths::FileExists(FullPath))
                {
                    ScoreSub->LoadPerformanceFromJsonFile(FullPath);
                }
            }
        }

        SetCameraViewMode(EGuitarCharacterCameraView::PerformanceShoulder);
        RefreshInputMode();
    }
}
