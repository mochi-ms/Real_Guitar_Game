#include "RealGuitarMenuWidget.h"
#include "GuitarScoreSubsystem.h"
#include "GuitarSongLibrarySubsystem.h"
#include "GuitarUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/CheckBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Kismet/KismetSystemLibrary.h"

namespace
{
    const float kFpsChoices[] = { 0.0f, 30.0f, 60.0f, 120.0f, 144.0f };
    constexpr int32 kNumFpsChoices = UE_ARRAY_COUNT(kFpsChoices);

    UButton* MakeMutableLabelButton(
        UWidgetTree* Tree,
        UPanelWidget* ParentBox,
        const FText& InitialText,
        TObjectPtr<UTextBlock>& OutLabel,
        bool bPrimaryAccent,
        int32 FontSize = 14,
        FMargin Padding = FMargin(16.0f, 9.0f)
    )
    {
        UButton* Btn = Tree->ConstructWidget<UButton>(UButton::StaticClass());
        FButtonStyle Style = Btn->GetStyle();
        const FLinearColor NormalTint = bPrimaryAccent ? GuitarUIStyle::AccentGoldDim : GuitarUIStyle::ButtonSecondary;
        const FLinearColor HoverTint = bPrimaryAccent ? GuitarUIStyle::AccentGold : GuitarUIStyle::ButtonHover;
        const FLinearColor PressTint = NormalTint * 0.85f;
        Style.SetNormal(GuitarUIStyle::MakeGlassBoxBrush(NormalTint));
        Style.SetHovered(GuitarUIStyle::MakeGlassBoxBrush(HoverTint));
        Style.SetPressed(GuitarUIStyle::MakeGlassBoxBrush(PressTint));
        Style.SetNormalPadding(Padding);
        Style.SetPressedPadding(Padding + FMargin(0.0f, 1.0f, 0.0f, -1.0f));
        Btn->SetStyle(Style);

        UTextBlock* Label = GuitarUIStyle::MakeText(
            Tree,
            InitialText,
            FontSize,
            bPrimaryAccent ? FLinearColor(0.99f, 0.97f, 0.93f, 1.0f) : GuitarUIStyle::TextPrimary,
            true
        );
        OutLabel = Label;
        Btn->AddChild(Label);

        if (ParentBox)
        {
            ParentBox->AddChild(Btn);
        }
        return Btn;
    }

    void AddStepperRowPtr(
        UWidgetTree* Tree,
        UPanelWidget* ParentBox,
        const FText& TitleText,
        TObjectPtr<UButton>& OutPrevBtn,
        TObjectPtr<UTextBlock>& OutValueText,
        TObjectPtr<UButton>& OutNextBtn
    )
    {
        UButton* PrevPtr = nullptr;
        UTextBlock* ValPtr = nullptr;
        UButton* NextPtr = nullptr;
        GuitarUIStyle::AddStepperRow(Tree, ParentBox, TitleText, PrevPtr, ValPtr, NextPtr);
        OutPrevBtn = PrevPtr;
        OutValueText = ValPtr;
        OutNextBtn = NextPtr;
    }

    void AddSliderRowPtr(
        UWidgetTree* Tree,
        UPanelWidget* ParentBox,
        const FText& TitleText,
        TObjectPtr<USlider>& OutSlider,
        TObjectPtr<UTextBlock>& OutValueText,
        float MinVal,
        float MaxVal,
        float StepSize
    )
    {
        USlider* SliderPtr = nullptr;
        UTextBlock* ValPtr = nullptr;
        GuitarUIStyle::AddSettingSliderRow(Tree, ParentBox, TitleText, SliderPtr, ValPtr, MinVal, MaxVal, StepSize);
        OutSlider = SliderPtr;
        OutValueText = ValPtr;
    }

    void AddCheckRowPtr(
        UWidgetTree* Tree,
        UPanelWidget* ParentBox,
        const FText& TitleText,
        TObjectPtr<UCheckBox>& OutCheckBox
    )
    {
        UCheckBox* CheckPtr = nullptr;
        GuitarUIStyle::AddCheckBoxRow(Tree, ParentBox, TitleText, CheckPtr);
        OutCheckBox = CheckPtr;
    }

    void AddKeybindRow(UWidgetTree* Tree, UPanelWidget* ParentBox, const FString& KeyText, const FString& ActionText)
    {
        UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

        UBorder* Badge = GuitarUIStyle::MakeGlassCard(
            Tree,
            GuitarUIStyle::MakeText(Tree, FText::FromString(KeyText), 11, GuitarUIStyle::AccentGold, true),
            FMargin(8.0f, 3.0f),
            FLinearColor(0.11f, 0.12f, 0.17f, 0.96f)
        );
        if (UHorizontalBoxSlot* KeySlot = Row->AddChildToHorizontalBox(Badge))
        {
            KeySlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
            KeySlot->SetVerticalAlignment(VAlign_Center);
            KeySlot->SetPadding(FMargin(0.0f, 0.0f, 10.0f, 0.0f));
        }

        UTextBlock* Desc = GuitarUIStyle::MakeText(Tree, FText::FromString(ActionText), 12, GuitarUIStyle::TextPrimary, false);
        if (UHorizontalBoxSlot* DescSlot = Row->AddChildToHorizontalBox(Desc))
        {
            DescSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            DescSlot->SetVerticalAlignment(VAlign_Center);
        }

        if (UVerticalBox* VBox = Cast<UVerticalBox>(ParentBox))
        {
            if (UVerticalBoxSlot* RowSlot = VBox->AddChildToVerticalBox(Row))
            {
                RowSlot->SetPadding(FMargin(0.0f, 2.5f));
            }
        }
        else if (ParentBox)
        {
            ParentBox->AddChild(Row);
        }
    }

    UBorder* MakeSectionCard(UWidgetTree* Tree, const FText& TitleText, UVerticalBox*& OutBodyBox)
    {
        OutBodyBox = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
        UTextBlock* Title = GuitarUIStyle::MakeText(Tree, TitleText, 13, GuitarUIStyle::AccentGold, true);
        if (UVerticalBoxSlot* TitleSlot = OutBodyBox->AddChildToVerticalBox(Title))
        {
            TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
        }
        return GuitarUIStyle::MakeGlassCard(Tree, OutBodyBox, FMargin(14.0f, 10.0f), GuitarUIStyle::CardSurface);
    }
}

void URealGuitarMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();

    ResolutionChoices = UGuitarGameSettingsSubsystem::GetSupportedResolutions();

    if (!RootBackdrop && WidgetTree)
    {
        BuildMenuLayout();
    }

    SetVisibility(ESlateVisibility::Collapsed);
    bMenuOpen = false;
}

UScrollBox* URealGuitarMenuWidget::CreateTabScrollContainer()
{
    UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
    Scroll->SetAnimateWheelScrolling(true);
    return Scroll;
}

