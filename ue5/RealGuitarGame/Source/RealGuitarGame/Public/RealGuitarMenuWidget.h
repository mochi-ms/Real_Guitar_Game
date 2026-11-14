#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GuitarGameSettings.h"
#include "RealGuitarMenuWidget.generated.h"

class UButton;
class UCheckBox;
class USlider;
class UTextBlock;
class UWidget;
class UWidgetSwitcher;

UENUM(BlueprintType)
enum class ERealGuitarMenuMode : uint8
{
    TitleScreen,
    PauseMenu
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGuitarMenuClosed);

/**
 * Full-featured dark-glass Title Screen, Pause Menu, Song & Live Jam Selector,
 * Graphics/Display Options, Audio/Mood Mixer, and Controls Settings menu.
 * Automatically constructs a complete UMG hierarchy when no Designer Blueprint override exists.
 */
UCLASS()
class REALGUITARGAME_API URealGuitarMenuWidget : public UUserWidget
{
    GENERATED_BODY()

public:
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

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
    void BuildDefaultMenuLayout();
    UWidget* BuildMainAndSongsTab();
    UWidget* BuildGraphicsTab();
    UWidget* BuildAudioAndMoodTab();
    UWidget* BuildControlsTab();

    void SyncWidgetsFromSettings();
    void RefreshSongDisplay();
    void PushDraftSettings(bool bPersist = true);

    // Navigation & main actions
    UFUNCTION() void HandleResumeOrStartClicked();
    UFUNCTION() void HandleBackToTitleClicked();
    UFUNCTION() void HandleQuitClicked();
    UFUNCTION() void HandleTabMainClicked();
    UFUNCTION() void HandleTabGraphicsClicked();
    UFUNCTION() void HandleTabAudioClicked();
    UFUNCTION() void HandleTabControlsClicked();

    // Song & Jam actions
    UFUNCTION() void HandleSongPrevClicked();
    UFUNCTION() void HandleSongNextClicked();
    UFUNCTION() void HandlePlayPauseClicked();
    UFUNCTION() void HandleLoopToggled(bool bChecked);
    UFUNCTION() void HandleJamBalladClicked();
    UFUNCTION() void HandleJamFingerstyleClicked();
    UFUNCTION() void HandleJamPopGrooveClicked();

    // Graphics steppers & toggles
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

    // Audio & Mood
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

    // Controls
    UFUNCTION() void HandleMouseSensChanged(float Value);
    UFUNCTION() void HandleFovChanged(float Value);
    UFUNCTION() void HandleInvertYToggled(bool bChecked);

    UPROPERTY(Transient) TObjectPtr<UWidgetSwitcher> ContentSwitcher;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> HeroTitleText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> HeroSubtitleText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> PrimaryActionLabel;
    UPROPERTY(Transient) TObjectPtr<UButton> PrimaryActionButton;
    UPROPERTY(Transient) TObjectPtr<UButton> BackToTitleButton;
    UPROPERTY(Transient) TObjectPtr<UButton> QuitButton;
    UPROPERTY(Transient) TObjectPtr<UButton> TabMainButton;
    UPROPERTY(Transient) TObjectPtr<UButton> TabGraphicsButton;
    UPROPERTY(Transient) TObjectPtr<UButton> TabAudioButton;
    UPROPERTY(Transient) TObjectPtr<UButton> TabControlsButton;

    // Songs Tab
    UPROPERTY(Transient) TObjectPtr<UButton> SongPrevButton;
    UPROPERTY(Transient) TObjectPtr<UButton> SongNextButton;
    UPROPERTY(Transient) TObjectPtr<UButton> SongPlayPauseButton;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> SongNameValueText;
    UPROPERTY(Transient) TObjectPtr<UCheckBox> LoopSongCheckBox;
    UPROPERTY(Transient) TObjectPtr<UButton> JamBalladButton;
    UPROPERTY(Transient) TObjectPtr<UButton> JamFingerButton;
    UPROPERTY(Transient) TObjectPtr<UButton> JamPopButton;

    // Graphics Tab
    UPROPERTY(Transient) TObjectPtr<UButton> ResPrevButton;
    UPROPERTY(Transient) TObjectPtr<UButton> ResNextButton;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ResValueText;
    UPROPERTY(Transient) TObjectPtr<UButton> WinModePrevButton;
    UPROPERTY(Transient) TObjectPtr<UButton> WinModeNextButton;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> WinModeValueText;
    UPROPERTY(Transient) TObjectPtr<UButton> FpsPrevButton;
    UPROPERTY(Transient) TObjectPtr<UButton> FpsNextButton;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> FpsValueText;
    UPROPERTY(Transient) TObjectPtr<UButton> QualityPrevButton;
    UPROPERTY(Transient) TObjectPtr<UButton> QualityNextButton;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> QualityValueText;
    UPROPERTY(Transient) TObjectPtr<UCheckBox> VSyncCheckBox;
    UPROPERTY(Transient) TObjectPtr<UCheckBox> LumenCheckBox;
    UPROPERTY(Transient) TObjectPtr<UButton> ApplyGraphicsButton;
    UPROPERTY(Transient) TObjectPtr<UButton> ResetDefaultsButton;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> GraphicsStatusText;

    // Audio & Mood Tab
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

    // Controls Tab
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
