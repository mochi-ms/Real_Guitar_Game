#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GuitarPerformanceTypes.h"
#include "GuitarPerformanceHUDWidget.generated.h"

class AGuitarPerformerActor;
class UButton;
class USlider;
class UTextBlock;

/**
 * Minimalist chill HUD base widget supporting both exploration prompts and
 * seated guitar performance controls (chord display, song switching, camera angle, live pattern studio).
 */
UCLASS(Abstract)
class REALGUITARGAME_API UGuitarPerformanceHUDWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Guitar|HUD")
    void SetInteractionPrompt(bool bVisible, const FText& PromptText);

    UFUNCTION(BlueprintCallable, Category = "Guitar|HUD")
    void SetSeatedMode(bool bInSeated, AGuitarPerformerActor* LinkedPerformer = nullptr);

    UFUNCTION(BlueprintCallable, Category = "Guitar|HUD")
    void HandlePlayPauseClicked();

    UFUNCTION(BlueprintCallable, Category = "Guitar|HUD")
    void HandleNextSongClicked();

    UFUNCTION(BlueprintCallable, Category = "Guitar|HUD")
    void HandlePrevSongClicked();

    UFUNCTION(BlueprintCallable, Category = "Guitar|HUD")
    void HandleCycleCameraClicked();

    UFUNCTION(BlueprintCallable, Category = "Guitar|HUD")
    void HandlePlaybackRateChanged(float NewRate);

    UFUNCTION(BlueprintCallable, Category = "Guitar|HUD")
    void HandleGenerateCustomPattern(
        const FString& CommaSeparatedChords,
        float Bpm,
        bool bFingerstyle,
        const FString& PresetName
    );

protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    UFUNCTION(BlueprintImplementableEvent, Category = "Guitar|HUD")
    void BP_OnSeatedModeChanged(bool bIsSeated);

    UFUNCTION(BlueprintImplementableEvent, Category = "Guitar|HUD")
    void BP_OnPromptUpdated(bool bVisible, const FText& PromptText);

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
    TObjectPtr<UButton> PlayPauseButton;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Guitar|HUD")
    TObjectPtr<UButton> NextSongButton;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Guitar|HUD")
    TObjectPtr<UButton> PrevSongButton;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Guitar|HUD")
    TObjectPtr<UButton> CameraCycleButton;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Guitar|HUD")
    TObjectPtr<USlider> PlaybackRateSlider;

private:
    UPROPERTY(Transient)
    TObjectPtr<AGuitarPerformerActor> ActivePerformer;

    bool bSeated = false;
};