void URealGuitarMenuWidget::BuildMenuLayout()
{
    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
    WidgetTree->RootWidget = Canvas;

    RootBackdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    RootBackdrop->SetBrushColor(GuitarUIStyle::TitleScrim);
    if (UCanvasPanelSlot* BgSlot = Canvas->AddChildToCanvas(RootBackdrop))
    {
        BgSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
        BgSlot->SetOffsets(FMargin(0.0f));
    }

    UCanvasPanel* BackdropCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
    RootBackdrop->SetContent(BackdropCanvas);

    UVerticalBox* PanelRoot = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    MainPanelBorder = GuitarUIStyle::MakeGlassCard(WidgetTree, PanelRoot, FMargin(22.0f, 18.0f), GuitarUIStyle::GlassPanel);
    MainPanelSlot = BackdropCanvas->AddChildToCanvas(MainPanelBorder);
    if (MainPanelSlot)
    {
        MainPanelSlot->SetAnchors(FAnchors(0.03f, 0.05f, 0.45f, 0.95f));
        MainPanelSlot->SetOffsets(FMargin(0.0f));
    }

    UHorizontalBox* HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    PanelRoot->AddChildToVerticalBox(HeaderRow);

    UVerticalBox* BrandCol = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    HeroTitleText = GuitarUIStyle::MakeText(WidgetTree, FText::FromString(TEXT("REAL GUITAR SANCTUARY")), 24, GuitarUIStyle::TextPrimary, true);
    HeroSubtitleText = GuitarUIStyle::MakeText(
        WidgetTree,
        FText::FromString(TEXT("호숫가 정자 · 어쿠스틱 라이브 시뮬레이션")),
        12,
        GuitarUIStyle::AccentGold,
        false
    );
    BrandCol->AddChildToVerticalBox(HeroTitleText);
    BrandCol->AddChildToVerticalBox(HeroSubtitleText);
    if (UHorizontalBoxSlot* BrandSlot = HeaderRow->AddChildToHorizontalBox(BrandCol))
    {
        BrandSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        BrandSlot->SetVerticalAlignment(VAlign_Center);
    }

    UHorizontalBox* TopActions = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    if (UHorizontalBoxSlot* ActionsSlot = HeaderRow->AddChildToHorizontalBox(TopActions))
    {
        ActionsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        ActionsSlot->SetVerticalAlignment(VAlign_Center);
    }

    PrimaryActionButton = MakeMutableLabelButton(
        WidgetTree,
        TopActions,
        FText::FromString(TEXT("산책 & 연주 시작")),
        PrimaryActionLabel,
        true,
        14,
        FMargin(16.0f, 8.0f)
    );
    PrimaryActionButton->OnClicked.AddDynamic(this, &URealGuitarMenuWidget::HandleResumeOrStartClicked);
    GuitarUIStyle::AddSpacer(WidgetTree, TopActions, 6.0f, 1.0f);

    BackToTitleButton = GuitarUIStyle::MakeButton(
        WidgetTree,
        TopActions,
        FText::FromString(TEXT("타이틀")),
        false,
        12,
        FMargin(11.0f, 8.0f)
    );
    BackToTitleButton->OnClicked.AddDynamic(this, &URealGuitarMenuWidget::HandleBackToTitleClicked);
    GuitarUIStyle::AddSpacer(WidgetTree, TopActions, 6.0f, 1.0f);

    QuitButton = GuitarUIStyle::MakeButton(
        WidgetTree,
        TopActions,
        FText::FromString(TEXT("종료")),
        false,
        12,
        FMargin(11.0f, 8.0f)
    );
    QuitButton->OnClicked.AddDynamic(this, &URealGuitarMenuWidget::HandleQuitClicked);

    GuitarUIStyle::AddSpacer(WidgetTree, PanelRoot, 1.0f, 10.0f);

    UHorizontalBox* TabBar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    PanelRoot->AddChildToVerticalBox(TabBar);

    auto AddTabBtn = [&](const TCHAR* IconPath, const FString& Label) -> UButton*
    {
        UButton* Btn = GuitarUIStyle::MakeIconButton(
            WidgetTree,
            TabBar,
            IconPath,
            FText::FromString(Label),
            false,
            15.0f,
            13,
            FMargin(12.0f, 7.0f)
        );
        if (UHorizontalBoxSlot* TabSlot = Cast<UHorizontalBoxSlot>(Btn->Slot))
        {
            TabSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            TabSlot->SetPadding(FMargin(0.0f, 0.0f, 6.0f, 0.0f));
        }
        TabButtons.Add(Btn);
        return Btn;
    };

    AddTabBtn(TEXT("/Game/UI/Textures/T_UI_Play"), TEXT("연주 & 곡"))->OnClicked.AddDynamic(this, &URealGuitarMenuWidget::HandleTabMainClicked);
    AddTabBtn(TEXT("/Game/UI/Textures/T_UI_Settings"), TEXT("그래픽"))->OnClicked.AddDynamic(this, &URealGuitarMenuWidget::HandleTabGraphicsClicked);
    AddTabBtn(TEXT("/Game/UI/Textures/T_UI_MoodSunset"), TEXT("오디오 & 분위기"))->OnClicked.AddDynamic(this, &URealGuitarMenuWidget::HandleTabAudioClicked);
    AddTabBtn(TEXT("/Game/UI/Textures/T_UI_Camera"), TEXT("카메라 & 조작"))->OnClicked.AddDynamic(this, &URealGuitarMenuWidget::HandleTabControlsClicked);

    GuitarUIStyle::AddSpacer(WidgetTree, PanelRoot, 1.0f, 10.0f);

    ContentSwitcher = WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass());
    if (UVerticalBoxSlot* SwitchSlot = PanelRoot->AddChildToVerticalBox(ContentSwitcher))
    {
        SwitchSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }

    // ==================== TAB 0: Main, Performer, Live Mode & Songs ====================
    UScrollBox* Tab0Scroll = CreateTabScrollContainer();
    UVerticalBox* Tab0 = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    Tab0Scroll->AddChild(Tab0);
    ContentSwitcher->AddChild(Tab0Scroll);

    UVerticalBox* CardPerf = nullptr;
    Tab0->AddChildToVerticalBox(MakeSectionCard(WidgetTree, FText::FromString(TEXT("연주자 · 실시간 모드")), CardPerf));

    AddStepperRowPtr(
        WidgetTree,
        CardPerf,
        FText::FromString(TEXT("캐릭터 외형 (실시간 반영)")),
        CharacterPrevButton,
        CharacterValueText,
        CharacterNextButton
    );
    CharacterPrevButton->OnClicked.AddDynamic(this, &URealGuitarMenuWidget::HandleCharacterPrevClicked);
    CharacterNextButton->OnClicked.AddDynamic(this, &URealGuitarMenuWidget::HandleCharacterNextClicked);

    UHorizontalBox* ModeRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    CardPerf->AddChildToVerticalBox(ModeRow);
    UTextBlock* ModeLabel = GuitarUIStyle::MakeText(WidgetTree, FText::FromString(TEXT("연주 방식 [M]")), 13, GuitarUIStyle::TextMuted);
    if (UHorizontalBoxSlot* ModeLblSlot = ModeRow->AddChildToHorizontalBox(ModeLabel))
    {
        ModeLblSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        ModeLblSlot->SetVerticalAlignment(VAlign_Center);
    }
    PlayModeValueText = GuitarUIStyle::MakeText(WidgetTree, FText::FromString(TEXT("직접 연주 (Free Play)")), 13, GuitarUIStyle::AccentTeal, true);
    if (UHorizontalBoxSlot* ModeValSlot = ModeRow->AddChildToHorizontalBox(PlayModeValueText))
    {
        ModeValSlot->SetVerticalAlignment(VAlign_Center);
        ModeValSlot->SetPadding(FMargin(8.0f, 0.0f));
    }
    TogglePlayModeButton = GuitarUIStyle::MakeButton(WidgetTree, ModeRow, FText::FromString(TEXT("모드 전환")), true, 12, FMargin(11.0f, 5.0f));
    TogglePlayModeButton->OnClicked.AddDynamic(this, &URealGuitarMenuWidget::HandleTogglePlayModeClicked);

    AddStepperRowPtr(
        WidgetTree,
        CardPerf,
        FText::FromString(TEXT("코드 뱅크 [ [ / ] ]")),
        ChordBankPrevButton,
        ChordBankValueText,
        ChordBankNextButton
    );
    ChordBankPrevButton->OnClicked.AddDynamic(this, &URealGuitarMenuWidget::HandleChordBankPrevClicked);
    ChordBankNextButton->OnClicked.AddDynamic(this, &URealGuitarMenuWidget::HandleChordBankNextClicked);

    GuitarUIStyle::AddSpacer(WidgetTree, Tab0, 1.0f, 8.0f);

    UVerticalBox* CardSongs = nullptr;
    Tab0->AddChildToVerticalBox(MakeSectionCard(WidgetTree, FText::FromString(TEXT("곡 재생 & 즉흥 프리셋")), CardSongs));

    SelectedSongText = GuitarUIStyle::MakeText(WidgetTree, FText::FromString(TEXT("악보를 불러오는 중...")), 14, GuitarUIStyle::TextPrimary, true);
    CardSongs->AddChildToVerticalBox(SelectedSongText);
    GuitarUIStyle::AddSpacer(WidgetTree, CardSongs, 1.0f, 6.0f);

    UHorizontalBox* SongControls = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    CardSongs->AddChildToVerticalBox(SongControls);
    SongPrevButton = GuitarUIStyle::MakeIconButton(WidgetTree, SongControls, TEXT("/Game/UI/Textures/T_UI_Prev"), FText::FromString(TEXT("이전 곡")), false, 15.0f, 12);
    GuitarUIStyle::AddSpacer(WidgetTree, SongControls, 6.0f, 1.0f);
    PlayPauseButton = MakeMutableLabelButton(WidgetTree, SongControls, FText::FromString(TEXT("재생 / 일시정지")), PlayPauseLabelText, true, 13, FMargin(14.0f, 6.0f));
    GuitarUIStyle::AddSpacer(WidgetTree, SongControls, 6.0f, 1.0f);
    SongNextButton = GuitarUIStyle::MakeIconButton(WidgetTree, SongControls, TEXT("/Game/UI/Textures/T_UI_Next"), FText::FromString(TEXT("다음 곡")), false, 15.0f, 12);
    GuitarUIStyle::AddSpacer(WidgetTree, SongControls, 10.0f, 1.0f);

    LoopCheckBox = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass());
    LoopCheckBox->SetIsChecked(true);
    SongControls->AddChildToHorizontalBox(LoopCheckBox);
    if (UHorizontalBoxSlot* LoopSlot = SongControls->AddChildToHorizontalBox(
            GuitarUIStyle::MakeText(WidgetTree, FText::FromString(TEXT("반복 재생")), 12, GuitarUIStyle::TextMuted)))
    {
        LoopSlot->SetVerticalAlignment(VAlign_Center);
        LoopSlot->SetPadding(FMargin(6.0f, 0.0f, 0.0f, 0.0f));
    }

    SongPrevButton->OnClicked.AddDynamic(this, &URealGuitarMenuWidget::HandleSongPrevClicked);
    PlayPauseButton->OnClicked.AddDynamic(this, &URealGuitarMenuWidget::HandlePlayPauseClicked);
    SongNextButton->OnClicked.AddDynamic(this, &URealGuitarMenuWidget::HandleSongNextClicked);
    LoopCheckBox->OnCheckStateChanged.AddDynamic(this, &URealGuitarMenuWidget::HandleLoopToggled);

    GuitarUIStyle::AddSpacer(WidgetTree, CardSongs, 1.0f, 8.0f);
    CardSongs->AddChildToVerticalBox(
        GuitarUIStyle::MakeText(WidgetTree, FText::FromString(TEXT("즉시 잼 반주 생성 (클릭 시 자동 연주)")), 12, GuitarUIStyle::TextMuted));
    GuitarUIStyle::AddSpacer(WidgetTree, CardSongs, 1.0f, 4.0f);

    UHorizontalBox* JamRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    CardSongs->AddChildToVerticalBox(JamRow);
    JamBalladButton = GuitarUIStyle::MakeButton(WidgetTree, JamRow, FText::FromString(TEXT("어쿠스틱 발라드 (C-G-Am-F)")), false, 11, FMargin(9.0f, 6.0f));
    GuitarUIStyle::AddSpacer(WidgetTree, JamRow, 5.0f, 1.0f);
    JamFingerstyleButton = GuitarUIStyle::MakeButton(WidgetTree, JamRow, FText::FromString(TEXT("포크 핑거스타일 (Em-C-G-D)")), false, 11, FMargin(9.0f, 6.0f));
    GuitarUIStyle::AddSpacer(WidgetTree, JamRow, 5.0f, 1.0f);
    JamPopButton = GuitarUIStyle::MakeButton(WidgetTree, JamRow, FText::FromString(TEXT("선셋 칠 (Fmaj7-G-Em7-Am7)")), false, 11, FMargin(9.0f, 6.0f));

    JamBalladButton->OnClicked.AddDynamic(this, &URealGuitarMenuWidget::HandleJamBalladClicked);
    JamFingerstyleButton->OnClicked.AddDynamic(this, &URealGuitarMenuWidget::HandleJamFingerstyleClicked);
    JamPopButton->OnClicked.AddDynamic(this, &URealGuitarMenuWidget::HandleJamPopGrooveClicked);

    // ==================== TAB 1: Graphics & Display ====================
    UScrollBox* Tab1Scroll = CreateTabScrollContainer();
    UVerticalBox* Tab1 = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    Tab1Scroll->AddChild(Tab1);
    ContentSwitcher->AddChild(Tab1Scroll);

    UVerticalBox* CardGraphics = nullptr;
    Tab1->AddChildToVerticalBox(MakeSectionCard(WidgetTree, FText::FromString(TEXT("화면 & 그래픽 품질")), CardGraphics));

    AddStepperRowPtr(WidgetTree, CardGraphics, FText::FromString(TEXT("해상도")), ResPrevButton, ResValueText, ResNextButton);
    AddStepperRowPtr(WidgetTree, CardGraphics, FText::FromString(TEXT("화면 모드")), WinModePrevButton, WinModeValueText, WinModeNextButton);
    AddStepperRowPtr(WidgetTree, CardGraphics, FText::FromString(TEXT("프레임 제한")), FpsPrevButton, FpsValueText, FpsNextButton);
    AddStepperRowPtr(WidgetTree, CardGraphics, FText::FromString(TEXT("그래픽 프리셋")), QualityPrevButton, QualityValueText, QualityNextButton);
    AddCheckRowPtr(WidgetTree, CardGraphics, FText::FromString(TEXT("수직 동기화 (VSync)")), VSyncCheckBox);
    AddCheckRowPtr(WidgetTree, CardGraphics, FText::FromString(TEXT("Lumen 실시간 전역 조명")), LumenCheckBox);

    ResPrevButton->OnClicked.AddDynamic(this, &URealGuitarMenuWidget::HandleResPrevClicked);
    ResNextButton->OnClicked.AddDynamic(this, &URealGuitarMenuWidget::HandleResNextClicked);
    WinModePrevButton->OnClicked.AddDynamic(this, &URealGuitarMenuWidget::HandleWinModePrevClicked);
    WinModeNextButton->OnClicked.AddDynamic(this, &URealGuitarMenuWidget::HandleWinModeNextClicked);
    FpsPrevButton->OnClicked.AddDynamic(this, &URealGuitarMenuWidget::HandleFpsPrevClicked);
    FpsNextButton->OnClicked.AddDynamic(this, &URealGuitarMenuWidget::HandleFpsNextClicked);
    QualityPrevButton->OnClicked.AddDynamic(this, &URealGuitarMenuWidget::HandleQualityPrevClicked);
    QualityNextButton->OnClicked.AddDynamic(this, &URealGuitarMenuWidget::HandleQualityNextClicked);
    VSyncCheckBox->OnCheckStateChanged.AddDynamic(this, &URealGuitarMenuWidget::HandleVSyncToggled);
    LumenCheckBox->OnCheckStateChanged.AddDynamic(this, &URealGuitarMenuWidget::HandleLumenToggled);

    GuitarUIStyle::AddSpacer(WidgetTree, CardGraphics, 1.0f, 8.0f);
    UHorizontalBox* GfxActionRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    CardGraphics->AddChildToVerticalBox(GfxActionRow);
    ApplyGraphicsButton = GuitarUIStyle::MakeButton(WidgetTree, GfxActionRow, FText::FromString(TEXT("설정 적용 & 저장")), true, 13);
    GuitarUIStyle::AddSpacer(WidgetTree, GfxActionRow, 8.0f, 1.0f);
    ResetDefaultsButton = GuitarUIStyle::MakeButton(WidgetTree, GfxActionRow, FText::FromString(TEXT("기본값 복원")), false, 13);
    ApplyGraphicsButton->OnClicked.AddDynamic(this, &URealGuitarMenuWidget::HandleApplyGraphicsClicked);
    ResetDefaultsButton->OnClicked.AddDynamic(this, &URealGuitarMenuWidget::HandleResetDefaultsClicked);

    GraphicsStatusText = GuitarUIStyle::MakeText(WidgetTree, FText::GetEmpty(), 12, GuitarUIStyle::AccentTeal);
    CardGraphics->AddChildToVerticalBox(GraphicsStatusText);

    // ==================== TAB 2: Audio & Sanctuary Mood ====================
    UScrollBox* Tab2Scroll = CreateTabScrollContainer();
    UVerticalBox* Tab2 = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    Tab2Scroll->AddChild(Tab2);
    ContentSwitcher->AddChild(Tab2Scroll);

    UVerticalBox* CardAudio = nullptr;
    Tab2->AddChildToVerticalBox(MakeSectionCard(WidgetTree, FText::FromString(TEXT("사운드 믹서")), CardAudio));
    AddSliderRowPtr(WidgetTree, CardAudio, FText::FromString(TEXT("마스터 볼륨")), MasterVolSlider, MasterVolValueText, 0.0f, 1.0f, 0.01f);
    AddSliderRowPtr(WidgetTree, CardAudio, FText::FromString(TEXT("어쿠스틱 기타 볼륨")), GuitarVolSlider, GuitarVolValueText, 0.0f, 1.0f, 0.01f);
    AddSliderRowPtr(WidgetTree, CardAudio, FText::FromString(TEXT("자연 호수 · 모닥불 소리")), AmbientVolSlider, AmbientVolValueText, 0.0f, 1.0f, 0.01f);
    AddSliderRowPtr(WidgetTree, CardAudio, FText::FromString(TEXT("홀 리버브 공간감")), ReverbSlider, ReverbValueText, 0.0f, 1.0f, 0.01f);

    MasterVolSlider->OnValueChanged.AddDynamic(this, &URealGuitarMenuWidget::HandleMasterVolChanged);
    GuitarVolSlider->OnValueChanged.AddDynamic(this, &URealGuitarMenuWidget::HandleGuitarVolChanged);
    AmbientVolSlider->OnValueChanged.AddDynamic(this, &URealGuitarMenuWidget::HandleAmbientVolChanged);
    ReverbSlider->OnValueChanged.AddDynamic(this, &URealGuitarMenuWidget::HandleReverbChanged);

    GuitarUIStyle::AddSpacer(WidgetTree, Tab2, 1.0f, 8.0f);

    UVerticalBox* CardMood = nullptr;
    Tab2->AddChildToVerticalBox(MakeSectionCard(WidgetTree, FText::FromString(TEXT("시간대 분위기 [T] & HUD")), CardMood));
    CurrentMoodLabel = GuitarUIStyle::MakeText(WidgetTree, FText::FromString(TEXT("현재 분위기: 황금빛 노을")), 13, GuitarUIStyle::TextPrimary, true);
    CardMood->AddChildToVerticalBox(CurrentMoodLabel);
    GuitarUIStyle::AddSpacer(WidgetTree, CardMood, 1.0f, 6.0f);

    UHorizontalBox* MoodRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    CardMood->AddChildToVerticalBox(MoodRow);
    MoodSunsetButton = GuitarUIStyle::MakeIconButton(WidgetTree, MoodRow, TEXT("/Game/UI/Textures/T_UI_MoodSunset"), FText::FromString(TEXT("노을")), false, 15.0f, 12);
    GuitarUIStyle::AddSpacer(WidgetTree, MoodRow, 5.0f, 1.0f);
    MoodNightButton = GuitarUIStyle::MakeIconButton(WidgetTree, MoodRow, TEXT("/Game/UI/Textures/T_UI_MoodNight"), FText::FromString(TEXT("모닥불 밤")), false, 15.0f, 12);
    GuitarUIStyle::AddSpacer(WidgetTree, MoodRow, 5.0f, 1.0f);
    MoodMorningButton = GuitarUIStyle::MakeButton(WidgetTree, MoodRow, FText::FromString(TEXT("아침 안개")), false, 12, FMargin(10.0f, 6.0f));
    GuitarUIStyle::AddSpacer(WidgetTree, MoodRow, 5.0f, 1.0f);
    MoodRainButton = GuitarUIStyle::MakeButton(WidgetTree, MoodRow, FText::FromString(TEXT("비 오는 로파이")), false, 12, FMargin(10.0f, 6.0f));

    MoodSunsetButton->OnClicked.AddDynamic(this, &URealGuitarMenuWidget::HandleMoodSunsetClicked);
    MoodNightButton->OnClicked.AddDynamic(this, &URealGuitarMenuWidget::HandleMoodNightClicked);
    MoodMorningButton->OnClicked.AddDynamic(this, &URealGuitarMenuWidget::HandleMoodMorningClicked);
    MoodRainButton->OnClicked.AddDynamic(this, &URealGuitarMenuWidget::HandleMoodRainClicked);

    AddCheckRowPtr(WidgetTree, CardMood, FText::FromString(TEXT("실시간 코드 · 지판 표시")), ShowChordsCheckBox);
    AddCheckRowPtr(WidgetTree, CardMood, FText::FromString(TEXT("착석 시 비연주 HUD 최소화")), AutoHideHudCheckBox);
    ShowChordsCheckBox->OnCheckStateChanged.AddDynamic(this, &URealGuitarMenuWidget::HandleShowChordsToggled);
    AutoHideHudCheckBox->OnCheckStateChanged.AddDynamic(this, &URealGuitarMenuWidget::HandleAutoHideHudToggled);

    // ==================== TAB 3: Camera & Complete Keybinds ====================
    UScrollBox* Tab3Scroll = CreateTabScrollContainer();
    UVerticalBox* Tab3 = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    Tab3Scroll->AddChild(Tab3);
    ContentSwitcher->AddChild(Tab3Scroll);

    UVerticalBox* CardCam = nullptr;
    Tab3->AddChildToVerticalBox(MakeSectionCard(WidgetTree, FText::FromString(TEXT("카메라 & 시점 감도")), CardCam));
    AddSliderRowPtr(WidgetTree, CardCam, FText::FromString(TEXT("3인칭 카메라 거리 (cm)")), CamDistSlider, CamDistValueText, 140.0f, 460.0f, 5.0f);
    AddSliderRowPtr(WidgetTree, CardCam, FText::FromString(TEXT("마우스 회전 감도")), MouseSensSlider, MouseSensValueText, 0.2f, 3.0f, 0.05f);
    AddSliderRowPtr(WidgetTree, CardCam, FText::FromString(TEXT("시야각 (FOV)")), FovSlider, FovValueText, 65.0f, 110.0f, 1.0f);
    AddCheckRowPtr(WidgetTree, CardCam, FText::FromString(TEXT("마우스 Y축 반전")), InvertYCheckBox);

    CamDistSlider->OnValueChanged.AddDynamic(this, &URealGuitarMenuWidget::HandleCamDistChanged);
    MouseSensSlider->OnValueChanged.AddDynamic(this, &URealGuitarMenuWidget::HandleMouseSensChanged);
    FovSlider->OnValueChanged.AddDynamic(this, &URealGuitarMenuWidget::HandleFovChanged);
    InvertYCheckBox->OnCheckStateChanged.AddDynamic(this, &URealGuitarMenuWidget::HandleInvertYToggled);

    GuitarUIStyle::AddSpacer(WidgetTree, Tab3, 1.0f, 8.0f);

    UVerticalBox* CardKeys = nullptr;
    Tab3->AddChildToVerticalBox(MakeSectionCard(WidgetTree, FText::FromString(TEXT("탐험 & 수동 기타 연주 조작 가이드")), CardKeys));
    AddKeybindRow(WidgetTree, CardKeys, TEXT("W A S D"), TEXT("호숫가 산책 · 정자 이동"));
    AddKeybindRow(WidgetTree, CardKeys, TEXT("G"), TEXT("기타 들기 (연주 자세) / 등에 메기"));
    AddKeybindRow(WidgetTree, CardKeys, TEXT("E"), TEXT("정자 벤치 · 부두 스툴 · 모닥불 의자 착석 / 기립"));
    AddKeybindRow(WidgetTree, CardKeys, TEXT("1 ~ 8"), TEXT("코드 슬롯 선택 & 지판 운지 전환"));
    AddKeybindRow(WidgetTree, CardKeys, TEXT("[ / ]"), TEXT("코드 뱅크 변경 (Sunset / Campfire / Lo-Fi)"));
    AddKeybindRow(WidgetTree, CardKeys, TEXT("Space / 좌클릭"), TEXT("다운 스트럼 (6현 -> 1현 아르페지오)"));
    AddKeybindRow(WidgetTree, CardKeys, TEXT("우클릭"), TEXT("업 스트럼 (1현 -> 6현 스트럼)"));
    AddKeybindRow(WidgetTree, CardKeys, TEXT("U I O J K L"), TEXT("6번줄(E) ~ 1번줄(e) 개별 줄 튕기기"));
    AddKeybindRow(WidgetTree, CardKeys, TEXT("X / 휠클릭"), TEXT("어쿠스틱 퍼커시브 커팅 (Chuck)"));
    AddKeybindRow(WidgetTree, CardKeys, TEXT("M / P / N B"), TEXT("직접·자동 전환 / 재생·정지 / 다음·이전 곡"));
    AddKeybindRow(WidgetTree, CardKeys, TEXT("C / Q / T / H"), TEXT("6종 카메라 전환 / 커서 토글 / 분위기 / HUD"));
    AddKeybindRow(WidgetTree, CardKeys, TEXT("Esc / Tab"), TEXT("이 설정 및 곡 라이브러리 메뉴 열기/닫기"));

    SelectTab(0);
}

