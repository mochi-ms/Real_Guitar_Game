#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GuitarGameSettings.h"
#include "RealGuitarMenuWidget.generated.h"

class UBorder;
class UButton;
class UCanvasPanelSlot;
class UCheckBox;
class UScrollBox;
class USlider;
class UTextBlock;
class UWidgetSwitcher;

UENUM(BlueprintType)
enum class ERealGuitarMenuMode : uint8
{
    TitleScreen UMETA(DisplayName = "Title Screen"),
    PauseMenu UMETA(DisplayName = "Pause Menu")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGuitarMenuClosed);

/**
 * Full procedural AAA dark-glass Title & Settings overlay menu.
 * Title mode docks cleanly on the left so Quinn/Manny and the lakeside stage stay framed on the right.
 */
UCLASS()
class REALGUITARGAME_API URealGuitarMenuWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

    UFUNCTION(BlueprintCallable, Category = "Guitar|Menu")
    void OpenMenu(ERealGuitarMenuMode InMode, int32 InitialTabIndex = 0);

    UFUNCTION(BlueprintCallable, Category = "Guitar|Menu")
    void CloseMenu();

    UFUNCTION(BlueprintPure, Category = "Guitar|Menu")
    bool IsMenuOpen() const { return bMenuOpen; }

    UFUNCTION(BlueprintPure, Category = "Guitar|Menu")
    ERealGuitarMenuMode GetCurrentMode() const { return CurrentMode; }

    UFUNCTION(BlueprintCallable, Category = "Guitar|Menu")
    void SelectTab(int32 TabIndex);

    UPROPERTY(BlueprintAssignable, Category = "Guitar|Menu")
    FOnGuitarMenuClosed OnMenuClosed;

