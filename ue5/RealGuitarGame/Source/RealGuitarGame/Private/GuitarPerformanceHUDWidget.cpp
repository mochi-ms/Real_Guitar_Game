#include "GuitarPerformanceHUDWidget.h"

#include "AcousticChillCharacter.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GuitarGameSettings.h"
#include "GuitarPerformerActor.h"
#include "GuitarScoreSubsystem.h"
#include "GuitarSongLibrarySubsystem.h"
#include "GuitarUIStyle.h"
#include "Styling/SlateTypes.h"

namespace
{
    FString FormatSecToClock(float Seconds)
    {
        const int32 Total = FMath::Max(0, FMath::FloorToInt(Seconds));
        return FString::Printf(TEXT("%02d:%02d"), Total / 60, Total % 60);
    }
}

TSharedRef<SWidget> UGuitarPerformanceHUDWidget::RebuildWidget()
{
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        BuildDefaultHUDLayout();
    }
    return Super::RebuildWidget();
}

void UGuitarPerformanceHUDWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (PlayPauseButton)
        PlayPauseButton->OnClicked.AddUniqueDynamic(this, &UGuitarPerformanceHUDWidget::HandlePlayPauseClicked);
    if (NextSongButton)
        NextSongButton->OnClicked.AddUniqueDynamic(this, &UGuitarPerformanceHUDWidget::HandleNextSongClicked);
    if (PrevSongButton)
        PrevSongButton->OnClicked.AddUniqueDynamic(this, &UGuitarPerformanceHUDWidget::HandlePrevSongClicked);
    if (CameraCycleButton)
        CameraCycleButton->OnClicked.AddUniqueDynamic(this, &UGuitarPerformanceHUDWidget::HandleCycleCameraClicked);
    if (MoodCycleButton)
        MoodCycleButton->OnClicked.AddUniqueDynamic(this, &UGuitarPerformanceHUDWidget::HandleCycleMoodClicked);
    if (OpenSettingsButton)
        OpenSettingsButton->OnClicked.AddUniqueDynamic(this, &UGuitarPerformanceHUDWidget::HandleOpenSettingsClicked);
    if (ToggleHoldButton)
        ToggleHoldButton->OnClicked.AddUniqueDynamic(this, &UGuitarPerformanceHUDWidget::HandleToggleHoldGuitarClicked);
    if (ToggleModeButton)
        ToggleModeButton->OnClicked.AddUniqueDynamic(this, &UGuitarPerformanceHUDWidget::HandleToggleControlModeClicked);
    if (StrumDownButton)
        StrumDownButton->OnClicked.AddUniqueDynamic(this, &UGuitarPerformanceHUDWidget::HandleStrumDownClicked);
    if (StrumUpButton)
        StrumUpButton->OnClicked.AddUniqueDynamic(this, &UGuitarPerformanceHUDWidget::HandleStrumUpClicked);
    if (ChuckButton)
        ChuckButton->OnClicked.AddUniqueDynamic(this, &UGuitarPerformanceHUDWidget::HandlePercussiveClicked);
    if (BankPrevButton)
        BankPrevButton->OnClicked.AddUniqueDynamic(this, &UGuitarPerformanceHUDWidget::HandleBankPrevClicked);
    if (BankNextButton)
        BankNextButton->OnClicked.AddUniqueDynamic(this, &UGuitarPerformanceHUDWidget::HandleBankNextClicked);
    if (PlaybackRateSlider)
        PlaybackRateSlider->OnValueChanged.AddUniqueDynamic(this, &UGuitarPerformanceHUDWidget::HandlePlaybackRateChanged);

    // Bind 8 chord pad buttons cleanly (literal member function pointers required by AddUniqueDynamic)
    if (ChordSlotButtons.IsValidIndex(0) && ChordSlotButtons[0]) ChordSlotButtons[0]->OnClicked.AddUniqueDynamic(this, &UGuitarPerformanceHUDWidget::HandleChordSlot0);
    if (ChordSlotButtons.IsValidIndex(1) && ChordSlotButtons[1]) ChordSlotButtons[1]->OnClicked.AddUniqueDynamic(this, &UGuitarPerformanceHUDWidget::HandleChordSlot1);
    if (ChordSlotButtons.IsValidIndex(2) && ChordSlotButtons[2]) ChordSlotButtons[2]->OnClicked.AddUniqueDynamic(this, &UGuitarPerformanceHUDWidget::HandleChordSlot2);
    if (ChordSlotButtons.IsValidIndex(3) && ChordSlotButtons[3]) ChordSlotButtons[3]->OnClicked.AddUniqueDynamic(this, &UGuitarPerformanceHUDWidget::HandleChordSlot3);
    if (ChordSlotButtons.IsValidIndex(4) && ChordSlotButtons[4]) ChordSlotButtons[4]->OnClicked.AddUniqueDynamic(this, &UGuitarPerformanceHUDWidget::HandleChordSlot4);
    if (ChordSlotButtons.IsValidIndex(5) && ChordSlotButtons[5]) ChordSlotButtons[5]->OnClicked.AddUniqueDynamic(this, &UGuitarPerformanceHUDWidget::HandleChordSlot5);
    if (ChordSlotButtons.IsValidIndex(6) && ChordSlotButtons[6]) ChordSlotButtons[6]->OnClicked.AddUniqueDynamic(this, &UGuitarPerformanceHUDWidget::HandleChordSlot6);
    if (ChordSlotButtons.IsValidIndex(7) && ChordSlotButtons[7]) ChordSlotButtons[7]->OnClicked.AddUniqueDynamic(this, &UGuitarPerformanceHUDWidget::HandleChordSlot7);

    // Bind 6 string pluck buttons cleanly (Index 0..5 -> String 6..1)
    if (StringPluckButtons.IsValidIndex(0) && StringPluckButtons[0]) StringPluckButtons[0]->OnClicked.AddUniqueDynamic(this, &UGuitarPerformanceHUDWidget::HandlePluckString6);
    if (StringPluckButtons.IsValidIndex(1) && StringPluckButtons[1]) StringPluckButtons[1]->OnClicked.AddUniqueDynamic(this, &UGuitarPerformanceHUDWidget::HandlePluckString5);
    if (StringPluckButtons.IsValidIndex(2) && StringPluckButtons[2]) StringPluckButtons[2]->OnClicked.AddUniqueDynamic(this, &UGuitarPerformanceHUDWidget::HandlePluckString4);
    if (StringPluckButtons.IsValidIndex(3) && StringPluckButtons[3]) StringPluckButtons[3]->OnClicked.AddUniqueDynamic(this, &UGuitarPerformanceHUDWidget::HandlePluckString3);
    if (StringPluckButtons.IsValidIndex(4) && StringPluckButtons[4]) StringPluckButtons[4]->OnClicked.AddUniqueDynamic(this, &UGuitarPerformanceHUDWidget::HandlePluckString2);
    if (StringPluckButtons.IsValidIndex(5) && StringPluckButtons[5]) StringPluckButtons[5]->OnClicked.AddUniqueDynamic(this, &UGuitarPerformanceHUDWidget::HandlePluckString1);

    if (UWorld* World = GetWorld())
    {
        if (UGuitarScoreSubsystem* ScoreSub = World->GetSubsystem<UGuitarScoreSubsystem>())
        {
            ScoreSub->OnGuitarEventTriggered.AddUniqueDynamic(this, &UGuitarPerformanceHUDWidget::HandleGuitarEventImpulse);
        }
    }

    RefreshChordBankVisuals();
    RefreshVisibilityState();
}