void URealGuitarMenuWidget::OpenMenu(ERealGuitarMenuMode InMode, int32 InitialTabIndex)
{
    CurrentMode = InMode;
    bMenuOpen = true;

    if (const UGuitarGameSettingsSubsystem* SettingsSub = GetSettingsSubsystem())
    {
        DraftSettings = SettingsSub->GetSettings();
    }

    RefreshModeChrome();
    SyncWidgetsFromSettings();
    SelectTab(InitialTabIndex);
    SetVisibility(ESlateVisibility::Visible);
}

void URealGuitarMenuWidget::CloseMenu()
{
    if (!bMenuOpen)
    {
        return;
    }

    bMenuOpen = false;
    SetVisibility(ESlateVisibility::Collapsed);
    OnMenuClosed.Broadcast();
}

void URealGuitarMenuWidget::SelectTab(int32 TabIndex)
{
    const int32 ClampedIndex = FMath::Clamp(TabIndex, 0, TabButtons.Num() - 1);
    if (ContentSwitcher)
    {
        ContentSwitcher->SetActiveWidgetIndex(ClampedIndex);
    }

    for (int32 i = 0; i < TabButtons.Num(); ++i)
    {
        if (!TabButtons[i])
        {
            continue;
        }
        const bool bActive = (i == ClampedIndex);
        FButtonStyle Style = TabButtons[i]->GetStyle();
        Style.SetNormal(GuitarUIStyle::MakeGlassBoxBrush(bActive ? GuitarUIStyle::AccentGoldDim : GuitarUIStyle::ButtonSecondary));
        Style.SetHovered(GuitarUIStyle::MakeGlassBoxBrush(bActive ? GuitarUIStyle::AccentGold : GuitarUIStyle::ButtonHover));
        TabButtons[i]->SetStyle(Style);
    }
}

