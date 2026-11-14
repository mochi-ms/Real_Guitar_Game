#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GuitarPerformanceTypes.h"
#include "GuitarPerformanceHUDWidget.generated.h"

class AGuitarPerformerActor;
class UBorder;
class UButton;
class UProgressBar;
class USlider;
class UTextBlock;
class UWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHUDRequestPauseMenu);

/**
 * Glassmorphic Sanctuary Guitar HUD supporting both 3rd-person exploration hints,
 * an interactive Live Play deck (8-chord pad, 6-string pluck & strum controls),
 * a real-time 6-string fretboard & vibration meter, and Auto-Song transport controls.
 */
UCLASS()
class REALGUITARGAME_API UGuitarPerformanceHUDWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Guitar|HUD")
    void SetInteractionPrompt(bool bVisible, const FText& PromptText);

    UFUNCTION(BlueprintCallable, Category = "Guitar|HUD")
    void SetSeatedMode(bool bInSeated, AGuitarPerformerActor* LinkedPerformer = nullptr);

    UFUNCTION(BlueprintCallable, Category = "Guitar|HUD")
    void SetGuitarState(bool bInHoldingGuitar, bool bInSeated, AGuitarPerformerActor* Performer = nullptr);

    UFUNCTION(BlueprintCallable, Category = "Guitar|HUD")
    void SetCameraLabel(const FText& LabelText);

    UFUNCTION(BlueprintCallable, Category = "Guitar|HUD")
    void SetCinematicHudHidden(bool bInHidden);

    UFUNCTION(BlueprintCallable, Category = "Guitar|HUD")
    bool ToggleCinematicHud();

    UFUNCTION(BlueprintPure, Category = "Guitar|HUD")
    bool IsCinematicHudHidden() const { return bCinematicHudHidden; }

    UFUNCTION(BlueprintCallable, Category = "Guitar|HUD")
    void HandlePlayPauseClicked();

    UFUNCTION(BlueprintCallable, Category = "Guitar|HUD")
    void HandleNextSongClicked();

    UFUNCTION(BlueprintCallable, Category = "Guitar|HUD")
    void HandlePrevSongClicked();

    UFUNCTION(BlueprintCallable, Category = "Guitar|HUD")
    void HandleCycleCameraClicked();

    UFUNCTION(BlueprintCallable, Category = "Guitar|HUD")
    void HandleCycleMoodClicked();

    UFUNCTION(BlueprintCallable, Category = "Guitar|HUD")
    void HandleOpenSettingsClicked();

    UFUNCTION(BlueprintCallable, Category = "Guitar|HUD")
    void HandleToggleHoldGuitarClicked();

    UFUNCTION(BlueprintCallable, Category = "Guitar|HUD")
    void HandleToggleControlModeClicked();

    UFUNCTION(BlueprintCallable, Category = "Guitar|HUD")
    void HandleStrumDownClicked();

    UFUNCTION(BlueprintCallable, Category = "Guitar|HUD")
    void HandleStrumUpClicked();

    UFUNCTION(BlueprintCallable, Category = "Guitar|HUD")
    void HandlePercussiveClicked();

    UFUNCTION(BlueprintCallable, Category = "Guitar|HUD")
    void HandleBankPrevClicked();

    UFUNCTION(BlueprintCallable, Category = "Guitar|HUD")
    void HandleBankNextClicked();

    UFUNCTION(BlueprintCallable, Category = "Guitar|HUD")
    void HandlePlaybackRateChanged(float NewRate);

    UFUNCTION(BlueprintCallable, Category = "Guitar|HUD")
    void HandleGenerateCustomPattern(
        const FString& CommaSeparatedChords,
        float Bpm,
        bool bFingerstyle,
        const FString& PresetName
    );

    UPROPERTY(BlueprintAssignable, Category = "Guitar|HUD")
    FOnHUDRequestPauseMenu OnRequestPauseMenu;

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    UFUNCTION(BlueprintImplementableEvent, Category = "Guitar|HUD")
    void BP_OnSeatedModeChanged(bool bIsSeated);

    UFUNCTION(BlueprintImplementableEvent, Category = "Guitar|HUD")
    void BP_OnPromptUpdated(bool bVisible, const FText& PromptText);

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Guitar|HUD")
    TObjectPtr<UWidget> TopHeaderBadgeRoot;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Guitar|HUD")
    TObjectPtr<UWidget> ChordCardRoot;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Guitar|HUD")
    TObjectPtr<UWidget> InteractionPromptPill;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Guitar|HUD")
    TObjectPtr<UWidget> SeatedControlBarRoot;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Guitar|HUD")
    TObjectPtr<UTextBlock> InteractionPromptText;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Guitar|HUD")
    TObjectPtr<UTextBlock> SongTitleText;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Guitar|HUD")
    TObjectPtr<UTextBlock> CurrentChordText;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Guitar|HUD")
    TObjectPtr<UTextBlock> NextChordText;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Guitar|HUD")
    TObjectPtr<UTextBlock> PlayStyleText;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Guitar|HUD")
    TObjectPtr<UTextBlock> TimeProgressText;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Guitar|HUD")
    TObjectPtr<UTextBlock> CameraModeText;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Guitar|HUD")
    TObjectPtr<UTextBlock> MoodBadgeText;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Guitar|HUD")
    TObjectPtr<UTextBlock> PlayPauseButtonLabel;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Guitar|HUD")
    TObjectPtr<UTextBlock> PlaybackRateValueText;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Guitar|HUD")
    TObjectPtr<UProgressBar> SongProgressBar;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Guitar|HUD")
    TObjectPtr<UButton> PlayPauseButton;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Guitar|HUD")
    TObjectPtr<UButton> NextSongButton;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Guitar|HUD")
    TObjectPtr<UButton> PrevSongButton;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Guitar|HUD")
    TObjectPtr<UButton> CameraCycleButton;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Guitar|HUD")
    TObjectPtr<UButton> MoodCycleButton;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Guitar|HUD")
    TObjectPtr<UButton> OpenSettingsButton;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Guitar|HUD")
    TObjectPtr<USlider> PlaybackRateSlider;