void UGuitarPerformanceHUDWidget::NativeDestruct()
{
    if (UWorld* World = GetWorld())
    {
        if (UGuitarScoreSubsystem* ScoreSub = World->GetSubsystem<UGuitarScoreSubsystem>())
        {
            ScoreSub->OnGuitarEventTriggered.RemoveDynamic(this, &UGuitarPerformanceHUDWidget::HandleGuitarEventImpulse);
        }
    }
    Super::NativeDestruct();
}

void UGuitarPerformanceHUDWidget::BuildDefaultHUDLayout()
{
    UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
    WidgetTree->RootWidget = RootCanvas;

    // ---------------------------------------------------------
    // 1. TOP-LEFT SANCTUARY & PERFORMER STATUS BADGE
    // ---------------------------------------------------------
    UVerticalBox* HeaderBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    SongTitleText = GuitarUIStyle::MakeText(WidgetTree, FText::FromString(TEXT("Acoustic Sanctuary — Free Play [자유 연주]")), 16, GuitarUIStyle::TextPrimary, true);
    HeaderBox->AddChild(SongTitleText);

    GuitarUIStyle::AddSpacer(WidgetTree, HeaderBox, 1.0f, 3.0f);

    UHorizontalBox* SubBadgeRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    PlayStyleText = GuitarUIStyle::MakeText(WidgetTree, FText::FromString(TEXT("FREEPLAY")), 11, GuitarUIStyle::AccentGold, true);
    GuitarStanceBadgeText = GuitarUIStyle::MakeText(WidgetTree, FText::FromString(TEXT("  |  기타 연주 자세 [G]  |  ")), 11, GuitarUIStyle::TextMuted, false);
    CameraModeText = GuitarUIStyle::MakeText(WidgetTree, FText::FromString(TEXT("3인칭 자유 시점")), 11, GuitarUIStyle::AccentTeal, true);
    MoodBadgeText = GuitarUIStyle::MakeText(WidgetTree, FText::FromString(TEXT("  |  골든 선셋")), 11, GuitarUIStyle::TextMuted, false);
    SubBadgeRow->AddChild(PlayStyleText);
    SubBadgeRow->AddChild(GuitarStanceBadgeText);
    SubBadgeRow->AddChild(CameraModeText);
    SubBadgeRow->AddChild(MoodBadgeText);
    HeaderBox->AddChild(SubBadgeRow);

    GuitarUIStyle::AddSpacer(WidgetTree, HeaderBox, 1.0f, 8.0f);

    UHorizontalBox* QuickActionRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    CameraCycleButton = GuitarUIStyle::MakeIconButton(WidgetTree, QuickActionRow, TEXT("/Game/UI/Textures/T_UI_Camera.T_UI_Camera"), FText::FromString(TEXT("시점 [C]")), false, 15.0f, 11);
    GuitarUIStyle::AddSpacer(WidgetTree, QuickActionRow, 6.0f, 1.0f);
    MoodCycleButton = GuitarUIStyle::MakeIconButton(WidgetTree, QuickActionRow, TEXT("/Game/UI/Textures/T_UI_MoodSunset.T_UI_MoodSunset"), FText::FromString(TEXT("분위기 [T]")), false, 15.0f, 11);
    GuitarUIStyle::AddSpacer(WidgetTree, QuickActionRow, 6.0f, 1.0f);
    ToggleHoldButton = GuitarUIStyle::MakeIconButton(WidgetTree, QuickActionRow, TEXT("/Game/UI/Textures/T_UI_SitBench.T_UI_SitBench"), FText::FromString(TEXT("기타 들기/메기 [G]")), false, 15.0f, 11);
    GuitarUIStyle::AddSpacer(WidgetTree, QuickActionRow, 6.0f, 1.0f);
    OpenSettingsButton = GuitarUIStyle::MakeIconButton(WidgetTree, QuickActionRow, TEXT("/Game/UI/Textures/T_UI_Settings.T_UI_Settings"), FText::FromString(TEXT("메뉴 [Esc]")), true, 15.0f, 11);
    HeaderBox->AddChild(QuickActionRow);

    UBorder* TopLeftCard = GuitarUIStyle::MakeGlassCard(WidgetTree, HeaderBox, FMargin(16.0f, 11.0f), GuitarUIStyle::GlassPanel);
    TopHeaderBadgeRoot = TopLeftCard;
    if (UCanvasPanelSlot* TLSlot = RootCanvas->AddChildToCanvas(TopLeftCard))
    {
        TLSlot->SetAnchors(FAnchors(0.0f, 0.0f));
        TLSlot->SetPosition(FVector2D(24.0f, 20.0f));
        TLSlot->SetAutoSize(true);
    }

    // ---------------------------------------------------------
    // 2. TOP-RIGHT LIVE 6-STRING VISUAL FRETBOARD & CHORD CARD
    // ---------------------------------------------------------
    UVerticalBox* ChordDiagBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());

    UHorizontalBox* ChordHeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    UVerticalBox* ChordTitleCol = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    ChordTitleCol->AddChild(GuitarUIStyle::MakeText(WidgetTree, FText::FromString(TEXT("LIVE FRETBOARD")), 10, GuitarUIStyle::TextMuted, true));
    CurrentChordText = GuitarUIStyle::MakeText(WidgetTree, FText::FromString(TEXT("C")), 26, GuitarUIStyle::AccentGold, true);
    ChordTitleCol->AddChild(CurrentChordText);
    UHorizontalBoxSlot* CTColSlot = ChordHeaderRow->AddChildToHorizontalBox(ChordTitleCol);
    CTColSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    UVerticalBox* NextBankCol = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    NextChordText = GuitarUIStyle::MakeText(WidgetTree, FText::FromString(TEXT("다음: G")), 12, GuitarUIStyle::TextPrimary, false);
    ChordBankTitleText = GuitarUIStyle::MakeText(WidgetTree, FText::FromString(TEXT("[ ] Sunset Acoustic")), 10, GuitarUIStyle::AccentTeal, true);
    NextBankCol->AddChild(NextChordText);
    NextBankCol->AddChild(ChordBankTitleText);
    ChordHeaderRow->AddChildToHorizontalBox(NextBankCol)->SetVerticalAlignment(VAlign_Center);
    ChordDiagBox->AddChild(ChordHeaderRow);

    GuitarUIStyle::AddSpacer(WidgetTree, ChordDiagBox, 1.0f, 6.0f);

    // 6 live string lanes (Top = High e1 .. Bottom = Low E6 like standard TAB!)
    static const TCHAR* StringLabels[6] = {
        TEXT("1 e4 [L]"), TEXT("2 B3 [K]"), TEXT("3 G3 [J]"),
        TEXT("4 D3 [O]"), TEXT("5 A2 [I]"), TEXT("6 E2 [U]")
    };
    StringFretTexts.SetNum(6);
    StringEnergyBars.SetNum(6);
    for (int32 RowIdx = 0; RowIdx < 6; ++RowIdx)
    {
        UHorizontalBox* StrRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
        UTextBlock* NameTxt = GuitarUIStyle::MakeText(WidgetTree, FText::FromString(StringLabels[RowIdx]), 11, GuitarUIStyle::TextMuted, true);
        USizeBox* NameSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        NameSize->SetWidthOverride(64.0f);
        NameSize->AddChild(NameTxt);
        StrRow->AddChildToHorizontalBox(NameSize)->SetVerticalAlignment(VAlign_Center);

        UTextBlock* FretTxt = GuitarUIStyle::MakeText(WidgetTree, FText::FromString(TEXT("0")), 12, GuitarUIStyle::TextPrimary, true);
        FretTxt->SetJustification(ETextJustify::Center);
        USizeBox* FretSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        FretSize->SetWidthOverride(34.0f);
        FretSize->AddChild(FretTxt);
        StrRow->AddChildToHorizontalBox(FretSize)->SetVerticalAlignment(VAlign_Center);
        StringFretTexts[RowIdx] = FretTxt;

        UProgressBar* StrMeter = GuitarUIStyle::MakeProgressPill(
            WidgetTree,
            RowIdx < 3 ? GuitarUIStyle::AccentTeal : GuitarUIStyle::AccentGold);
        USizeBox* MeterSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        MeterSize->SetWidthOverride(132.0f);
        MeterSize->SetHeightOverride(7.0f);
        MeterSize->AddChild(StrMeter);
        StrRow->AddChildToHorizontalBox(MeterSize)->SetVerticalAlignment(VAlign_Center);
        StringEnergyBars[RowIdx] = StrMeter;

        ChordDiagBox->AddChild(StrRow);
        if (RowIdx < 5)
        {
            GuitarUIStyle::AddSpacer(WidgetTree, ChordDiagBox, 1.0f, 2.0f);
        }
    }

    UBorder* TopRightCard = GuitarUIStyle::MakeGlassCard(WidgetTree, ChordDiagBox, FMargin(16.0f, 11.0f), GuitarUIStyle::GlassPanel);
    ChordCardRoot = TopRightCard;
    if (UCanvasPanelSlot* TRSlot = RootCanvas->AddChildToCanvas(TopRightCard))
    {
        TRSlot->SetAnchors(FAnchors(1.0f, 0.0f));
        TRSlot->SetAlignment(FVector2D(1.0f, 0.0f));
        TRSlot->SetPosition(FVector2D(-24.0f, 20.0f));
        TRSlot->SetAutoSize(true);
    }

    // ---------------------------------------------------------
    // 3. CENTER-LOWER INTERACTION PROMPT PILL
    // ---------------------------------------------------------
    InteractionPromptText = GuitarUIStyle::MakeText(WidgetTree, FText::FromString(TEXT("[E] 앉아서 기타 연주하기")), 15, GuitarUIStyle::TextPrimary, true);
    UBorder* PromptCard = GuitarUIStyle::MakeGlassCard(WidgetTree, InteractionPromptText, FMargin(20.0f, 9.0f), GuitarUIStyle::CardSurface);
    InteractionPromptPill = PromptCard;
    InteractionPromptPill->SetVisibility(ESlateVisibility::Collapsed);
    if (UCanvasPanelSlot* PromptSlot = RootCanvas->AddChildToCanvas(PromptCard))
    {
        PromptSlot->SetAnchors(FAnchors(0.5f, 0.72f));
        PromptSlot->SetAlignment(FVector2D(0.5f, 0.5f));
        PromptSlot->SetPosition(FVector2D(0.0f, 0.0f));
        PromptSlot->SetAutoSize(true);
    }

    // ---------------------------------------------------------
    // 4. BOTTOM INTERACTIVE GUITARIST PERFORMANCE & EXPLORE DOCK
    // ---------------------------------------------------------
    UVerticalBox* BottomStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());

    // Row A: Manual Chords [1..8] + String Plucks [U..L] + Strums [LMB/RMB/X]
    UVerticalBox* PlayDeckInner = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    UHorizontalBox* ChordPadRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

    BankPrevButton = GuitarUIStyle::MakeButton(WidgetTree, ChordPadRow, FText::FromString(TEXT("[")), false, 12, FMargin(9.0f, 5.0f));
    GuitarUIStyle::AddSpacer(WidgetTree, ChordPadRow, 4.0f, 1.0f);

    ChordSlotButtons.SetNum(8);
    ChordSlotTexts.SetNum(8);
    for (int32 SlotIdx = 0; SlotIdx < 8; ++SlotIdx)
    {
        UButton* CBtn = GuitarUIStyle::MakeSimpleButton(
            WidgetTree,
            FText::FromString(FString::Printf(TEXT("%d: C"), SlotIdx + 1)),
            SlotIdx == 0,
            12,
            FMargin(10.0f, 6.0f));
        // Retrieve button's inner text label cleanly
        if (UBorder* Inner = Cast<UBorder>(CBtn->GetChildAt(0)))
        {
            ChordSlotTexts[SlotIdx] = Cast<UTextBlock>(Inner->GetContent());
        }
        ChordSlotButtons[SlotIdx] = CBtn;
        ChordPadRow->AddChildToHorizontalBox(CBtn)->SetVerticalAlignment(VAlign_Center);
        GuitarUIStyle::AddSpacer(WidgetTree, ChordPadRow, 4.0f, 1.0f);
    }

    BankNextButton = GuitarUIStyle::MakeButton(WidgetTree, ChordPadRow, FText::FromString(TEXT("]")), false, 12, FMargin(9.0f, 5.0f));
    GuitarUIStyle::AddSpacer(WidgetTree, ChordPadRow, 14.0f, 1.0f);

    // String pluck keys: Low 6 -> High 1
    static const TCHAR* PluckKeyLabels[6] = {
        TEXT("U:6"), TEXT("I:5"), TEXT("O:4"), TEXT("J:3"), TEXT("K:2"), TEXT("L:1")
    };
    StringPluckButtons.SetNum(6);
    for (int32 k = 0; k < 6; ++k)
    {
        StringPluckButtons[k] = GuitarUIStyle::MakeButton(WidgetTree, ChordPadRow, FText::FromString(PluckKeyLabels[k]), false, 11, FMargin(8.0f, 6.0f));
        GuitarUIStyle::AddSpacer(WidgetTree, ChordPadRow, 3.0f, 1.0f);
    }

    GuitarUIStyle::AddSpacer(WidgetTree, ChordPadRow, 8.0f, 1.0f);
    StrumDownButton = GuitarUIStyle::MakeButton(WidgetTree, ChordPadRow, FText::FromString(TEXT("다운 스트럼 [LMB/Space]")), true, 12, FMargin(11.0f, 6.0f));
    GuitarUIStyle::AddSpacer(WidgetTree, ChordPadRow, 4.0f, 1.0f);
    StrumUpButton = GuitarUIStyle::MakeButton(WidgetTree, ChordPadRow, FText::FromString(TEXT("업 [RMB]")), false, 12, FMargin(10.0f, 6.0f));
    GuitarUIStyle::AddSpacer(WidgetTree, ChordPadRow, 4.0f, 1.0f);
    ChuckButton = GuitarUIStyle::MakeButton(WidgetTree, ChordPadRow, FText::FromString(TEXT("커팅 [X]")), false, 12, FMargin(10.0f, 6.0f));

    PlayDeckInner->AddChild(ChordPadRow);
    LivePlayDeckRoot = PlayDeckInner;
    BottomStack->AddChild(PlayDeckInner);

    GuitarUIStyle::AddSpacer(WidgetTree, BottomStack, 1.0f, 7.0f);

    // Row B: Auto-Song Transport + Explore / Cursor Keys
    UHorizontalBox* TransportRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

    ToggleModeButton = GuitarUIStyle::MakeSimpleButton(WidgetTree, FText::FromString(TEXT("모드 [M]: 자유 연주")), false, 12, FMargin(10.0f, 5.0f));
    if (UBorder* ModeBorder = Cast<UBorder>(ToggleModeButton->GetChildAt(0)))
    {
        ModeToggleLabelText = Cast<UTextBlock>(ModeBorder->GetContent());
    }
    TransportRow->AddChildToHorizontalBox(ToggleModeButton)->SetVerticalAlignment(VAlign_Center);
    GuitarUIStyle::AddSpacer(WidgetTree, TransportRow, 8.0f, 1.0f);

    PrevSongButton = GuitarUIStyle::MakeIconButton(WidgetTree, TransportRow, TEXT("/Game/UI/Textures/T_UI_Prev.T_UI_Prev"), FText::GetEmpty(), false, 15.0f, 12, FMargin(9.0f, 5.0f));
    GuitarUIStyle::AddSpacer(WidgetTree, TransportRow, 4.0f, 1.0f);

    PlayPauseButton = GuitarUIStyle::MakeIconButton(WidgetTree, TransportRow, TEXT("/Game/UI/Textures/T_UI_Play.T_UI_Play"), FText::FromString(TEXT("자동 연주 [P]")), true, 15.0f, 12, FMargin(11.0f, 5.0f));
    // Grab label inside PlayPauseButton's HBox
    if (UBorder* PlayBorder = Cast<UBorder>(PlayPauseButton->GetChildAt(0)))
    {
        if (UHorizontalBox* InnerHBox = Cast<UHorizontalBox>(PlayBorder->GetContent()))
        {
            if (InnerHBox->GetChildrenCount() >= 3)
            {
                PlayPauseButtonLabel = Cast<UTextBlock>(InnerHBox->GetChildAt(2));
            }
        }
    }
    GuitarUIStyle::AddSpacer(WidgetTree, TransportRow, 4.0f, 1.0f);

    NextSongButton = GuitarUIStyle::MakeIconButton(WidgetTree, TransportRow, TEXT("/Game/UI/Textures/T_UI_Next.T_UI_Next"), FText::GetEmpty(), false, 15.0f, 12, FMargin(9.0f, 5.0f));
    GuitarUIStyle::AddSpacer(WidgetTree, TransportRow, 10.0f, 1.0f);

    SongProgressBar = GuitarUIStyle::MakeProgressPill(WidgetTree, GuitarUIStyle::AccentGold);
    USizeBox* BarSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    BarSize->SetWidthOverride(170.0f);
    BarSize->SetHeightOverride(7.0f);
    BarSize->AddChild(SongProgressBar);
    TransportRow->AddChildToHorizontalBox(BarSize)->SetVerticalAlignment(VAlign_Center);

    GuitarUIStyle::AddSpacer(WidgetTree, TransportRow, 8.0f, 1.0f);
    TimeProgressText = GuitarUIStyle::MakeText(WidgetTree, FText::FromString(TEXT("00:00 / 00:00")), 11, GuitarUIStyle::TextMuted, false);
    TransportRow->AddChildToHorizontalBox(TimeProgressText)->SetVerticalAlignment(VAlign_Center);

    GuitarUIStyle::AddSpacer(WidgetTree, TransportRow, 10.0f, 1.0f);
    PlaybackRateSlider = GuitarUIStyle::MakeStyledSlider(WidgetTree, 0.5f, 1.5f, 0.05f);
    PlaybackRateSlider->SetValue(1.0f);
    USizeBox* SliderSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    SliderSize->SetWidthOverride(84.0f);
    SliderSize->AddChild(PlaybackRateSlider);
    TransportRow->AddChildToHorizontalBox(SliderSize)->SetVerticalAlignment(VAlign_Center);

    GuitarUIStyle::AddSpacer(WidgetTree, TransportRow, 5.0f, 1.0f);
    PlaybackRateValueText = GuitarUIStyle::MakeText(WidgetTree, FText::FromString(TEXT("1.00x")), 11, GuitarUIStyle::AccentGold, true);
    TransportRow->AddChildToHorizontalBox(PlaybackRateValueText)->SetVerticalAlignment(VAlign_Center);

    GuitarUIStyle::AddSpacer(WidgetTree, TransportRow, 12.0f, 1.0f);
    CursorHintText = GuitarUIStyle::MakeText(WidgetTree, FText::FromString(TEXT("[WASD] 이동  [Q] 마우스 커서  [H] UI 숨김")), 11, GuitarUIStyle::TextMuted, false);
    TransportRow->AddChildToHorizontalBox(CursorHintText)->SetVerticalAlignment(VAlign_Center);

    BottomStack->AddChild(TransportRow);

    UBorder* BottomBarCard = GuitarUIStyle::MakeGlassCard(WidgetTree, BottomStack, FMargin(16.0f, 10.0f), GuitarUIStyle::GlassPanel);
    SeatedControlBarRoot = BottomBarCard;
    if (UCanvasPanelSlot* BotSlot = RootCanvas->AddChildToCanvas(BottomBarCard))
    {
        BotSlot->SetAnchors(FAnchors(0.5f, 1.0f));
        BotSlot->SetAlignment(FVector2D(0.5f, 1.0f));
        BotSlot->SetPosition(FVector2D(0.0f, -18.0f));
        BotSlot->SetAutoSize(true);
    }
}

void UGuitarPerformanceHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    if (UGuitarScoreSubsystem* ScoreSub = World->GetSubsystem<UGuitarScoreSubsystem>())
    {
        const FGuitarPerformanceData& Perf = ScoreSub->GetLoadedPerformance();
        const EPlayControlMode Mode = ScoreSub->GetControlMode();
        const bool bAutoPlaying = ScoreSub->IsPlaying();

        if (SongTitleText)
        {
            if (Mode == EPlayControlMode::FreePlay && !bAutoPlaying)
            {
                SongTitleText->SetText(FText::FromString(TEXT("Acoustic Sanctuary — Free Play [직접 연주]")));
            }
            else
            {
                const FString DisplayTitle = Perf.Title.IsEmpty() ? TEXT("Acoustic Performance") : Perf.Title;
                SongTitleText->SetText(FText::FromString(FString::Printf(TEXT("%s (%.0f BPM)"), *DisplayTitle, Perf.Bpm)));
            }
        }

        if (PlayStyleText)
        {
            PlayStyleText->SetText(
                (Mode == EPlayControlMode::FreePlay && !bAutoPlaying)
                    ? FText::FromString(TEXT("FREEPLAY"))
                    : FText::FromString(Perf.PlayStyle.IsEmpty() ? TEXT("AUTO SONG") : Perf.PlayStyle.ToUpper()));
        }

        if (ModeToggleLabelText)
        {
            ModeToggleLabelText->SetText(
                (Mode == EPlayControlMode::FreePlay)
                    ? FText::FromString(TEXT("모드 [M]: 자유 연주"))
                    : FText::FromString(TEXT("모드 [M]: 자동 연주")));
        }

        if (PlayPauseButtonLabel)
        {
            PlayPauseButtonLabel->SetText(bAutoPlaying ? FText::FromString(TEXT("일시정지 [P]")) : FText::FromString(TEXT("자동 연주 [P]")));
        }

        const float Cur = ScoreSub->GetCurrentTimeSec();
        const float Dur = FMath::Max(0.1f, Perf.DurationSec);
        if (SongProgressBar)
        {
            SongProgressBar->SetPercent(FMath::Clamp(Cur / Dur, 0.0f, 1.0f));
        }
        if (TimeProgressText)
        {
            TimeProgressText->SetText(FText::FromString(FString::Printf(TEXT("%s / %s"), *FormatSecToClock(Cur), *FormatSecToClock(Dur))));
        }

        if (PlaybackRateValueText)
        {
            PlaybackRateValueText->SetText(FText::FromString(FString::Printf(TEXT("%.2fx"), ScoreSub->GetPlaybackRate())));
        }

        FGuitarChordSnapshot CurChord;
        if (ScoreSub->GetCurrentChord(CurChord))
        {
            if (CurrentChordText)
            {
                CurrentChordText->SetText(FText::FromString(CurChord.Symbol));
            }
            for (int32 RowIdx = 0; RowIdx < 6 && StringFretTexts.IsValidIndex(RowIdx); ++RowIdx)
            {
                // RowIdx 0 = High E (String 1), RowIdx 5 = Low E (String 6)
                const int32 Fret = CurChord.Frets.IsValidIndex(RowIdx) ? CurChord.Frets[RowIdx] : -1;
                const int32 Finger = CurChord.Fingers.IsValidIndex(RowIdx) ? CurChord.Fingers[RowIdx] : 0;
                FString CellText;
                FLinearColor CellColor = GuitarUIStyle::TextMuted;
                if (Fret < 0)
                {
                    CellText = TEXT("X");
                    CellColor = FLinearColor(0.55f, 0.34f, 0.32f, 0.9f);
                }
                else if (Fret == 0)
                {
                    CellText = TEXT("O");
                    CellColor = GuitarUIStyle::AccentTeal;
                }
                else
                {
                    CellText = (Finger > 0)
                        ? FString::Printf(TEXT("%df%d"), Fret, Finger)
                        : FString::FromInt(Fret);
                    CellColor = GuitarUIStyle::AccentGold;
                }
                if (StringFretTexts[RowIdx])
                {
                    StringFretTexts[RowIdx]->SetText(FText::FromString(CellText));
                    StringFretTexts[RowIdx]->SetColorAndOpacity(FSlateColor(CellColor));
                }
            }
        }

        FGuitarChordSnapshot NextChord;
        if (NextChordText)
        {
            if (Mode == EPlayControlMode::AutoSong && bAutoPlaying && ScoreSub->GetNextChord(NextChord))
            {
                NextChordText->SetText(FText::FromString(FString::Printf(TEXT("다음: %s"), *NextChord.Symbol)));
            }
            else
            {
                NextChordText->SetText(FText::FromString(TEXT("1~8 코드 선택")));
            }
        }

        // Update string energy bars from active ringing notes + flash transients
        TArray<FGuitarEvent> ActiveEvents;
        ScoreSub->GetActiveEvents(ActiveEvents);
        TStaticArray<bool, 6> StringActive{ InPlace, false };
        for (const FGuitarEvent& Ev : ActiveEvents)
        {
            const int32 Idx = Ev.StringIndex - 1;
            if (Idx >= 0 && Idx < 6)
            {
                StringActive[Idx] = true;
            }
        }
        for (int32 i = 0; i < 6; ++i)
        {
            const float Floor = StringActive[i] ? 0.38f : 0.0f;
            StringFlashLevels[i] = FMath::FInterpTo(StringFlashLevels[i], Floor, InDeltaTime, 7.5f);
            if (StringEnergyBars.IsValidIndex(i) && StringEnergyBars[i])
            {
                StringEnergyBars[i]->SetPercent(FMath::Clamp(StringFlashLevels[i], 0.0f, 1.0f));
            }
        }

        RefreshChordBankVisuals();
    }

    if (UGameInstance* GI = GetGameInstance())
    {
        if (UGuitarGameSettingsSubsystem* SettingsSub = GI->GetSubsystem<UGuitarGameSettingsSubsystem>())
        {
            if (MoodBadgeText)
            {
                MoodBadgeText->SetText(FText::FromString(
                    FString::Printf(TEXT("  |  %s"), *UGuitarGameSettingsSubsystem::GetMoodDisplayName(SettingsSub->GetSettings().AtmosphereMood).ToString())));
            }
        }
    }

    if (CursorHintText)
    {
        bool bCursorFree = false;
        if (const AAcousticChillCharacter* OwnerChar = Cast<AAcousticChillCharacter>(GetOwningPlayerPawn()))
        {
            bCursorFree = OwnerChar->IsFreeCursorMode();
        }
        CursorHintText->SetText(FText::FromString(
            bCursorFree
                ? TEXT("커서 활성 (클릭 연주 가능)  [Q] 카메라 회전 전환  [H] UI 숨김")
                : TEXT("[WASD] 이동  [Q] 마우스 커서  [H] UI 숨김")));
    }
}