void URealGuitarMenuWidget::RefreshModeChrome()
{
    const bool bTitle = (CurrentMode == ERealGuitarMenuMode::TitleScreen);
    if (RootBackdrop)
    {
        RootBackdrop->SetBrushColor(bTitle ? GuitarUIStyle::TitleScrim : GuitarUIStyle::BackdropScrim);
    }
    if (MainPanelSlot)
    {
        if (bTitle)
        {
            MainPanelSlot->SetAnchors(FAnchors(0.03f, 0.05f, 0.45f, 0.95f));
        }
        else
        {
            MainPanelSlot->SetAnchors(FAnchors(0.16f, 0.07f, 0.84f, 0.93f));
        }
        MainPanelSlot->SetOffsets(FMargin(0.0f));
    }
    if (HeroTitleText)
    {
        HeroTitleText->SetText(FText::FromString(bTitle ? TEXT("REAL GUITAR SANCTUARY") : TEXT("일시 정지 · 환경 설정")));
    }
    if (HeroSubtitleText)
    {
        HeroSubtitleText->SetText(FText::FromString(
            bTitle
                ? TEXT("호숫가 정자와 숲속 모닥불에서 내 손으로 직접 어쿠스틱 기타를 연주하세요")
                : TEXT("연주자 · 그래픽 · 오디오 · 조작 설정 변경 시 즉시 반영됩니다")
        ));
    }
    if (PrimaryActionLabel)
    {
        PrimaryActionLabel->SetText(FText::FromString(bTitle ? TEXT("산책 & 연주 시작") : TEXT("계속 연주하기")));
    }
    if (BackToTitleButton)
    {
        BackToTitleButton->SetVisibility(bTitle ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    }
}

void URealGuitarMenuWidget::SyncWidgetsFromSettings()
{
    bSyncingUI = true;

    ResolutionIndex = FindClosestResolutionIndex(DraftSettings.Resolution);
    FpsChoiceIndex = 2;
    for (int32 i = 0; i < kNumFpsChoices; ++i)
    {
        if (FMath::IsNearlyEqual(DraftSettings.FrameRateLimit, kFpsChoices[i], 1.0f))
        {
            FpsChoiceIndex = i;
            break;
        }
    }

    if (ResValueText && ResolutionChoices.IsValidIndex(ResolutionIndex))
    {
        const FIntPoint& Res = ResolutionChoices[ResolutionIndex];
        ResValueText->SetText(FText::FromString(FString::Printf(TEXT("%d x %d"), Res.X, Res.Y)));
    }
    if (WinModeValueText)
    {
        const TCHAR* ModeNames[] = { TEXT("전체 화면"), TEXT("테두리 없는 창모드"), TEXT("창모드") };
        WinModeValueText->SetText(FText::FromString(ModeNames[FMath::Clamp(DraftSettings.WindowMode, 0, 2)]));
    }
    if (FpsValueText)
    {
        FpsValueText->SetText(
            kFpsChoices[FpsChoiceIndex] <= 0.0f
                ? FText::FromString(TEXT("무제한"))
                : FText::FromString(FString::Printf(TEXT("%d FPS"), FMath::RoundToInt(kFpsChoices[FpsChoiceIndex])))
        );
    }
    if (QualityValueText)
    {
        QualityValueText->SetText(UGuitarGameSettingsSubsystem::GetGraphicsPresetName(DraftSettings.GraphicsPreset));
    }
    if (VSyncCheckBox) VSyncCheckBox->SetIsChecked(DraftSettings.bVSync);
    if (LumenCheckBox) LumenCheckBox->SetIsChecked(DraftSettings.bLumenEnabled);

    if (CharacterValueText)
    {
        CharacterValueText->SetText(FText::FromString(
            DraftSettings.CharacterModelIndex == 1 ? TEXT("Manny (남성형 마네킹)") : TEXT("Quinn (여성형 마네킹)")));
    }
    if (ChordBankValueText)
    {
        ChordBankValueText->SetText(FText::FromString(
            UGuitarSongLibrarySubsystem::GetChordBankName(DraftSettings.ChordBankIndex)));
    }

    auto SyncSlider = [](USlider* Slider, UTextBlock* Label, float Val, const FString& Formatted)
    {
        if (Slider) Slider->SetValue(Val);
        if (Label) Label->SetText(FText::FromString(Formatted));
    };

    SyncSlider(MasterVolSlider, MasterVolValueText, DraftSettings.MasterVolume, FString::Printf(TEXT("%d%%"), FMath::RoundToInt(DraftSettings.MasterVolume * 100.0f)));
    SyncSlider(GuitarVolSlider, GuitarVolValueText, DraftSettings.GuitarVolume, FString::Printf(TEXT("%d%%"), FMath::RoundToInt(DraftSettings.GuitarVolume * 100.0f)));
    SyncSlider(AmbientVolSlider, AmbientVolValueText, DraftSettings.AmbientVolume, FString::Printf(TEXT("%d%%"), FMath::RoundToInt(DraftSettings.AmbientVolume * 100.0f)));
    SyncSlider(ReverbSlider, ReverbValueText, DraftSettings.ReverbMix, FString::Printf(TEXT("%d%%"), FMath::RoundToInt(DraftSettings.ReverbMix * 100.0f)));

    if (CurrentMoodLabel)
    {
        CurrentMoodLabel->SetText(FText::FromString(
            FString::Printf(TEXT("현재 분위기: %s"), *UGuitarGameSettingsSubsystem::GetMoodDisplayName(DraftSettings.AtmosphereMood).ToString())
        ));
    }
    if (ShowChordsCheckBox) ShowChordsCheckBox->SetIsChecked(DraftSettings.bShowChordDiagrams);
    if (AutoHideHudCheckBox) AutoHideHudCheckBox->SetIsChecked(DraftSettings.bAutoHideHudWhenSeated);

    SyncSlider(CamDistSlider, CamDistValueText, DraftSettings.CameraDistance, FString::Printf(TEXT("%d cm"), FMath::RoundToInt(DraftSettings.CameraDistance)));
    SyncSlider(MouseSensSlider, MouseSensValueText, DraftSettings.MouseSensitivity, FString::Printf(TEXT("%.2fx"), DraftSettings.MouseSensitivity));
    SyncSlider(FovSlider, FovValueText, DraftSettings.FieldOfView, FString::Printf(TEXT("%d°"), FMath::RoundToInt(DraftSettings.FieldOfView)));
    if (InvertYCheckBox) InvertYCheckBox->SetIsChecked(DraftSettings.bInvertY);

    SyncSongAndPlayLabels();
    bSyncingUI = false;
}

void URealGuitarMenuWidget::SyncSongAndPlayLabels()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    if (UGuitarScoreSubsystem* Score = World->GetSubsystem<UGuitarScoreSubsystem>())
    {
        const bool bLiveMode = (Score->GetControlMode() == EPlayControlMode::FreePlay);
        if (PlayModeValueText)
        {
            PlayModeValueText->SetText(FText::FromString(
                bLiveMode ? TEXT("직접 연주 (Free Play)") : TEXT("자동 곡 감상 (Auto Song)")));
        }
        if (PlayPauseLabelText)
        {
            PlayPauseLabelText->SetText(FText::FromString(Score->IsPlaying() ? TEXT("일시 정지") : TEXT("곡 재생")));
        }
        if (LoopCheckBox)
        {
            LoopCheckBox->SetIsChecked(Score->IsLooping());
        }
        if (SelectedSongText)
        {
            SelectedSongText->SetText(FText::FromString(Score->GetLoadedPerformance().Title));
        }
    }
}