private:
    UFUNCTION() void HandleGuitarEventImpulse(const FGuitarEvent& Event);

    // Interactive chord & pluck button handlers
    UFUNCTION() void HandleChordSlot0() { TriggerChordSlot(0); }
    UFUNCTION() void HandleChordSlot1() { TriggerChordSlot(1); }
    UFUNCTION() void HandleChordSlot2() { TriggerChordSlot(2); }
    UFUNCTION() void HandleChordSlot3() { TriggerChordSlot(3); }
    UFUNCTION() void HandleChordSlot4() { TriggerChordSlot(4); }
    UFUNCTION() void HandleChordSlot5() { TriggerChordSlot(5); }
    UFUNCTION() void HandleChordSlot6() { TriggerChordSlot(6); }
    UFUNCTION() void HandleChordSlot7() { TriggerChordSlot(7); }

    UFUNCTION() void HandlePluckString1() { TriggerPluckString(1); }
    UFUNCTION() void HandlePluckString2() { TriggerPluckString(2); }
    UFUNCTION() void HandlePluckString3() { TriggerPluckString(3); }
    UFUNCTION() void HandlePluckString4() { TriggerPluckString(4); }
    UFUNCTION() void HandlePluckString5() { TriggerPluckString(5); }
    UFUNCTION() void HandlePluckString6() { TriggerPluckString(6); }

    void TriggerChordSlot(int32 SlotIndex);
    void TriggerPluckString(int32 String1To6);
    void BuildDefaultHUDLayout();
    void RefreshVisibilityState();
    void RefreshChordBankVisuals();

    UPROPERTY(Transient) TObjectPtr<AGuitarPerformerActor> ActivePerformer;
    UPROPERTY(Transient) TObjectPtr<UWidget> LivePlayDeckRoot;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> GuitarStanceBadgeText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ChordBankTitleText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ModeToggleLabelText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> CursorHintText;
    UPROPERTY(Transient) TObjectPtr<UButton> ToggleHoldButton;
    UPROPERTY(Transient) TObjectPtr<UButton> ToggleModeButton;
    UPROPERTY(Transient) TObjectPtr<UButton> BankPrevButton;
    UPROPERTY(Transient) TObjectPtr<UButton> BankNextButton;
    UPROPERTY(Transient) TObjectPtr<UButton> StrumDownButton;
    UPROPERTY(Transient) TObjectPtr<UButton> StrumUpButton;
    UPROPERTY(Transient) TObjectPtr<UButton> ChuckButton;

    UPROPERTY(Transient) TArray<TObjectPtr<UButton>> ChordSlotButtons;
    UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> ChordSlotTexts;
    UPROPERTY(Transient) TArray<TObjectPtr<UButton>> StringPluckButtons;
    UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> StringFretTexts;
    UPROPERTY(Transient) TArray<TObjectPtr<UProgressBar>> StringEnergyBars;

    TStaticArray<float, 6> StringFlashLevels{ InPlace, 0.0f };
    FText CachedCameraLabel;
    bool bHoldingGuitar = true;
    bool bSeated = false;
    bool bCinematicHudHidden = false;
    bool bPromptCurrentlyVisible = false;
};