void UGuitarPerformanceHUDWidget::HandleGuitarEventImpulse(const FGuitarEvent& Event)
{
    const int32 Idx = Event.StringIndex - 1;
    if (Idx >= 0 && Idx < 6)
    {
        StringFlashLevels[Idx] = FMath::Max(StringFlashLevels[Idx], FMath::Clamp(Event.Velocity, 0.55f, 1.0f));
    }
}

void UGuitarPerformanceHUDWidget::RefreshChordBankVisuals()
{
    UWorld* World = GetWorld();
    UGuitarScoreSubsystem* ScoreSub = World ? World->GetSubsystem<UGuitarScoreSubsystem>() : nullptr;
    if (!ScoreSub)
    {
        return;
    }

    const int32 BankIdx = ScoreSub->GetChordBankIndex();
    const TArray<FString> Slots = ScoreSub->GetLiveChordSlots();
    const int32 ActiveSlot = ScoreSub->GetActiveLiveChordSlot();

    if (ChordBankTitleText)
    {
        ChordBankTitleText->SetText(FText::FromString(
            FString::Printf(TEXT("[ ] %s"), *UGuitarSongLibrarySubsystem::GetChordBankName(BankIdx))));
    }

    for (int32 i = 0; i < ChordSlotTexts.Num() && i < 8; ++i)
    {
        const FString ChordName = Slots.IsValidIndex(i) ? Slots[i] : TEXT("-");
        const bool bIsSelected = (i == ActiveSlot) && (ScoreSub->GetControlMode() == EPlayControlMode::FreePlay || !ScoreSub->IsPlaying());
        if (ChordSlotTexts[i])
        {
            ChordSlotTexts[i]->SetText(FText::FromString(FString::Printf(TEXT("%d:%s"), i + 1, *ChordName)));
            ChordSlotTexts[i]->SetColorAndOpacity(FSlateColor(bIsSelected ? GuitarUIStyle::AccentGold : GuitarUIStyle::TextPrimary));
        }
    }
}