void URealGuitarMenuWidget::PushDraftSettings(bool bPersistToDisk)
{
    if (UGuitarGameSettingsSubsystem* SettingsSub = GetSettingsSubsystem())
    {
        SettingsSub->UpdateAndApplySettings(DraftSettings, bPersistToDisk);
    }
}

UGuitarGameSettingsSubsystem* URealGuitarMenuWidget::GetSettingsSubsystem() const
{
    if (const UGameInstance* GI = GetGameInstance())
    {
        return GI->GetSubsystem<UGuitarGameSettingsSubsystem>();
    }
    return nullptr;
}

int32 URealGuitarMenuWidget::FindClosestResolutionIndex(const FIntPoint& Res) const
{
    int32 BestIdx = 0;
    int32 BestDist = MAX_int32;
    for (int32 i = 0; i < ResolutionChoices.Num(); ++i)
    {
        const int32 Dist = FMath::Abs(ResolutionChoices[i].X - Res.X) + FMath::Abs(ResolutionChoices[i].Y - Res.Y);
        if (Dist < BestDist)
        {
            BestDist = Dist;
            BestIdx = i;
        }
    }
    return BestIdx;
}

void URealGuitarMenuWidget::HandleResumeOrStartClicked()
{
    CloseMenu();
}

void URealGuitarMenuWidget::HandleBackToTitleClicked()
{
    OpenMenu(ERealGuitarMenuMode::TitleScreen, 0);
}

