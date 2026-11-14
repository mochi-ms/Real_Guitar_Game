#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GuitarPerformanceTypes.h"
#include "InputActionValue.h"
#include "AcousticChillCharacter.generated.h"

struct FGuitarSettingsData;
class AGuitarPerformerActor;
class AInteractableSeatActor;
class UCameraComponent;
class UGuitarPerformanceHUDWidget;
class UInputAction;
class UInputMappingContext;
class URealGuitarMenuWidget;
class USkeletalMesh;
class USkeletalMeshComponent;
class USpringArmComponent;

UENUM(BlueprintType)
enum class EGuitarCharacterCameraView : uint8
{
    ExploreFollow UMETA(DisplayName = "3인칭 자유 시점"),
    PerformanceShoulder UMETA(DisplayName = "연주 숄더 뷰"),
    PerformanceFront UMETA(DisplayName = "정면 아티스트 뷰"),
    FretboardCloseUp UMETA(DisplayName = "왼손 프렛 클로즈업"),
    PickingHandCloseUp UMETA(DisplayName = "오른손 피킹 클로즈업"),
    FirstPerson UMETA(DisplayName = "1인칭 시점")
};

/**
 * Visible 3rd/1st-person guitarist avatar supporting free lakeside exploration,
 * carrying/playing the acoustic guitar with live Two-Bone IK finger/strum animation,
 * seating on sanctuary benches/stools, and hybrid AutoSong + manual FreePlay input.
 */
UCLASS()
class REALGUITARGAME_API AAcousticChillCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AAcousticChillCharacter();

    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character|Animation")
    TObjectPtr<USkeletalMeshComponent> LocomotionDriverMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    TObjectPtr<USpringArmComponent> CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    TObjectPtr<UCameraComponent> FollowCamera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    TObjectPtr<UCameraComponent> ShoulderCamera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    TObjectPtr<UCameraComponent> FrontPortraitCamera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    TObjectPtr<UCameraComponent> FirstPersonCamera;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputMappingContext> DefaultMappingContext;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> MoveAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> LookAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> InteractAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> TogglePlayAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> CycleCameraAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> NextSongAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> PauseMenuAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> ToggleHudAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> CycleMoodAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
    TSubclassOf<UGuitarPerformanceHUDWidget> PerformanceHUDClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
    TSubclassOf<URealGuitarMenuWidget> MenuWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    bool bShowTitleScreenOnBeginPlay = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    float InteractTraceDistance = 280.0f;

    UFUNCTION(BlueprintPure, Category = "Interaction")
    bool IsSeated() const { return ActiveSeat != nullptr; }

    UFUNCTION(BlueprintPure, Category = "Guitar")
    bool IsHoldingGuitar() const { return bHoldingGuitar || ActiveSeat != nullptr; }

    UFUNCTION(BlueprintCallable, Category = "Guitar")
    void SetHoldingGuitar(bool bInHolding);

    UFUNCTION(BlueprintCallable, Category = "Guitar")
    void ToggleHoldingGuitar() { SetHoldingGuitar(!bHoldingGuitar); }

    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void TryToggleSeat();

    UFUNCTION(BlueprintPure, Category = "Camera")
    EGuitarCharacterCameraView GetCameraViewMode() const { return CurrentCameraView; }

    UFUNCTION(BlueprintCallable, Category = "Camera")
    void SetCameraViewMode(EGuitarCharacterCameraView NewView);

    UFUNCTION(BlueprintPure, Category = "Camera")
    FText GetCameraViewDisplayName() const;

    UFUNCTION(BlueprintPure, Category = "Guitar")
    AGuitarPerformerActor* GetEquippedGuitar() const { return EquippedGuitar; }

    UFUNCTION(BlueprintPure, Category = "UI")
    bool IsFreeCursorMode() const { return bFreeCursorMode; }

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void HandleMove(const FInputActionValue& Value);
    void HandleLook(const FInputActionValue& Value);
    void HandleInteract();
    void HandleTogglePlay();
    void HandleCycleCamera();
    void HandleNextSong();
    void HandlePrevSong();
    void HandlePauseMenu();
    void HandleToggleHud();
    void HandleCycleMood();
    void HandleToggleControlMode();
    void HandleToggleCursorMode();
    void HandleSpaceAction();
    void HandleSprintStart();
    void HandleSprintStop();

    // Live manual guitar controls
    void HandleHoldChordSlot(int32 SlotIndex);
    void HandlePluckStringKey(int32 String1To6);
    void HandleLiveDownStrum();
    void HandleLiveUpStrum();
    void HandleLivePercussive();
    void HandleCycleChordBank(int32 Delta);

    UFUNCTION()
    void HandleGuitarEventTriggered(const FGuitarEvent& Event);

    UFUNCTION()
    void HandleOpenPauseFromHUD();

    UFUNCTION()
    void HandleMenuClosed();

    UFUNCTION()
    void HandleSettingsChanged(const FGuitarSettingsData& NewSettings);

    void ApplyControlSettings(const FGuitarSettingsData& Settings);
    void ApplyCharacterAppearance(int32 CharacterModelIndex);
    void EnsureDefaultEnhancedInputSetup();
    void SpawnOrAdoptEquippedGuitar();
    void UpdateEquippedGuitarTransform(float SitAlpha, float HoldAlpha, float BreathPhase) const;
    void RefreshInputMode();
    FTransform BuildCradleTransform(float SitAlpha) const;
    FTransform BuildBackSlingTransform() const;

    AInteractableSeatActor* FindNearestSeat() const;

    UPROPERTY(Transient)
    TObjectPtr<AInteractableSeatActor> ActiveSeat;

    UPROPERTY(Transient)
    TObjectPtr<AGuitarPerformerActor> EquippedGuitar;

    UPROPERTY(Transient)
    TObjectPtr<UGuitarPerformanceHUDWidget> HUDWidget;

    UPROPERTY(Transient)
    TObjectPtr<URealGuitarMenuWidget> MenuWidget;

    UPROPERTY(Transient)
    TObjectPtr<USkeletalMesh> CachedQuinnMesh;

    UPROPERTY(Transient)
    TObjectPtr<USkeletalMesh> CachedMannyMesh;

    UPROPERTY(Transient)
    TObjectPtr<UClass> CachedUnarmedAnimClass;

    EGuitarCharacterCameraView CurrentCameraView = EGuitarCharacterCameraView::ExploreFollow;
    FTransform SavedStandingTransform = FTransform::Identity;
    float LookSensitivity = 1.0f;
    bool bInvertLookY = false;
    bool bHoldingGuitar = true;
    bool bFreeCursorMode = false;
    float SitBlendAlpha = 0.0f;
    float GuitarHoldBlendAlpha = 1.0f;
    float StrumSwipeOffset = 0.0f;
    float TargetStrumSwipe = 0.0f;
    float BreathTime = 0.0f;
    int32 AppliedCharacterIndex = INDEX_NONE;
};