void UGuitarPerformanceHUDWidget::SetInteractionPrompt(bool bVisible, const FText& PromptText)
{
    bPromptCurrentlyVisible = bVisible;
    if (InteractionPromptText && bVisible)
    {
        InteractionPromptText->SetText(PromptText);
    }
    RefreshVisibilityState();
    BP_OnPromptUpdated(bVisible, PromptText);
}

void UGuitarPerformanceHUDWidget::SetSeatedMode(bool bInSeated, AGuitarPerformerActor* LinkedPerformer)
{
    SetGuitarState(bInSeated || bHoldingGuitar, bInSeated, LinkedPerformer);
}

void UGuitarPerformanceHUDWidget::SetGuitarState(bool bInHoldingGuitar, bool bInSeated, AGuitarPerformerActor* Performer)
{
    const bool bChanged = (bHoldingGuitar != bInHoldingGuitar) || (bSeated != bInSeated);
    bHoldingGuitar = bInHoldingGuitar;
    bSeated = bInSeated;
    if (Performer)
    {
        ActivePerformer = Performer;
    }

    if (GuitarStanceBadgeText)
    {
        const TCHAR* StanceStr = bSeated
            ? TEXT("  |  착석 연주 중 [E 일어나기]  |  ")
            : (bHoldingGuitar ? TEXT("  |  기타 연주 자세 [G]  |  ") : TEXT("  |  탐험 (등에 착용) [G]  |  "));
        GuitarStanceBadgeText->SetText(FText::FromString(StanceStr));
    }

    RefreshVisibilityState();
    if (bChanged)
    {
        BP_OnSeatedModeChanged(bSeated);
    }
}