void URealGuitarMenuWidget::HandleQuitClicked()
{
    UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

void URealGuitarMenuWidget::HandleTabMainClicked() { SelectTab(0); }
void URealGuitarMenuWidget::HandleTabGraphicsClicked() { SelectTab(1); }
void URealGuitarMenuWidget::HandleTabAudioClicked() { SelectTab(2); }
void URealGuitarMenuWidget::HandleTabControlsClicked() { SelectTab(3); }

void URealGuitarMenuWidget::HandleTogglePlayModeClicked()
{
    if (UWorld* World = GetWorld())
    {
        if (UGuitarScoreSubsystem* Score = World->GetSubsystem<UGuitarScoreSubsystem>())
        {
            Score->ToggleControlMode();
            SyncSongAndPlayLabels();
        }
    }
}

void URealGuitarMenuWidget::HandleChordBankPrevClicked()
{
    const int32 NumBanks = FMath::Max(1, UGuitarSongLibrarySubsystem::GetNumChordBanks());
    DraftSettings.ChordBankIndex = (DraftSettings.ChordBankIndex - 1 + NumBanks) % NumBanks;
    if (UWorld* World = GetWorld())
    {
        if (UGuitarScoreSubsystem* Score = World->GetSubsystem<UGuitarScoreSubsystem>())
        {
            Score->SetChordBankIndex(DraftSettings.ChordBankIndex);
        }
    }
    SyncWidgetsFromSettings();
    PushDraftSettings(true);
}

void URealGuitarMenuWidget::HandleChordBankNextClicked()
{
    const int32 NumBanks = FMath::Max(1, UGuitarSongLibrarySubsystem::GetNumChordBanks());
    DraftSettings.ChordBankIndex = (DraftSettings.ChordBankIndex + 1) % NumBanks;
    if (UWorld* World = GetWorld())
    {
        if (UGuitarScoreSubsystem* Score = World->GetSubsystem<UGuitarScoreSubsystem>())
        {
            Score->SetChordBankIndex(DraftSettings.ChordBankIndex);
        }
    }
    SyncWidgetsFromSettings();
    PushDraftSettings(true);
}

void URealGuitarMenuWidget::HandleCharacterPrevClicked()
{
    DraftSettings.CharacterModelIndex = (DraftSettings.CharacterModelIndex == 0) ? 1 : 0;
    SyncWidgetsFromSettings();
    PushDraftSettings(true);
}

void URealGuitarMenuWidget::HandleCharacterNextClicked()
{
    HandleCharacterPrevClicked();
}

void URealGuitarMenuWidget::HandleSongPrevClicked()
{
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UGuitarSongLibrarySubsystem* Lib = GI->GetSubsystem<UGuitarSongLibrarySubsystem>())
        {
            Lib->PlayPreviousSong();
            SyncSongAndPlayLabels();
        }
    }
}

