#include "GuitarPerformanceHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ProgressBar.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GuitarGameSettings.h"
#include "GuitarPerformerActor.h"
#include "GuitarScoreSubsystem.h"
#include "GuitarSongLibrarySubsystem.h"
#include "GuitarUIStyle.h"

TSharedRef<SWidget> UGuitarPerformanceHUDWidget::RebuildWidget()
{
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        BuildDefaultHUDLayout();
    }
    return Super::RebuildWidget();
}

void UGuitarPerformanceHUDWidget::BuildDefaultHUDLayout()
{
    UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("HUDRootCanvas"));
    WidgetTree->RootWidget = RootCanvas;

    // 1. Top-Left Sanctuary Info Pill
    {
        UTextBlock* BrandTitle = GuitarUIStyle::MakeText(
            WidgetTree,
            NSLOCTEXT("GuitarHUD", "BrandBadge", "REAL GUITAR SANCTUARY"),
            13,
            GuitarUIStyle::AccentGold,
            true
        );
        UTextBlock* HintText = GuitarUIStyle::MakeText(
            WidgetTree,
            NSLOCTEXT("GuitarHUD", "HotkeyBadge", "[ESC] 메뉴/설정  ·  [T] 분위기  ·  [H] HUD 숨김"),
            11,
            GuitarUIStyle::TextMuted
        );
        MoodBadgeText = GuitarUIStyle::MakeText(
            WidgetTree,
            NSLOCTEXT("GuitarHUD", "MoodInitial", "황금빛 노을 (Sunset)"),
            12,
            GuitarUIStyle::TextPrimary
        );

        UVerticalBox* TopLeftVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
        TopLeftVBox->AddChildToVerticalBox(BrandTitle);
        TopLeftVBox->AddChildToVerticalBox(MoodBadgeText);
        TopLeftVBox->AddChildToVerticalBox(HintText);

        UBorder* TopLeftCard = GuitarUIStyle::MakeGlassCard(WidgetTree, TopLeftVBox, FMargin(16.0f, 10.0f));
        TopHeaderBadgeRoot = TopLeftCard;

        if (UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(TopLeftCard))
        {
            Slot->SetAnchors(FAnchors(0.0f, 0.0f));
            Slot->SetOffsets(FMargin(28.0f, 24.0f, 0.0f, 0.0f));
            Slot->SetAutoSize(true);
        }
    }

    // 2. Top-Right Chord & Tempo Card
    {
        SongTitleText = GuitarUIStyle::MakeText(WidgetTree, NSLOCTEXT("GuitarHUD", "NoSong", "악보를 선택하세요"), 15, GuitarUIStyle::TextPrimary, true);
        PlayStyleText = GuitarUIStyle::MakeText(WidgetTree, FText::FromString(TEXT("FINGERSTYLE · 90 BPM")), 11, GuitarUIStyle::AccentGold);

        UHorizontalBox* ChordRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
        CurrentChordText = GuitarUIStyle::MakeText(WidgetTree, FText::FromString(TEXT("-")), 28, GuitarUIStyle::TextPrimary, true);
        NextChordText = GuitarUIStyle::MakeText(WidgetTree, FText::FromString(TEXT("-")), 16, GuitarUIStyle::TextMuted);
        UTextBlock* ArrowTB = GuitarUIStyle::MakeText(WidgetTree, FText::FromString(TEXT("  ->  ")), 14, GuitarUIStyle::TextMuted);

        if (UHorizontalBoxSlot* S1 = ChordRow->AddChildToHorizontalBox(CurrentChordText))
        {
            S1->SetVerticalAlignment(VAlign_Bottom);
        }
        if (UHorizontalBoxSlot* S2 = ChordRow->AddChildToHorizontalBox(ArrowTB))
        {
            S2->SetVerticalAlignment(VAlign_Center);
        }
        if (UHorizontalBoxSlot* S3 = ChordRow->AddChildToHorizontalBox(NextChordText))
        {
            S3->SetVerticalAlignment(VAlign_Bottom);
        }

        UVerticalBox* TopRightVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
        TopRightVBox->AddChildToVerticalBox(SongTitleText);
        TopRightVBox->AddChildToVerticalBox(PlayStyleText);
        TopRightVBox->AddChildToVerticalBox(ChordRow);

        UBorder* TopRightCard = GuitarUIStyle::MakeGlassCard(WidgetTree, TopRightVBox, FMargin(18.0f, 12.0f));
        ChordCardRoot = TopRightCard;

        if (UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(TopRightCard))
        {
            Slot->SetAnchors(FAnchors(1.0f, 0.0f));
            Slot->SetAlignment(FVector2D(1.0f, 0.0f));
            Slot->SetOffsets(FMargin(-28.0f, 24.0f, 0.0f, 0.0f));
            Slot->SetAutoSize(true);
        }
    }

    // 3. Center-Bottom Interaction Prompt Pill
    {
        InteractionPromptText = GuitarUIStyle::MakeText(
            WidgetTree,
            NSLOCTEXT("GuitarHUD", "DefaultPrompt", "[E] 벤치에 앉기"),
            15,
            GuitarUIStyle::TextPrimary,
            true
        );
        UBorder* PromptCard = GuitarUIStyle::MakeGlassCard(WidgetTree, InteractionPromptText, FMargin(22.0f, 10.0f), GuitarUIStyle::CardSurface);
        InteractionPromptPill = PromptCard;

        if (UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(PromptCard))
        {
            Slot->SetAnchors(FAnchors(0.5f, 1.0f));
            Slot->SetAlignment(FVector2D(0.5f, 1.0f));
            Slot->SetOffsets(FMargin(0.0f, -155.0f, 0.0f, 0.0f));
            Slot->SetAutoSize(true);
        }
    }

    // 4. Bottom Seated Player Dock
    {
        UVerticalBox* DockVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());

        // Row A: Camera view + Time + Progress bar
        UHorizontalBox* MetaRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
        CameraModeText = GuitarUIStyle::MakeText(WidgetTree, NSLOCTEXT("GuitarHUD", "CamOverview", "전체 감상 뷰"), 12, GuitarUIStyle::AccentGold);
        TimeProgressText = GuitarUIStyle::MakeText(WidgetTree, FText::FromString(TEXT("0.0s / 0.0s")), 12, GuitarUIStyle::TextMuted);
        PlaybackRateValueText = GuitarUIStyle::MakeText(WidgetTree, FText::FromString(TEXT("1.00x")), 12, GuitarUIStyle::TextPrimary);

        if (UHorizontalBoxSlot* S = MetaRow->AddChildToHorizontalBox(CameraModeText))
        {
            S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        }
        if (UHorizontalBoxSlot* S = MetaRow->AddChildToHorizontalBox(PlaybackRateValueText))
        {
            S->SetPadding(FMargin(0.0f, 0.0f, 14.0f, 0.0f));
        }
        MetaRow->AddChildToHorizontalBox(TimeProgressText);
        DockVBox->AddChildToVerticalBox(MetaRow);

        SongProgressBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass());
        SongProgressBar->SetFillColorAndOpacity(GuitarUIStyle::AccentGold);
        SongProgressBar->SetPercent(0.0f);
        if (UVerticalBoxSlot* PSlot = DockVBox->AddChildToVerticalBox(SongProgressBar))
        {
            PSlot->SetPadding(FMargin(0.0f, 6.0f, 0.0f, 10.0f));
        }

        // Row B: Buttons & Tempo Slider
        UHorizontalBox* ControlsRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

        UTextBlock* PrevLbl = nullptr;
        PrevSongButton = GuitarUIStyle::MakeButton(WidgetTree, NSLOCTEXT("GuitarHUD", "Prev", "|< 이전 곡"), PrevLbl, false, 12);
        PlayPauseButton = GuitarUIStyle::MakeButton(WidgetTree, NSLOCTEXT("GuitarHUD", "Play", "▶ 재생"), PlayPauseButtonLabel, true, 13);
        UTextBlock* NextLbl = nullptr;
        NextSongButton = GuitarUIStyle::MakeButton(WidgetTree, NSLOCTEXT("GuitarHUD", "Next", "다음 곡 >|"), NextLbl, false, 12);
        UTextBlock* CamLbl = nullptr;
        CameraCycleButton = GuitarUIStyle::MakeButton(WidgetTree, NSLOCTEXT("GuitarHUD", "Cam", "카메라 [C]"), CamLbl, false, 12);
        UTextBlock* MoodLbl = nullptr;
        MoodCycleButton = GuitarUIStyle::MakeButton(WidgetTree, NSLOCTEXT("GuitarHUD", "Mood", "분위기 [T]"), MoodLbl, false, 12);
        UTextBlock* SetLbl = nullptr;
        OpenSettingsButton = GuitarUIStyle::MakeButton(WidgetTree, NSLOCTEXT("GuitarHUD", "Settings", "설정 [ESC]"), SetLbl, false, 12);

        PlaybackRateSlider = GuitarUIStyle::MakeStyledSlider(WidgetTree, 0.5f, 1.5f, 0.05f);

        auto AddBtn = [&](UButton* Btn)
        {
            if (UHorizontalBoxSlot* Slot = ControlsRow->AddChildToHorizontalBox(Btn))
            {
                Slot->SetVerticalAlignment(VAlign_Center);
                Slot->SetPadding(FMargin(4.0f, 0.0f));
            }
        };

        AddBtn(PrevSongButton);
        AddBtn(PlayPauseButton);
        AddBtn(NextSongButton);

        if (UHorizontalBoxSlot* SliderSlot = ControlsRow->AddChildToHorizontalBox(PlaybackRateSlider))
        {
            SliderSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            SliderSlot->SetVerticalAlignment(VAlign_Center);
            SliderSlot->SetPadding(FMargin(14.0f, 0.0f));
        }

        AddBtn(CameraCycleButton);
        AddBtn(MoodCycleButton);
        AddBtn(OpenSettingsButton);

        DockVBox->AddChildToVerticalBox(ControlsRow);

        UBorder* DockCard = GuitarUIStyle::MakeGlassCard(WidgetTree, DockVBox, FMargin(20.0f, 12.0f));
        SeatedControlBarRoot = DockCard;

        if (UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(DockCard))
        {
            Slot->SetAnchors(FAnchors(0.5f, 1.0f));
            Slot->SetAlignment(FVector2D(0.5f, 1.0f));
            Slot->SetOffsets(FMargin(0.0f, -24.0f, 820.0f, 0.0f));
            Slot->SetAutoSize(true);
        }
    }
}

void UGuitarPerformanceHUDWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (PlayPauseButton && !PlayPauseButton->OnClicked.IsAlreadyBound(this, &UGuitarPerformanceHUDWidget::HandlePlayPauseClicked))
    {
        PlayPauseButton->OnClicked.AddDynamic(this, &UGuitarPerformanceHUDWidget::HandlePlayPauseClicked);
    }
    if (NextSongButton && !NextSongButton->OnClicked.IsAlreadyBound(this, &UGuitarPerformanceHUDWidget::HandleNextSongClicked))
    {
        NextSongButton->OnClicked.AddDynamic(this, &UGuitarPerformanceHUDWidget::HandleNextSongClicked);
    }
    if (PrevSongButton && !PrevSongButton->OnClicked.IsAlreadyBound(this, &UGuitarPerformanceHUDWidget::HandlePrevSongClicked))
    {
        PrevSongButton->OnClicked.AddDynamic(this, &UGuitarPerformanceHUDWidget::HandlePrevSongClicked);
    }
    if (CameraCycleButton && !CameraCycleButton->OnClicked.IsAlreadyBound(this, &UGuitarPerformanceHUDWidget::HandleCycleCameraClicked))
    {
        CameraCycleButton->OnClicked.AddDynamic(this, &UGuitarPerformanceHUDWidget::HandleCycleCameraClicked);
    }
    if (MoodCycleButton && !MoodCycleButton->OnClicked.IsAlreadyBound(this, &UGuitarPerformanceHUDWidget::HandleCycleMoodClicked))
    {
        MoodCycleButton->OnClicked.AddDynamic(this, &UGuitarPerformanceHUDWidget::HandleCycleMoodClicked);
    }
    if (OpenSettingsButton && !OpenSettingsButton->OnClicked.IsAlreadyBound(this, &UGuitarPerformanceHUDWidget::HandleOpenSettingsClicked))
    {
        OpenSettingsButton->OnClicked.AddDynamic(this, &UGuitarPerformanceHUDWidget::HandleOpenSettingsClicked);
    }
    if (PlaybackRateSlider)
    {
        PlaybackRateSlider->SetMinValue(0.5f);
        PlaybackRateSlider->SetMaxValue(1.5f);
        PlaybackRateSlider->SetValue(1.0f);
        if (!PlaybackRateSlider->OnValueChanged.IsAlreadyBound(this, &UGuitarPerformanceHUDWidget::HandlePlaybackRateChanged))
        {
            PlaybackRateSlider->OnValueChanged.AddDynamic(this, &UGuitarPerformanceHUDWidget::HandlePlaybackRateChanged);
        }
    }

    SetInteractionPrompt(false, FText::GetEmpty());
    RefreshVisibilityState();
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

    const float CurSec = ScoreSub->GetCurrentTimeSec();
    const float TotalSec = FMath::Max(0.0f, Data.DurationSec);
    if (TimeProgressText)
    {
        TimeProgressText->SetText(FText::FromString(
            FString::Printf(TEXT("%.1fs / %.1fs"), CurSec, TotalSec)
        ));
    }
    if (SongProgressBar)
    {
        SongProgressBar->SetPercent(TotalSec > KINDA_SMALL_NUMBER ? FMath::Clamp(CurSec / TotalSec, 0.0f, 1.0f) : 0.0f);
    }
    if (PlayPauseButtonLabel)
    {
        PlayPauseButtonLabel->SetText(ScoreSub->IsPlaying()
            ? NSLOCTEXT("GuitarHUD", "PauseLbl", "❚❚ 일시정지")
            : NSLOCTEXT("GuitarHUD", "PlayLbl", "▶ 재생"));
    }
    if (PlaybackRateValueText)
    {
        PlaybackRateValueText->SetText(FText::FromString(
            FString::Printf(TEXT("템포 %.2fx"), ScoreSub->GetPlaybackRate())
        ));
    }

    FGuitarChordSnapshot CurrChord;
    if (CurrentChordText)
    {
        CurrentChordText->SetText(ScoreSub->GetCurrentChord(CurrChord)
            ? FText::FromString(CurrChord.Symbol)
            : FText::FromString(TEXT("-")));
    }
    if (NextChordText)
    {
        FGuitarChordSnapshot NextC;
        NextChordText->SetText(ScoreSub->GetNextChord(NextC)
            ? FText::FromString(NextC.Symbol)
            : FText::FromString(TEXT("-")));
    }
    if (CameraModeText && ActivePerformer)
    {
        CameraModeText->SetText(ActivePerformer->GetActiveCameraDisplayName());
    }

    bool bShowChords = true;
    if (const UGameInstance* GI = GetGameInstance())
    {
        if (const UGuitarGameSettingsSubsystem* SettingsSub = GI->GetSubsystem<UGuitarGameSettingsSubsystem>())
        {
            const FGuitarSettingsData& S = SettingsSub->GetSettings();
            bShowChords = S.bShowChordDiagrams;
            if (MoodBadgeText)
            {
                MoodBadgeText->SetText(UGuitarGameSettingsSubsystem::GetMoodDisplayName(S.AtmosphereMood));
            }
        }
    }

    if (ChordCardRoot)
    {
        ChordCardRoot->SetVisibility((!bCinematicHudHidden && bShowChords)
            ? ESlateVisibility::SelfHitTestInvisible
            : ESlateVisibility::Collapsed);
    }
}