void UGuitarPerformanceHUDWidget::SetCameraLabel(const FText& LabelText)
{
    CachedCameraLabel = LabelText;
    if (CameraModeText)
    {
        CameraModeText->SetText(LabelText);
    }
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

void UGuitarPerformanceHUDWidget::RefreshVisibilityState()
{
    const ESlateVisibility StdVis = bCinematicHudHidden ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible;
    const ESlateVisibility InteractiveVis = bCinematicHudHidden ? ESlateVisibility::Collapsed : ESlateVisibility::Visible;

    if (TopHeaderBadgeRoot) TopHeaderBadgeRoot->SetVisibility(InteractiveVis);
    if (SeatedControlBarRoot) SeatedControlBarRoot->SetVisibility(InteractiveVis);

    if (LivePlayDeckRoot)
    {
        const bool bShowPlayDeck = !bCinematicHudHidden && (bHoldingGuitar || bSeated);
        LivePlayDeckRoot->SetVisibility(bShowPlayDeck ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }

    bool bShowChordDiagram = true;
    if (const UGameInstance* GI = GetGameInstance())
    {
        if (const UGuitarGameSettingsSubsystem* SettingsSub = GI->GetSubsystem<UGuitarGameSettingsSubsystem>())
        {
            bShowChordDiagram = SettingsSub->GetSettings().bShowChordDiagrams;
        }
    }
    if (ChordCardRoot)
    {
        ChordCardRoot->SetVisibility((bShowChordDiagram && !bCinematicHudHidden) ? StdVis : ESlateVisibility::Collapsed);
    }
    if (InteractionPromptPill)
    {
        const bool bShowPrompt = !bCinematicHudHidden && !bSeated && bPromptCurrentlyVisible;
        InteractionPromptPill->SetVisibility(bShowPrompt ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    }
}

void UGuitarPerformanceHUDWidget::HandlePlayPauseClicked()
{
    if (UWorld* World = GetWorld())
    {
        if (UGuitarScoreSubsystem* ScoreSub = World->GetSubsystem<UGuitarScoreSubsystem>())
        {
            if (ScoreSub->IsPlaying())
                ScoreSub->Pause();
            else
                ScoreSub->Play();
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
    if (AAcousticChillCharacter* OwnerChar = Cast<AAcousticChillCharacter>(GetOwningPlayerPawn()))
    {
        const uint8 NextView = (static_cast<uint8>(OwnerChar->GetCameraViewMode()) + 1) % 6;
        OwnerChar->SetCameraViewMode(static_cast<EGuitarCharacterCameraView>(NextView));
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

void UGuitarPerformanceHUDWidget::HandleToggleHoldGuitarClicked()
{
    if (AAcousticChillCharacter* OwnerChar = Cast<AAcousticChillCharacter>(GetOwningPlayerPawn()))
    {
        OwnerChar->ToggleHoldingGuitar();
    }
}

void UGuitarPerformanceHUDWidget::HandleToggleControlModeClicked()
{
    if (UWorld* World = GetWorld())
    {
        if (UGuitarScoreSubsystem* ScoreSub = World->GetSubsystem<UGuitarScoreSubsystem>())
        {
            ScoreSub->ToggleControlMode();
        }
    }
}

void UGuitarPerformanceHUDWidget::HandleStrumDownClicked()
{
    if (UWorld* World = GetWorld())
    {
        if (UGuitarScoreSubsystem* ScoreSub = World->GetSubsystem<UGuitarScoreSubsystem>())
        {
            ScoreSub->TriggerLiveStrum(true, 0.90f);
        }
    }
}

void UGuitarPerformanceHUDWidget::HandleStrumUpClicked()
{
    if (UWorld* World = GetWorld())
    {
        if (UGuitarScoreSubsystem* ScoreSub = World->GetSubsystem<UGuitarScoreSubsystem>())
        {
            ScoreSub->TriggerLiveStrum(false, 0.72f);
        }
    }
}

void UGuitarPerformanceHUDWidget::HandlePercussiveClicked()
{
    if (UWorld* World = GetWorld())
    {
        if (UGuitarScoreSubsystem* ScoreSub = World->GetSubsystem<UGuitarScoreSubsystem>())
        {
            ScoreSub->TriggerLivePercussive(0.75f);
        }
    }
}

void UGuitarPerformanceHUDWidget::HandleBankPrevClicked()
{
    if (UWorld* World = GetWorld())
    {
        if (UGuitarScoreSubsystem* ScoreSub = World->GetSubsystem<UGuitarScoreSubsystem>())
        {
            const int32 N = UGuitarSongLibrarySubsystem::GetNumChordBanks();
            ScoreSub->SetChordBankIndex((ScoreSub->GetChordBankIndex() - 1 + N) % N);
            RefreshChordBankVisuals();
        }
    }
}

void UGuitarPerformanceHUDWidget::HandleBankNextClicked()
{
    if (UWorld* World = GetWorld())
    {
        if (UGuitarScoreSubsystem* ScoreSub = World->GetSubsystem<UGuitarScoreSubsystem>())
        {
            const int32 N = UGuitarSongLibrarySubsystem::GetNumChordBanks();
            ScoreSub->SetChordBankIndex((ScoreSub->GetChordBankIndex() + 1) % N);
            RefreshChordBankVisuals();
        }
    }
}

void UGuitarPerformanceHUDWidget::TriggerChordSlot(int32 SlotIndex)
{
    if (UWorld* World = GetWorld())
    {
        if (UGuitarScoreSubsystem* ScoreSub = World->GetSubsystem<UGuitarScoreSubsystem>())
        {
            ScoreSub->SelectLiveChordSlot(SlotIndex);
            RefreshChordBankVisuals();
        }
    }
}

void UGuitarPerformanceHUDWidget::TriggerPluckString(int32 String1To6)
{
    if (UWorld* World = GetWorld())
    {
        if (UGuitarScoreSubsystem* ScoreSub = World->GetSubsystem<UGuitarScoreSubsystem>())
        {
            ScoreSub->PluckLiveString(String1To6, 0.88f);
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
    const FString& PresetName)
{
    TArray<FString> Chords;
    CommaSeparatedChords.ParseIntoArray(Chords, TEXT(","), true);
    for (FString& C : Chords)
    {
        C.TrimStartAndEndInline();
    }

    if (UGameInstance* GI = GetGameInstance())
    {
        if (UGuitarSongLibrarySubsystem* Lib = GI->GetSubsystem<UGuitarSongLibrarySubsystem>())
        {
            Lib->GenerateAndPlayCustomChart(Chords, Bpm, bFingerstyle, PresetName, true);
        }
    }
}