void URealGuitarMenuWidget::HandleSongNextClicked()
{
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UGuitarSongLibrarySubsystem* Lib = GI->GetSubsystem<UGuitarSongLibrarySubsystem>())
        {
            Lib->PlayNextSong();
            SyncSongAndPlayLabels();
        }
    }
}

void URealGuitarMenuWidget::HandlePlayPauseClicked()
{
    if (UWorld* World = GetWorld())
    {
        if (UGuitarScoreSubsystem* Score = World->GetSubsystem<UGuitarScoreSubsystem>())
        {
            if (Score->IsPlaying())
            {
                Score->Pause();
            }
            else
            {
                Score->SetControlMode(EPlayControlMode::AutoSong);
                Score->Play();
            }
            SyncSongAndPlayLabels();
        }
    }
}

void URealGuitarMenuWidget::HandleLoopToggled(bool bChecked)
{
    if (bSyncingUI) return;
    if (UWorld* World = GetWorld())
    {
        if (UGuitarScoreSubsystem* Score = World->GetSubsystem<UGuitarScoreSubsystem>())
        {
            Score->SetLooping(bChecked);
        }
    }
}

void URealGuitarMenuWidget::HandleJamBalladClicked()
{
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UGuitarSongLibrarySubsystem* Lib = GI->GetSubsystem<UGuitarSongLibrarySubsystem>())
        {
            Lib->GenerateAndPlayCustomChart({ TEXT("C"), TEXT("G"), TEXT("Am"), TEXT("F") }, 88.0f, false, TEXT("어쿠스틱 발라드 잼"), true);
            SyncSongAndPlayLabels();
        }
    }
}

void URealGuitarMenuWidget::HandleJamFingerstyleClicked()
{
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UGuitarSongLibrarySubsystem* Lib = GI->GetSubsystem<UGuitarSongLibrarySubsystem>())
        {
            Lib->GenerateAndPlayCustomChart({ TEXT("Em"), TEXT("C"), TEXT("G"), TEXT("D") }, 82.0f, true, TEXT("포크 핑거스타일 잼"), true);
            SyncSongAndPlayLabels();
        }
    }
}

void URealGuitarMenuWidget::HandleJamPopGrooveClicked()
{
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UGuitarSongLibrarySubsystem* Lib = GI->GetSubsystem<UGuitarSongLibrarySubsystem>())
        {
            Lib->GenerateAndPlayCustomChart({ TEXT("Fmaj7"), TEXT("G"), TEXT("Em7"), TEXT("Am7") }, 92.0f, false, TEXT("선셋 칠 로파이 잼"), true);
            SyncSongAndPlayLabels();
        }
    }
}

void URealGuitarMenuWidget::HandleResPrevClicked()
{
    if (ResolutionChoices.Num() == 0) return;
    ResolutionIndex = (ResolutionIndex - 1 + ResolutionChoices.Num()) % ResolutionChoices.Num();
    DraftSettings.Resolution = ResolutionChoices[ResolutionIndex];
    SyncWidgetsFromSettings();
}

