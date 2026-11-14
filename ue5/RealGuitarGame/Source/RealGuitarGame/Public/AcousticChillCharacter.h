#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "AcousticChillCharacter.generated.h"

struct FGuitarSettingsData;
class AInteractableSeatActor;
class UCameraComponent;
class UGuitarPerformanceHUDWidget;
class UInputAction;
class UInputMappingContext;
class URealGuitarMenuWidget;

/**
 * First-person exploration character capable of walking around the photorealistic scene
 * and sitting on any AInteractableSeatActor to watch/listen to auto-played guitar music.
 */
UCLASS()
class REALGUITARGAME_API AAcousticChillCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AAcousticChillCharacter();

    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

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
    float InteractTraceDistance = 260.0f;

    UFUNCTION(BlueprintPure, Category = "Interaction")
    bool IsSeated() const { return ActiveSeat != nullptr; }

    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void TryToggleSeat();

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
    void HandlePauseMenu();
    void HandleToggleHud();
    void HandleCycleMood();

    UFUNCTION()
    void HandleOpenPauseFromHUD();

    UFUNCTION()
    void HandleMenuClosed();

    UFUNCTION()
    void HandleSettingsChanged(const FGuitarSettingsData& NewSettings);

    void ApplyControlSettings(const FGuitarSettingsData& Settings);
    void EnsureDefaultEnhancedInputSetup();

    AInteractableSeatActor* FindLookedAtSeat() const;

    UPROPERTY(Transient)
    TObjectPtr<AInteractableSeatActor> ActiveSeat;

    UPROPERTY(Transient)
    TObjectPtr<UGuitarPerformanceHUDWidget> HUDWidget;

    UPROPERTY(Transient)
    TObjectPtr<URealGuitarMenuWidget> MenuWidget;

    float LookSensitivity = 1.0f;
    bool bInvertLookY = false;
};