void UGuitarPerformanceHUDWidget::RefreshVisibilityState()
{
    if (TopHeaderBadgeRoot)
    {
        TopHeaderBadgeRoot->SetVisibility(bCinematicHudHidden
            ? ESlateVisibility::Collapsed
            : ESlateVisibility::SelfHitTestInvisible);
    }
    if (SeatedControlBarRoot)
    {
        SeatedControlBarRoot->SetVisibility((bSeated && !bCinematicHudHidden)
            ? ESlateVisibility::Visible
            : ESlateVisibility::Collapsed);
    }
    if (InteractionPromptPill)
    {
        InteractionPromptPill->SetVisibility((bPromptCurrentlyVisible && !bCinematicHudHidden)
            ? ESlateVisibility::SelfHitTestInvisible
            : ESlateVisibility::Collapsed);
    }
}

void UGuitarPerformanceHUDWidget::SetInteractionPrompt(bool bVisible, const FText& PromptText)
{
    bPromptCurrentlyVisible = bVisible;
    if (InteractionPromptText)
    {
        InteractionPromptText->SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
        InteractionPromptText->SetText(PromptText);
    }
    RefreshVisibilityState();
    BP_OnPromptUpdated(bVisible, PromptText);
}

void UGuitarPerformanceHUDWidget::SetSeatedMode(bool bInSeated, AGuitarPerformerActor* LinkedPerformer)
{
    bSeated = bInSeated;
    ActivePerformer = LinkedPerformer;

    if (const UGameInstance* GI = GetGameInstance())
    {
        if (const UGuitarGameSettingsSubsystem* SettingsSub = GI->GetSubsystem<UGuitarGameSettingsSubsystem>())
        {
            if (SettingsSub->GetSettings().bAutoHideHudWhenSeated)
            {
                bCinematicHudHidden = bSeated;
            }
        }
    }

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

    RefreshVisibilityState();
    BP_OnSeatedModeChanged(bSeated);
}

void UGuitarPerformanceHUDWidget::SetCinematicHudHidden(bool bInHidden)
{
    bCinematicHudHidden = bInHidden;
    RefreshVisibilityState();
}

bool UGuitarPerformanceHUDWidget::ToggleCinematicHud()
{
    SetCinematicHudHidden(!bCinematicHudHidden);
    return bCinematicHudHidden;
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

void UGuitarPerformanceHUDWidget::HandleCycleMoodClicked()
{
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UGuitarGameSettingsSubsystem* SettingsSub = GI->GetSubsystem<UGuitarGameSettingsSubsystem>())
        {
            SettingsSub->CycleAtmosphereMood();
        }
    }
}

void UGuitarPerformanceHUDWidget::HandleOpenSettingsClicked()
{
    OnRequestPauseMenu.Broadcast();
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