void URealGuitarMenuWidget::HandleResNextClicked()
{
    if (ResolutionChoices.Num() == 0) return;
    ResolutionIndex = (ResolutionIndex + 1) % ResolutionChoices.Num();
    DraftSettings.Resolution = ResolutionChoices[ResolutionIndex];
    SyncWidgetsFromSettings();
}

void URealGuitarMenuWidget::HandleWinModePrevClicked()
{
    DraftSettings.WindowMode = (DraftSettings.WindowMode + 2) % 3;
    SyncWidgetsFromSettings();
}

void URealGuitarMenuWidget::HandleWinModeNextClicked()
{
    DraftSettings.WindowMode = (DraftSettings.WindowMode + 1) % 3;
    SyncWidgetsFromSettings();
}

void URealGuitarMenuWidget::HandleFpsPrevClicked()
{
    FpsChoiceIndex = (FpsChoiceIndex - 1 + kNumFpsChoices) % kNumFpsChoices;
    DraftSettings.FrameRateLimit = kFpsChoices[FpsChoiceIndex];
    SyncWidgetsFromSettings();
}

void URealGuitarMenuWidget::HandleFpsNextClicked()
{
    FpsChoiceIndex = (FpsChoiceIndex + 1) % kNumFpsChoices;
    DraftSettings.FrameRateLimit = kFpsChoices[FpsChoiceIndex];
    SyncWidgetsFromSettings();
}

void URealGuitarMenuWidget::HandleQualityPrevClicked()
{
    DraftSettings.GraphicsPreset = (DraftSettings.GraphicsPreset + 4) % 5;
    SyncWidgetsFromSettings();
}

void URealGuitarMenuWidget::HandleQualityNextClicked()
{
    DraftSettings.GraphicsPreset = (DraftSettings.GraphicsPreset + 1) % 5;
    SyncWidgetsFromSettings();
}

void URealGuitarMenuWidget::HandleVSyncToggled(bool bChecked)
{
    if (!bSyncingUI) DraftSettings.bVSync = bChecked;
}

void URealGuitarMenuWidget::HandleLumenToggled(bool bChecked)
{
    if (!bSyncingUI) DraftSettings.bLumenEnabled = bChecked;
}

void URealGuitarMenuWidget::HandleApplyGraphicsClicked()
{
    PushDraftSettings(true);
    if (GraphicsStatusText)
    {
        GraphicsStatusText->SetText(FText::FromString(TEXT("그래픽 및 해상도 설정이 저장되어 적용되었습니다.")));
    }
}

void URealGuitarMenuWidget::HandleResetDefaultsClicked()
{
    if (UGuitarGameSettingsSubsystem* SettingsSub = GetSettingsSubsystem())
    {
        SettingsSub->ResetToDefaults();
        DraftSettings = SettingsSub->GetSettings();
        SyncWidgetsFromSettings();
        if (GraphicsStatusText)
        {
            GraphicsStatusText->SetText(FText::FromString(TEXT("모든 설정을 기본값으로 복원했습니다.")));
        }
    }
}

void URealGuitarMenuWidget::HandleMasterVolChanged(float Value)
{
    if (bSyncingUI) return;
    DraftSettings.MasterVolume = FMath::Clamp(Value, 0.0f, 1.0f);
    if (MasterVolValueText) MasterVolValueText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Value * 100.0f))));
    PushDraftSettings(true);
}

void URealGuitarMenuWidget::HandleGuitarVolChanged(float Value)
{
    if (bSyncingUI) return;
    DraftSettings.GuitarVolume = FMath::Clamp(Value, 0.0f, 1.0f);
    if (GuitarVolValueText) GuitarVolValueText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Value * 100.0f))));
    PushDraftSettings(true);
}

void URealGuitarMenuWidget::HandleAmbientVolChanged(float Value)
{
    if (bSyncingUI) return;
    DraftSettings.AmbientVolume = FMath::Clamp(Value, 0.0f, 1.0f);
    if (AmbientVolValueText) AmbientVolValueText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Value * 100.0f))));
    PushDraftSettings(true);
}

void URealGuitarMenuWidget::HandleReverbChanged(float Value)
{
    if (bSyncingUI) return;
    DraftSettings.ReverbMix = FMath::Clamp(Value, 0.0f, 1.0f);
    if (ReverbValueText) ReverbValueText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Value * 100.0f))));
    PushDraftSettings(true);
}

void URealGuitarMenuWidget::HandleMoodSunsetClicked()
{
    DraftSettings.AtmosphereMood = ESanctuaryMood::GoldenSunset;
    PushDraftSettings(true);
    SyncWidgetsFromSettings();
}

void URealGuitarMenuWidget::HandleMoodNightClicked()
{
    DraftSettings.AtmosphereMood = ESanctuaryMood::CozyNightLamp;
    PushDraftSettings(true);
    SyncWidgetsFromSettings();
}

void URealGuitarMenuWidget::HandleMoodMorningClicked()
{
    DraftSettings.AtmosphereMood = ESanctuaryMood::MorningMist;
    PushDraftSettings(true);
    SyncWidgetsFromSettings();
}

void URealGuitarMenuWidget::HandleMoodRainClicked()
{
    DraftSettings.AtmosphereMood = ESanctuaryMood::RainyLoFi;
    PushDraftSettings(true);
    SyncWidgetsFromSettings();
}

void URealGuitarMenuWidget::HandleShowChordsToggled(bool bChecked)
{
    if (bSyncingUI) return;
    DraftSettings.bShowChordDiagrams = bChecked;
    PushDraftSettings(true);
}

void URealGuitarMenuWidget::HandleAutoHideHudToggled(bool bChecked)
{
    if (bSyncingUI) return;
    DraftSettings.bAutoHideHudWhenSeated = bChecked;
    PushDraftSettings(true);
}

void URealGuitarMenuWidget::HandleCamDistChanged(float Value)
{
    if (bSyncingUI) return;
    DraftSettings.CameraDistance = FMath::Clamp(Value, 140.0f, 460.0f);
    if (CamDistValueText) CamDistValueText->SetText(FText::FromString(FString::Printf(TEXT("%d cm"), FMath::RoundToInt(DraftSettings.CameraDistance))));
    PushDraftSettings(true);
}

void URealGuitarMenuWidget::HandleMouseSensChanged(float Value)
{
    if (bSyncingUI) return;
    DraftSettings.MouseSensitivity = FMath::Clamp(Value, 0.2f, 3.0f);
    if (MouseSensValueText) MouseSensValueText->SetText(FText::FromString(FString::Printf(TEXT("%.2fx"), DraftSettings.MouseSensitivity)));
    PushDraftSettings(true);
}

void URealGuitarMenuWidget::HandleFovChanged(float Value)
{
    if (bSyncingUI) return;
    DraftSettings.FieldOfView = FMath::Clamp(Value, 65.0f, 110.0f);
    if (FovValueText) FovValueText->SetText(FText::FromString(FString::Printf(TEXT("%d°"), FMath::RoundToInt(DraftSettings.FieldOfView))));
    PushDraftSettings(true);
}

void URealGuitarMenuWidget::HandleInvertYToggled(bool bChecked)
{
    if (bSyncingUI) return;
    DraftSettings.bInvertY = bChecked;
    PushDraftSettings(true);
}
