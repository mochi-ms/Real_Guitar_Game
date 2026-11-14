#include "GuitarPerformanceHUDWidget.h"

#include "Components/Button.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GuitarPerformerActor.h"
#include "GuitarScoreSubsystem.h"
#include "GuitarSongLibrarySubsystem.h"

void UGuitarPerformanceHUDWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (PlayPauseButton)
    {
        PlayPauseButton->OnClicked.AddDynamic(this, &UGuitarPerformanceHUDWidget::HandlePlayPauseClicked);
    }
    if (NextSongButton)
    {
        NextSongButton->OnClicked.AddDynamic(this, &UGuitarPerformanceHUDWidget::HandleNextSongClicked);
    }
    if (PrevSongButton)
    {
        PrevSongButton->OnClicked.AddDynamic(this, &UGuitarPerformanceHUDWidget::HandlePrevSongClicked);
    }
    if (CameraCycleButton)
    {
        CameraCycleButton->OnClicked.AddDynamic(this, &UGuitarPerformanceHUDWidget::HandleCycleCameraClicked);
    }
    if (PlaybackRateSlider)
    {
        PlaybackRateSlider->SetMinValue(0.5f);
        PlaybackRateSlider->SetMaxValue(1.5f);
        PlaybackRateSlider->SetValue(1.0f);
        PlaybackRateSlider->OnValueChanged.AddDynamic(this, &UGuitarPerformanceHUDWidget::HandlePlaybackRateChanged);
    }
    SetInteractionPrompt(false, FText::GetEmpty());
}

void UGuitarPerformanceHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    const UWorld* World = GetWorld();
    const UGuitarScoreSubsystem* ScoreSub = World ? World->GetSubsystem<UGuitarScoreSubsystem>() : nullptr;
    if (!ScoreSub)
    {
        return;
    }

    const FGuitarPerformanceData& Data = ScoreSub->GetLoadedPerformance();
    if (SongTitleText)
    {
        SongTitleText->SetText(
            Data.Title.IsEmpty() ? NSLOCTEXT("GuitarHUD", "NoSong", "악보를 선택하세요") : FText::FromString(Data.Title)
        );
    }
    if (PlayStyleText)
    {
        PlayStyleText->SetText(FText::FromString(
            FString::Printf(TEXT("%s · %.0f BPM"), *Data.PlayStyle.ToUpper(), Data.Bpm)
        ));
    }
    if (TimeProgressText)
    {
        TimeProgressText->SetText(FText::FromString(
            FString::Printf(TEXT("%.1fs / %.1fs"), ScoreSub->GetCurrentTimeSec(), Data.DurationSec)
        ));
    }

    FGuitarChordSnapshot CurrChord;
    if (CurrentChordText)
    {
        if (ScoreSub->GetCurrentChord(CurrChord))
        {
            CurrentChordText->SetText(FText::FromString(CurrChord.Symbol));
        }
        else
        {
            CurrentChordText->SetText(FText::FromString(TEXT("-")));
        }
    }
    if (NextChordText)
    {
        FGuitarChordSnapshot NextC;
        NextChordText->SetText(ScoreSub->GetNextChord(NextC) ? FText::FromString(NextC.Symbol) : FText::FromString(TEXT("-")));
    }
    if (CameraModeText && ActivePerformer)
    {
        CameraModeText->SetText(ActivePerformer->GetActiveCameraDisplayName());
    }
}

void UGuitarPerformanceHUDWidget::SetInteractionPrompt(bool bVisible, const FText& PromptText)
{
    if (InteractionPromptText)
    {
        InteractionPromptText->SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
        InteractionPromptText->SetText(PromptText);
    }
    BP_OnPromptUpdated(bVisible, PromptText);
}

void UGuitarPerformanceHUDWidget::SetSeatedMode(bool bInSeated, AGuitarPerformerActor* LinkedPerformer)
{
    bSeated = bInSeated;
    ActivePerformer = LinkedPerformer;

    if (APlayerController* PC = GetOwningPlayer())
    {
        PC->SetShowMouseCursor(bSeated);
        if (bSeated)
        {
            FInputModeGameAndUI Mode;
            Mode.SetHideCursorDuringCapture(false);
            PC->SetInputMode(Mode);
        }
        else
        {
            PC->SetInputMode(FInputModeGameOnly());
        }
    }

    BP_OnSeatedModeChanged(bSeated);
}

void UGuitarPerformanceHUDWidget::HandlePlayPauseClicked()
{
    if (UWorld* World = GetWorld())
    {
        if (UGuitarScoreSubsystem* ScoreSub = World->GetSubsystem<UGuitarScoreSubsystem>())
        {
            if (ScoreSub->IsPlaying()) ScoreSub->Pause();
            else ScoreSub->Play();
        }
    }
}

void UGuitarPerformanceHUDWidget::HandleNextSongClicked()
{
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UGuitarSongLibrarySubsystem* Lib = GI->GetSubsystem<UGuitarSongLibrarySubsystem>())
        {
            Lib->PlayNextSong();
        }
    }
}

void UGuitarPerformanceHUDWidget::HandlePrevSongClicked()
{
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UGuitarSongLibrarySubsystem* Lib = GI->GetSubsystem<UGuitarSongLibrarySubsystem>())
        {
            Lib->PlayPreviousSong();
        }
    }
}

void UGuitarPerformanceHUDWidget::HandleCycleCameraClicked()
{
    if (ActivePerformer)
    {
        ActivePerformer->CycleCameraAngle();
        if (APlayerController* PC = GetOwningPlayer())
        {
            PC->SetViewTargetWithBlend(ActivePerformer, 0.55f, EViewTargetBlendFunction::VTBlend_Cubic);
        }
    }
}

void UGuitarPerformanceHUDWidget::HandlePlaybackRateChanged(float NewRate)
{
    if (UWorld* World = GetWorld())
    {
        if (UGuitarScoreSubsystem* ScoreSub = World->GetSubsystem<UGuitarScoreSubsystem>())
        {
            ScoreSub->SetPlaybackRate(NewRate);
        }
    }
}

void UGuitarPerformanceHUDWidget::HandleGenerateCustomPattern(
    const FString& CommaSeparatedChords,
    float Bpm,
    bool bFingerstyle,
    const FString& PresetName
)
{
    TArray<FString> Chords;
    CommaSeparatedChords.ParseIntoArray(Chords, TEXT(","), true);
    for (FString& Item : Chords)
    {
        Item = Item.TrimStartAndEnd();
    }

    if (UGameInstance* GI = GetGameInstance())
    {
        if (UGuitarSongLibrarySubsystem* Lib = GI->GetSubsystem<UGuitarSongLibrarySubsystem>())
        {
            Lib->GenerateAndPlayCustomChart(Chords, Bpm, bFingerstyle, PresetName, true);
        }
    }
}