private:
    void BuildMenuLayout();
    UScrollBox* CreateTabScrollContainer();
    void RefreshModeChrome();
    void SyncWidgetsFromSettings();
    void SyncSongAndPlayLabels();
    void PushDraftSettings(bool bPersistToDisk);
    UGuitarGameSettingsSubsystem* GetSettingsSubsystem() const;
    int32 FindClosestResolutionIndex(const FIntPoint& Res) const;

    // Chrome & Navigation Handlers
    UFUNCTION() void HandleResumeOrStartClicked();
    UFUNCTION() void HandleBackToTitleClicked();
    UFUNCTION() void HandleQuitClicked();
    UFUNCTION() void HandleTabMainClicked();
    UFUNCTION() void HandleTabGraphicsClicked();
    UFUNCTION() void HandleTabAudioClicked();
    UFUNCTION() void HandleTabControlsClicked();

    // Tab 0: Main, Performer & Songs
    UFUNCTION() void HandleTogglePlayModeClicked();
    UFUNCTION() void HandleChordBankPrevClicked();
    UFUNCTION() void HandleChordBankNextClicked();
    UFUNCTION() void HandleCharacterPrevClicked();
    UFUNCTION() void HandleCharacterNextClicked();
    UFUNCTION() void HandleSongPrevClicked();
    UFUNCTION() void HandleSongNextClicked();
    UFUNCTION() void HandlePlayPauseClicked();
    UFUNCTION() void HandleLoopToggled(bool bChecked);
    UFUNCTION() void HandleJamBalladClicked();
    UFUNCTION() void HandleJamFingerstyleClicked();
    UFUNCTION() void HandleJamPopGrooveClicked();

    // Tab 1: Graphics
    UFUNCTION() void HandleResPrevClicked();
    UFUNCTION() void HandleResNextClicked();
    UFUNCTION() void HandleWinModePrevClicked();
    UFUNCTION() void HandleWinModeNextClicked();
    UFUNCTION() void HandleFpsPrevClicked();
    UFUNCTION() void HandleFpsNextClicked();
    UFUNCTION() void HandleQualityPrevClicked();
    UFUNCTION() void HandleQualityNextClicked();
    UFUNCTION() void HandleVSyncToggled(bool bChecked);
    UFUNCTION() void HandleLumenToggled(bool bChecked);
    UFUNCTION() void HandleApplyGraphicsClicked();
    UFUNCTION() void HandleResetDefaultsClicked();

    // Tab 2: Audio & Mood
    UFUNCTION() void HandleMasterVolChanged(float Value);
    UFUNCTION() void HandleGuitarVolChanged(float Value);
    UFUNCTION() void HandleAmbientVolChanged(float Value);
    UFUNCTION() void HandleReverbChanged(float Value);
    UFUNCTION() void HandleMoodSunsetClicked();
    UFUNCTION() void HandleMoodNightClicked();
    UFUNCTION() void HandleMoodMorningClicked();
    UFUNCTION() void HandleMoodRainClicked();
    UFUNCTION() void HandleShowChordsToggled(bool bChecked);
    UFUNCTION() void HandleAutoHideHudToggled(bool bChecked);

    // Tab 3: Camera & Controls
    UFUNCTION() void HandleCamDistChanged(float Value);
    UFUNCTION() void HandleMouseSensChanged(float Value);
    UFUNCTION() void HandleFovChanged(float Value);
    UFUNCTION() void HandleInvertYToggled(bool bChecked);

    UPROPERTY(Transient) TObjectPtr<UBorder> RootBackdrop;
    UPROPERTY(Transient) TObjectPtr<UBorder> MainPanelBorder;
    UPROPERTY(Transient) TObjectPtr<UCanvasPanelSlot> MainPanelSlot;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> HeroTitleText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> HeroSubtitleText;
    UPROPERTY(Transient) TObjectPtr<UButton> PrimaryActionButton;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> PrimaryActionLabel;
    UPROPERTY(Transient) TObjectPtr<UButton> BackToTitleButton;
    UPROPERTY(Transient) TObjectPtr<UButton> QuitButton;
    UPROPERTY(Transient) TArray<TObjectPtr<UButton>> TabButtons;
    UPROPERTY(Transient) TObjectPtr<UWidgetSwitcher> ContentSwitcher;

    // Tab 0 widgets
    UPROPERTY(Transient) TObjectPtr<UTextBlock> PlayModeValueText;
    UPROPERTY(Transient) TObjectPtr<UButton> TogglePlayModeButton;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ChordBankValueText;
    UPROPERTY(Transient) TObjectPtr<UButton> ChordBankPrevButton;
    UPROPERTY(Transient) TObjectPtr<UButton> ChordBankNextButton;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> CharacterValueText;
    UPROPERTY(Transient) TObjectPtr<UButton> CharacterPrevButton;
    UPROPERTY(Transient) TObjectPtr<UButton> CharacterNextButton;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> SelectedSongText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> PlayPauseLabelText;
    UPROPERTY(Transient) TObjectPtr<UButton> SongPrevButton;
    UPROPERTY(Transient) TObjectPtr<UButton> SongNextButton;
    UPROPERTY(Transient) TObjectPtr<UButton> PlayPauseButton;
    UPROPERTY(Transient) TObjectPtr<UCheckBox> LoopCheckBox;
    UPROPERTY(Transient) TObjectPtr<UButton> JamBalladButton;
    UPROPERTY(Transient) TObjectPtr<UButton> JamFingerstyleButton;
    UPROPERTY(Transient) TObjectPtr<UButton> JamPopButton;

    // Tab 1 widgets
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ResValueText;
    UPROPERTY(Transient) TObjectPtr<UButton> ResPrevButton;
    UPROPERTY(Transient) TObjectPtr<UButton> ResNextButton;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> WinModeValueText;
    UPROPERTY(Transient) TObjectPtr<UButton> WinModePrevButton;
    UPROPERTY(Transient) TObjectPtr<UButton> WinModeNextButton;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> FpsValueText;
    UPROPERTY(Transient) TObjectPtr<UButton> FpsPrevButton;
    UPROPERTY(Transient) TObjectPtr<UButton> FpsNextButton;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> QualityValueText;
    UPROPERTY(Transient) TObjectPtr<UButton> QualityPrevButton;
    UPROPERTY(Transient) TObjectPtr<UButton> QualityNextButton;
    UPROPERTY(Transient) TObjectPtr<UCheckBox> VSyncCheckBox;
    UPROPERTY(Transient) TObjectPtr<UCheckBox> LumenCheckBox;
    UPROPERTY(Transient) TObjectPtr<UButton> ApplyGraphicsButton;
    UPROPERTY(Transient) TObjectPtr<UButton> ResetDefaultsButton;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> GraphicsStatusText;

    // Tab 2 widgets
    UPROPERTY(Transient) TObjectPtr<USlider> MasterVolSlider;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> MasterVolValueText;
    UPROPERTY(Transient) TObjectPtr<USlider> GuitarVolSlider;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> GuitarVolValueText;
    UPROPERTY(Transient) TObjectPtr<USlider> AmbientVolSlider;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> AmbientVolValueText;
    UPROPERTY(Transient) TObjectPtr<USlider> ReverbSlider;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ReverbValueText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> CurrentMoodLabel;
    UPROPERTY(Transient) TObjectPtr<UButton> MoodSunsetButton;
    UPROPERTY(Transient) TObjectPtr<UButton> MoodNightButton;
    UPROPERTY(Transient) TObjectPtr<UButton> MoodMorningButton;
    UPROPERTY(Transient) TObjectPtr<UButton> MoodRainButton;
    UPROPERTY(Transient) TObjectPtr<UCheckBox> ShowChordsCheckBox;
    UPROPERTY(Transient) TObjectPtr<UCheckBox> AutoHideHudCheckBox;

    // Tab 3 widgets
    UPROPERTY(Transient) TObjectPtr<USlider> CamDistSlider;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> CamDistValueText;
    UPROPERTY(Transient) TObjectPtr<USlider> MouseSensSlider;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> MouseSensValueText;
    UPROPERTY(Transient) TObjectPtr<USlider> FovSlider;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> FovValueText;
    UPROPERTY(Transient) TObjectPtr<UCheckBox> InvertYCheckBox;

    ERealGuitarMenuMode CurrentMode = ERealGuitarMenuMode::TitleScreen;
    FGuitarSettingsData DraftSettings;
    TArray<FIntPoint> ResolutionChoices;
    int32 ResolutionIndex = 2;
    int32 FpsChoiceIndex = 1;
    bool bMenuOpen = false;
    bool bSyncingUI = false;
};
