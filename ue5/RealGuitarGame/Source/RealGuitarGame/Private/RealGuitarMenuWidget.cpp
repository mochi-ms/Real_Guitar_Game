#include "RealGuitarMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/CheckBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GuitarScoreSubsystem.h"
#include "GuitarSongLibrarySubsystem.h"
#include "GuitarUIStyle.h"
#include "InputCoreTypes.h"
#include "Kismet/KismetSystemLibrary.h"

namespace
{
    constexpr float GFpsChoices[] = { 30.0f, 60.0f, 120.0f, 144.0f, 0.0f };
    constexpr int32 GNumFpsChoices = 5;

    FText FormatFpsText(float Limit)
    {
        if (Limit <= 0.0f)
        {
            return NSLOCTEXT("GuitarMenu", "FpsUnlim", "무제한 (Unlimited)");
        }
        return FText::FromString(FString::Printf(TEXT("%.0f FPS"), Limit));
    }

    FText FormatWindowModeText(int32 Mode)
    {
        switch (Mode)
        {
        case 0:
            return NSLOCTEXT("GuitarMenu", "WinFull", "전체화면 (Fullscreen)");
        case 2:
            return NSLOCTEXT("GuitarMenu", "WinWindow", "창모드 (Windowed)");
        default:
            return NSLOCTEXT("GuitarMenu", "WinBorderless", "테두리 없는 창");
        }
    }
}

#define BIND_BTN_CLICKED(Btn, Method) \
    if ((Btn) && !(Btn)->OnClicked.IsAlreadyBound(this, &URealGuitarMenuWidget::Method)) \
    { \
        (Btn)->OnClicked.AddDynamic(this, &URealGuitarMenuWidget::Method); \
    }

TSharedRef<SWidget> URealGuitarMenuWidget::RebuildWidget()
{
    SetIsFocusable(true);
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        BuildDefaultMenuLayout();
    }
    return Super::RebuildWidget();
}

void URealGuitarMenuWidget::BuildDefaultMenuLayout()
{
    UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("MenuRootCanvas"));
    WidgetTree->RootWidget = RootCanvas;

    // Full-screen translucent warm dark backdrop
    UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Backdrop"));
    Backdrop->SetBrushColor(GuitarUIStyle::BackdropScrim);
    Backdrop->SetHorizontalAlignment(HAlign_Center);
    Backdrop->SetVerticalAlignment(VAlign_Center);
    if (UCanvasPanelSlot* BgSlot = RootCanvas->AddChildToCanvas(Backdrop))
    {
        BgSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
        BgSlot->SetOffsets(FMargin(0.0f));
    }

    UVerticalBox* ShellVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());

    // Header
    HeroTitleText = GuitarUIStyle::MakeText(
        WidgetTree,
        NSLOCTEXT("GuitarMenu", "HeroTitle", "REAL GUITAR SANCTUARY"),
        28,
        GuitarUIStyle::AccentGold,
        true
    );
    HeroSubtitleText = GuitarUIStyle::MakeText(
        WidgetTree,
        NSLOCTEXT("GuitarMenu", "HeroSub", "실사 배경 속에서 감상하는 1인칭 오토 기타 연주 공간"),
        13,
        GuitarUIStyle::TextMuted
    );
    ShellVBox->AddChildToVerticalBox(HeroTitleText);
    if (UVerticalBoxSlot* SubSlot = ShellVBox->AddChildToVerticalBox(HeroSubtitleText))
    {
        SubSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 16.0f));
    }

    // Top Navigation Tabs
    UHorizontalBox* TabRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    UTextBlock* UnusedTabLbl = nullptr;
    TabMainButton = GuitarUIStyle::MakeButton(WidgetTree, NSLOCTEXT("GuitarMenu", "TabMain", "메인 · 악보"), UnusedTabLbl, true, 13);
    TabGraphicsButton = GuitarUIStyle::MakeButton(WidgetTree, NSLOCTEXT("GuitarMenu", "TabGfx", "화면 · 그래픽"), UnusedTabLbl, false, 13);
    TabAudioButton = GuitarUIStyle::MakeButton(WidgetTree, NSLOCTEXT("GuitarMenu", "TabAudio", "사운드 · 분위기"), UnusedTabLbl, false, 13);
    TabControlsButton = GuitarUIStyle::MakeButton(WidgetTree, NSLOCTEXT("GuitarMenu", "TabCtrl", "조작 · 키 안내"), UnusedTabLbl, false, 13);

    for (UButton* TabBtn : { TabMainButton.Get(), TabGraphicsButton.Get(), TabAudioButton.Get(), TabControlsButton.Get() })
    {
        if (UHorizontalBoxSlot* TSlot = TabRow->AddChildToHorizontalBox(TabBtn))
        {
            TSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            TSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
        }
    }
    if (UVerticalBoxSlot* TRowSlot = ShellVBox->AddChildToVerticalBox(TabRow))
    {
        TRowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));
    }

    // Tab Pages
    ContentSwitcher = WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass());
    ContentSwitcher->AddChild(BuildMainAndSongsTab());
    ContentSwitcher->AddChild(BuildGraphicsTab());
    ContentSwitcher->AddChild(BuildAudioAndMoodTab());
    ContentSwitcher->AddChild(BuildControlsTab());

    if (UVerticalBoxSlot* SwitchSlot = ShellVBox->AddChildToVerticalBox(ContentSwitcher))
    {
        SwitchSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }

    // Footer Action Bar
    UHorizontalBox* FooterRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    PrimaryActionButton = GuitarUIStyle::MakeButton(WidgetTree, NSLOCTEXT("GuitarMenu", "Resume", "계속 감상"), PrimaryActionLabel, true, 14, FMargin(22.0f, 9.0f));
    UTextBlock* TitleLbl = nullptr;
    BackToTitleButton = GuitarUIStyle::MakeButton(WidgetTree, NSLOCTEXT("GuitarMenu", "BackTitle", "타이틀 화면"), TitleLbl, false, 13);
    UTextBlock* QuitLbl = nullptr;
    QuitButton = GuitarUIStyle::MakeButton(WidgetTree, NSLOCTEXT("GuitarMenu", "Quit", "게임 종료"), QuitLbl, false, 13);

    if (UHorizontalBoxSlot* S1 = FooterRow->AddChildToHorizontalBox(PrimaryActionButton))
    {
        S1->SetPadding(FMargin(0.0f, 0.0f, 10.0f, 0.0f));
    }
    if (UHorizontalBoxSlot* S2 = FooterRow->AddChildToHorizontalBox(BackToTitleButton))
    {
        S2->SetPadding(FMargin(0.0f, 0.0f, 10.0f, 0.0f));
    }
    GuitarUIStyle::AddSpacer(WidgetTree, FooterRow, 20.0f, 1.0f);
    if (UHorizontalBoxSlot* S3 = FooterRow->AddChildToHorizontalBox(QuitButton))
    {
        S3->SetHorizontalAlignment(HAlign_Right);
    }

    if (UVerticalBoxSlot* FSlot = ShellVBox->AddChildToVerticalBox(FooterRow))
    {
        FSlot->SetPadding(FMargin(0.0f, 18.0f, 0.0f, 0.0f));
    }

    UBorder* MainCard = GuitarUIStyle::MakeGlassCard(WidgetTree, ShellVBox, FMargin(32.0f, 28.0f));
    Backdrop->SetContent(MainCard);
}

UWidget* URealGuitarMenuWidget::BuildMainAndSongsTab()
{
    UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());

    Box->AddChildToVerticalBox(GuitarUIStyle::MakeText(
        WidgetTree, NSLOCTEXT("GuitarMenu", "SecSongs", "현재 연주곡 / 악보 라이브러리"), 15, GuitarUIStyle::AccentGold, true));

    GuitarUIStyle::AddStepperRow(
        WidgetTree, Box, NSLOCTEXT("GuitarMenu", "ActiveSong", "선택된 악보"),
        SongPrevButton, SongNameValueText, SongNextButton
    );

    UHorizontalBox* PlaybackRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    UTextBlock* PlayLbl = nullptr;
    SongPlayPauseButton = GuitarUIStyle::MakeButton(WidgetTree, NSLOCTEXT("GuitarMenu", "TogglePlay", "재생 / 일시정지"), PlayLbl, true, 13);
    PlaybackRow->AddChildToHorizontalBox(SongPlayPauseButton);
    Box->AddChildToVerticalBox(PlaybackRow);

    GuitarUIStyle::AddCheckBoxRow(WidgetTree, Box, NSLOCTEXT("GuitarMenu", "LoopToggle", "현재 곡 반복 재생 (Loop)"), LoopSongCheckBox);

    GuitarUIStyle::AddSpacer(WidgetTree, Box, 1.0f, 14.0f);
    Box->AddChildToVerticalBox(GuitarUIStyle::MakeText(
        WidgetTree, NSLOCTEXT("GuitarMenu", "SecQuickJam", "즉석 반주 패턴 (코드/아르페지오/스트로크)"), 15, GuitarUIStyle::AccentGold, true));

    UHorizontalBox* JamRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    UTextBlock* Lbl1 = nullptr;
    UTextBlock* Lbl2 = nullptr;
    UTextBlock* Lbl3 = nullptr;
    JamBalladButton = GuitarUIStyle::MakeButton(WidgetTree, NSLOCTEXT("GuitarMenu", "JamBallad", "어쿠스틱 발라드 (C-G-Am-F)"), Lbl1, false, 12);
    JamFingerButton = GuitarUIStyle::MakeButton(WidgetTree, NSLOCTEXT("GuitarMenu", "JamFinger", "핑거스타일 아르페지오 (D-A-Bm-G)"), Lbl2, false, 12);
    JamPopButton = GuitarUIStyle::MakeButton(WidgetTree, NSLOCTEXT("GuitarMenu", "JamPop", "감성 팝 스트로크 (Em-C-G-D)"), Lbl3, false, 12);

    for (UButton* Btn : { JamBalladButton.Get(), JamFingerButton.Get(), JamPopButton.Get() })
    {
        if (UHorizontalBoxSlot* Slot = JamRow->AddChildToHorizontalBox(Btn))
        {
            Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            Slot->SetPadding(FMargin(0.0f, 8.0f, 8.0f, 0.0f));
        }
    }
    Box->AddChildToVerticalBox(JamRow);

    return GuitarUIStyle::MakeGlassCard(WidgetTree, Box, FMargin(20.0f, 18.0f), GuitarUIStyle::CardSurface);
}

UWidget* URealGuitarMenuWidget::BuildGraphicsTab()
{
    UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());

    GuitarUIStyle::AddStepperRow(WidgetTree, Box, NSLOCTEXT("GuitarMenu", "Res", "해상도 (Resolution)"), ResPrevButton, ResValueText, ResNextButton);
    GuitarUIStyle::AddStepperRow(WidgetTree, Box, NSLOCTEXT("GuitarMenu", "WinMode", "화면 모드 (Window Mode)"), WinModePrevButton, WinModeValueText, WinModeNextButton);
    GuitarUIStyle::AddStepperRow(WidgetTree, Box, NSLOCTEXT("GuitarMenu", "FpsCap", "최대 프레임 (FPS Cap)"), FpsPrevButton, FpsValueText, FpsNextButton);
    GuitarUIStyle::AddStepperRow(WidgetTree, Box, NSLOCTEXT("GuitarMenu", "Quality", "그래픽 프리셋 (Scalability)"), QualityPrevButton, QualityValueText, QualityNextButton);
    GuitarUIStyle::AddCheckBoxRow(WidgetTree, Box, NSLOCTEXT("GuitarMenu", "VSync", "수직 동기화 (VSync)"), VSyncCheckBox);
    GuitarUIStyle::AddCheckBoxRow(WidgetTree, Box, NSLOCTEXT("GuitarMenu", "Lumen", "Lumen 전역 조명 & 반사 활성화"), LumenCheckBox);

    UHorizontalBox* ActionRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    UTextBlock* ApplyLbl = nullptr;
    UTextBlock* ResetLbl = nullptr;
    ApplyGraphicsButton = GuitarUIStyle::MakeButton(WidgetTree, NSLOCTEXT("GuitarMenu", "ApplyGfx", "그래픽 적용 및 저장"), ApplyLbl, true, 13);
    ResetDefaultsButton = GuitarUIStyle::MakeButton(WidgetTree, NSLOCTEXT("GuitarMenu", "ResetDef", "기본값 복원"), ResetLbl, false, 13);

    if (UHorizontalBoxSlot* S1 = ActionRow->AddChildToHorizontalBox(ApplyGraphicsButton))
    {
        S1->SetPadding(FMargin(0.0f, 10.0f, 10.0f, 0.0f));
    }
    if (UHorizontalBoxSlot* S2 = ActionRow->AddChildToHorizontalBox(ResetDefaultsButton))
    {
        S2->SetPadding(FMargin(0.0f, 10.0f, 10.0f, 0.0f));
    }
    Box->AddChildToVerticalBox(ActionRow);

    GraphicsStatusText = GuitarUIStyle::MakeText(WidgetTree, FText::GetEmpty(), 12, GuitarUIStyle::AccentGold);
    Box->AddChildToVerticalBox(GraphicsStatusText);

    return GuitarUIStyle::MakeGlassCard(WidgetTree, Box, FMargin(20.0f, 18.0f), GuitarUIStyle::CardSurface);
}

UWidget* URealGuitarMenuWidget::BuildAudioAndMoodTab()
{
    UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());

    GuitarUIStyle::AddSettingSliderRow(WidgetTree, Box, NSLOCTEXT("GuitarMenu", "MasterVol", "마스터 볼륨"), 0.0f, 1.0f, 0.01f, MasterVolSlider, MasterVolValueText);
    GuitarUIStyle::AddSettingSliderRow(WidgetTree, Box, NSLOCTEXT("GuitarMenu", "GuitarVol", "기타 사운드 볼륨"), 0.0f, 1.0f, 0.01f, GuitarVolSlider, GuitarVolValueText);
    GuitarUIStyle::AddSettingSliderRow(WidgetTree, Box, NSLOCTEXT("GuitarMenu", "AmbientVol", "환경음(자연/빗소리) 볼륨"), 0.0f, 1.0f, 0.01f, AmbientVolSlider, AmbientVolValueText);
    GuitarUIStyle::AddSettingSliderRow(WidgetTree, Box, NSLOCTEXT("GuitarMenu", "ReverbMix", "공간감 리버브 (Reverb)"), 0.0f, 1.0f, 0.01f, ReverbSlider, ReverbValueText);

    GuitarUIStyle::AddSpacer(WidgetTree, Box, 1.0f, 10.0f);
    CurrentMoodLabel = GuitarUIStyle::MakeText(WidgetTree, NSLOCTEXT("GuitarMenu", "MoodHeader", "시간대 · 분위기 프리셋"), 14, GuitarUIStyle::AccentGold, true);
    Box->AddChildToVerticalBox(CurrentMoodLabel);

    UHorizontalBox* MoodRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    UTextBlock* M1 = nullptr;
    UTextBlock* M2 = nullptr;
    UTextBlock* M3 = nullptr;
    UTextBlock* M4 = nullptr;
    MoodSunsetButton = GuitarUIStyle::MakeButton(WidgetTree, NSLOCTEXT("GuitarMenu", "MoodSunset", "황금빛 노을"), M1, false, 12);
    MoodNightButton = GuitarUIStyle::MakeButton(WidgetTree, NSLOCTEXT("GuitarMenu", "MoodNight", "아늑한 밤 램프"), M2, false, 12);
    MoodMorningButton = GuitarUIStyle::MakeButton(WidgetTree, NSLOCTEXT("GuitarMenu", "MoodMorn", "아침 물안개"), M3, false, 12);
    MoodRainButton = GuitarUIStyle::MakeButton(WidgetTree, NSLOCTEXT("GuitarMenu", "MoodRain", "비 오는 로파이"), M4, false, 12);

    for (UButton* Btn : { MoodSunsetButton.Get(), MoodNightButton.Get(), MoodMorningButton.Get(), MoodRainButton.Get() })
    {
        if (UHorizontalBoxSlot* Slot = MoodRow->AddChildToHorizontalBox(Btn))
        {
            Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            Slot->SetPadding(FMargin(0.0f, 6.0f, 6.0f, 6.0f));
        }
    }
    Box->AddChildToVerticalBox(MoodRow);

    GuitarUIStyle::AddCheckBoxRow(WidgetTree, Box, NSLOCTEXT("GuitarMenu", "ShowChords", "연주 중 코드 배지 표시"), ShowChordsCheckBox);
    GuitarUIStyle::AddCheckBoxRow(WidgetTree, Box, NSLOCTEXT("GuitarMenu", "AutoHide", "벤치 착석 시 HUD 자동 숨김 (몰입 모드)"), AutoHideHudCheckBox);

    return GuitarUIStyle::MakeGlassCard(WidgetTree, Box, FMargin(20.0f, 18.0f), GuitarUIStyle::CardSurface);
}

UWidget* URealGuitarMenuWidget::BuildControlsTab()
{
    UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());

    GuitarUIStyle::AddSettingSliderRow(WidgetTree, Box, NSLOCTEXT("GuitarMenu", "MouseSens", "마우스 시점 감도"), 0.2f, 3.0f, 0.05f, MouseSensSlider, MouseSensValueText);
    GuitarUIStyle::AddSettingSliderRow(WidgetTree, Box, NSLOCTEXT("GuitarMenu", "Fov", "시야각 (FOV)"), 60.0f, 105.0f, 1.0f, FovSlider, FovValueText);
    GuitarUIStyle::AddCheckBoxRow(WidgetTree, Box, NSLOCTEXT("GuitarMenu", "InvertY", "마우스 Y축 반전 (Invert Y)"), InvertYCheckBox);

    GuitarUIStyle::AddSpacer(WidgetTree, Box, 1.0f, 12.0f);
    Box->AddChildToVerticalBox(GuitarUIStyle::MakeText(
        WidgetTree,
        NSLOCTEXT(
            "GuitarMenu",
            "KeybindGuide",
            "조작 키 안내:\n"
            "  • W / A / S / D : 공간 자유 이동\n"
            "  • E : 벤치·의자 앉기 / 일어나기\n"
            "  • Space : 연주 재생 / 일시정지\n"
            "  • C : 카메라 앵글 전환 (전체 / 왼손 프렛 / 오른손 피킹)\n"
            "  • N : 다음 악보 전환   ·   T : 분위기 프리셋 전환\n"
            "  • H : 시네마틱 HUD 숨기기   ·   ESC : 일시정지 / 환경설정"
        ),
        13,
        GuitarUIStyle::TextMuted
    ));

    return GuitarUIStyle::MakeGlassCard(WidgetTree, Box, FMargin(20.0f, 18.0f), GuitarUIStyle::CardSurface);
}

void URealGuitarMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();

    BIND_BTN_CLICKED(PrimaryActionButton, HandleResumeOrStartClicked);
    BIND_BTN_CLICKED(BackToTitleButton, HandleBackToTitleClicked);
    BIND_BTN_CLICKED(QuitButton, HandleQuitClicked);

    BIND_BTN_CLICKED(TabMainButton, HandleTabMainClicked);
    BIND_BTN_CLICKED(TabGraphicsButton, HandleTabGraphicsClicked);
    BIND_BTN_CLICKED(TabAudioButton, HandleTabAudioClicked);
    BIND_BTN_CLICKED(TabControlsButton, HandleTabControlsClicked);

    BIND_BTN_CLICKED(SongPrevButton, HandleSongPrevClicked);
    BIND_BTN_CLICKED(SongNextButton, HandleSongNextClicked);
    BIND_BTN_CLICKED(SongPlayPauseButton, HandlePlayPauseClicked);
    BIND_BTN_CLICKED(JamBalladButton, HandleJamBalladClicked);
    BIND_BTN_CLICKED(JamFingerButton, HandleJamFingerstyleClicked);
    BIND_BTN_CLICKED(JamPopButton, HandleJamPopGrooveClicked);

    BIND_BTN_CLICKED(ResPrevButton, HandleResPrevClicked);
    BIND_BTN_CLICKED(ResNextButton, HandleResNextClicked);
    BIND_BTN_CLICKED(WinModePrevButton, HandleWinModePrevClicked);
    BIND_BTN_CLICKED(WinModeNextButton, HandleWinModeNextClicked);
    BIND_BTN_CLICKED(FpsPrevButton, HandleFpsPrevClicked);
    BIND_BTN_CLICKED(FpsNextButton, HandleFpsNextClicked);
    BIND_BTN_CLICKED(QualityPrevButton, HandleQualityPrevClicked);
    BIND_BTN_CLICKED(QualityNextButton, HandleQualityNextClicked);
    BIND_BTN_CLICKED(ApplyGraphicsButton, HandleApplyGraphicsClicked);
    BIND_BTN_CLICKED(ResetDefaultsButton, HandleResetDefaultsClicked);

    BIND_BTN_CLICKED(MoodSunsetButton, HandleMoodSunsetClicked);
    BIND_BTN_CLICKED(MoodNightButton, HandleMoodNightClicked);
    BIND_BTN_CLICKED(MoodMorningButton, HandleMoodMorningClicked);
    BIND_BTN_CLICKED(MoodRainButton, HandleMoodRainClicked);

    if (LoopSongCheckBox && !LoopSongCheckBox->OnCheckStateChanged.IsAlreadyBound(this, &URealGuitarMenuWidget::HandleLoopToggled))
    {
        LoopSongCheckBox->OnCheckStateChanged.AddDynamic(this, &URealGuitarMenuWidget::HandleLoopToggled);
    }
    if (VSyncCheckBox && !VSyncCheckBox->OnCheckStateChanged.IsAlreadyBound(this, &URealGuitarMenuWidget::HandleVSyncToggled))
    {
        VSyncCheckBox->OnCheckStateChanged.AddDynamic(this, &URealGuitarMenuWidget::HandleVSyncToggled);
    }
    if (LumenCheckBox && !LumenCheckBox->OnCheckStateChanged.IsAlreadyBound(this, &URealGuitarMenuWidget::HandleLumenToggled))
    {
        LumenCheckBox->OnCheckStateChanged.AddDynamic(this, &URealGuitarMenuWidget::HandleLumenToggled);
    }
    if (ShowChordsCheckBox && !ShowChordsCheckBox->OnCheckStateChanged.IsAlreadyBound(this, &URealGuitarMenuWidget::HandleShowChordsToggled))
    {
        ShowChordsCheckBox->OnCheckStateChanged.AddDynamic(this, &URealGuitarMenuWidget::HandleShowChordsToggled);
    }
    if (AutoHideHudCheckBox && !AutoHideHudCheckBox->OnCheckStateChanged.IsAlreadyBound(this, &URealGuitarMenuWidget::HandleAutoHideHudToggled))
    {
        AutoHideHudCheckBox->OnCheckStateChanged.AddDynamic(this, &URealGuitarMenuWidget::HandleAutoHideHudToggled);
    }
    if (InvertYCheckBox && !InvertYCheckBox->OnCheckStateChanged.IsAlreadyBound(this, &URealGuitarMenuWidget::HandleInvertYToggled))
    {
        InvertYCheckBox->OnCheckStateChanged.AddDynamic(this, &URealGuitarMenuWidget::HandleInvertYToggled);
    }

    if (MasterVolSlider && !MasterVolSlider->OnValueChanged.IsAlreadyBound(this, &URealGuitarMenuWidget::HandleMasterVolChanged))
    {
        MasterVolSlider->OnValueChanged.AddDynamic(this, &URealGuitarMenuWidget::HandleMasterVolChanged);
    }
    if (GuitarVolSlider && !GuitarVolSlider->OnValueChanged.IsAlreadyBound(this, &URealGuitarMenuWidget::HandleGuitarVolChanged))
    {
        GuitarVolSlider->OnValueChanged.AddDynamic(this, &URealGuitarMenuWidget::HandleGuitarVolChanged);
    }
    if (AmbientVolSlider && !AmbientVolSlider->OnValueChanged.IsAlreadyBound(this, &URealGuitarMenuWidget::HandleAmbientVolChanged))
    {
        AmbientVolSlider->OnValueChanged.AddDynamic(this, &URealGuitarMenuWidget::HandleAmbientVolChanged);
    }
    if (ReverbSlider && !ReverbSlider->OnValueChanged.IsAlreadyBound(this, &URealGuitarMenuWidget::HandleReverbChanged))
    {
        ReverbSlider->OnValueChanged.AddDynamic(this, &URealGuitarMenuWidget::HandleReverbChanged);
    }
    if (MouseSensSlider && !MouseSensSlider->OnValueChanged.IsAlreadyBound(this, &URealGuitarMenuWidget::HandleMouseSensChanged))
    {
        MouseSensSlider->OnValueChanged.AddDynamic(this, &URealGuitarMenuWidget::HandleMouseSensChanged);
    }
    if (FovSlider && !FovSlider->OnValueChanged.IsAlreadyBound(this, &URealGuitarMenuWidget::HandleFovChanged))
    {
        FovSlider->OnValueChanged.AddDynamic(this, &URealGuitarMenuWidget::HandleFovChanged);
    }
}

FReply URealGuitarMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    if (InKeyEvent.GetKey() == EKeys::Escape && CurrentMode == ERealGuitarMenuMode::PauseMenu && bMenuOpen)
    {
        CloseMenu();
        return FReply::Handled();
    }
    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void URealGuitarMenuWidget::OpenMenu(ERealGuitarMenuMode InMode, int32 InitialTabIndex)
{
    CurrentMode = InMode;
    bMenuOpen = true;
    SetVisibility(ESlateVisibility::Visible);

    const bool bIsTitle = (CurrentMode == ERealGuitarMenuMode::TitleScreen);
    if (PrimaryActionLabel)
    {
        PrimaryActionLabel->SetText(bIsTitle
            ? NSLOCTEXT("GuitarMenu", "StartSanctuary", "감상 시작하기")
            : NSLOCTEXT("GuitarMenu", "ResumePlay", "계속 감상하기"));
    }
    if (BackToTitleButton)
    {
        BackToTitleButton->SetVisibility(bIsTitle ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    }

    if (APlayerController* PC = GetOwningPlayer())
    {
        PC->SetShowMouseCursor(true);
        PC->SetIgnoreMoveInput(true);
        PC->SetIgnoreLookInput(true);
        FInputModeGameAndUI Mode;
        Mode.SetHideCursorDuringCapture(false);
        Mode.SetWidgetToFocus(TakeWidget());
        PC->SetInputMode(Mode);
    }

    SelectTab(InitialTabIndex);
    SyncWidgetsFromSettings();
    RefreshSongDisplay();
}

void URealGuitarMenuWidget::CloseMenu()
{
    bMenuOpen = false;
    SetVisibility(ESlateVisibility::Collapsed);

    if (APlayerController* PC = GetOwningPlayer())
    {
        PC->SetIgnoreMoveInput(false);
        PC->SetIgnoreLookInput(false);
    }

    OnMenuClosed.Broadcast();
}

void URealGuitarMenuWidget::SelectTab(int32 TabIndex)
{
    const int32 Clamped = FMath::Clamp(TabIndex, 0, 3);
    if (ContentSwitcher)
    {
        ContentSwitcher->SetActiveWidgetIndex(Clamped);
    }
    UButton* Tabs[] = { TabMainButton.Get(), TabGraphicsButton.Get(), TabAudioButton.Get(), TabControlsButton.Get() };
    for (int32 i = 0; i < 4; ++i)
    {
        if (Tabs[i])
        {
            Tabs[i]->SetBackgroundColor(i == Clamped ? GuitarUIStyle::AccentGoldDim : GuitarUIStyle::ButtonSecondary);
        }
    }
}

void URealGuitarMenuWidget::SyncWidgetsFromSettings()
{
    bSyncingUI = true;

    if (const UGameInstance* GI = GetGameInstance())
    {
        if (const UGuitarGameSettingsSubsystem* SettingsSub = GI->GetSubsystem<UGuitarGameSettingsSubsystem>())
        {
            DraftSettings = SettingsSub->GetSettings();
        }
    }

    ResolutionChoices = UGuitarGameSettingsSubsystem::GetSupportedResolutions();
    ResolutionIndex = ResolutionChoices.IndexOfByKey(DraftSettings.Resolution);
    if (ResolutionIndex == INDEX_NONE)
    {
        ResolutionIndex = FMath::Min(2, FMath::Max(0, ResolutionChoices.Num() - 1));
    }

    FpsChoiceIndex = 1;
    for (int32 i = 0; i < GNumFpsChoices; ++i)
    {
        if (FMath::IsNearlyEqual(DraftSettings.FrameRateLimit, GFpsChoices[i], 1.0f))
        {
            FpsChoiceIndex = i;
            break;
        }
    }

    if (ResValueText && ResolutionChoices.IsValidIndex(ResolutionIndex))
    {
        const FIntPoint R = ResolutionChoices[ResolutionIndex];
        ResValueText->SetText(FText::FromString(FString::Printf(TEXT("%d x %d"), R.X, R.Y)));
    }
    if (WinModeValueText) WinModeValueText->SetText(FormatWindowModeText(DraftSettings.WindowMode));
    if (FpsValueText) FpsValueText->SetText(FormatFpsText(DraftSettings.FrameRateLimit));
    if (QualityValueText) QualityValueText->SetText(UGuitarGameSettingsSubsystem::GetGraphicsPresetName(DraftSettings.GraphicsPreset));
    if (VSyncCheckBox) VSyncCheckBox->SetIsChecked(DraftSettings.bVSync);
    if (LumenCheckBox) LumenCheckBox->SetIsChecked(DraftSettings.bLumenEnabled);

    if (MasterVolSlider) MasterVolSlider->SetValue(DraftSettings.MasterVolume);
    if (MasterVolValueText) MasterVolValueText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(DraftSettings.MasterVolume * 100.0f))));
    if (GuitarVolSlider) GuitarVolSlider->SetValue(DraftSettings.GuitarVolume);
    if (GuitarVolValueText) GuitarVolValueText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(DraftSettings.GuitarVolume * 100.0f))));
    if (AmbientVolSlider) AmbientVolSlider->SetValue(DraftSettings.AmbientVolume);
    if (AmbientVolValueText) AmbientVolValueText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(DraftSettings.AmbientVolume * 100.0f))));
    if (ReverbSlider) ReverbSlider->SetValue(DraftSettings.ReverbMix);
    if (ReverbValueText) ReverbValueText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(DraftSettings.ReverbMix * 100.0f))));

    if (CurrentMoodLabel)
    {
        CurrentMoodLabel->SetText(FText::Format(
            NSLOCTEXT("GuitarMenu", "MoodCur", "시간대 · 분위기 프리셋: {0}"),
            UGuitarGameSettingsSubsystem::GetMoodDisplayName(DraftSettings.AtmosphereMood)
        ));
    }
    if (ShowChordsCheckBox) ShowChordsCheckBox->SetIsChecked(DraftSettings.bShowChordDiagrams);
    if (AutoHideHudCheckBox) AutoHideHudCheckBox->SetIsChecked(DraftSettings.bAutoHideHudWhenSeated);

    if (MouseSensSlider) MouseSensSlider->SetValue(DraftSettings.MouseSensitivity);
    if (MouseSensValueText) MouseSensValueText->SetText(FText::FromString(FString::Printf(TEXT("%.2fx"), DraftSettings.MouseSensitivity)));
    if (FovSlider) FovSlider->SetValue(DraftSettings.FieldOfView);
    if (FovValueText) FovValueText->SetText(FText::FromString(FString::Printf(TEXT("%.0f°"), DraftSettings.FieldOfView)));
    if (InvertYCheckBox) InvertYCheckBox->SetIsChecked(DraftSettings.bInvertY);

    bSyncingUI = false;
}

void URealGuitarMenuWidget::RefreshSongDisplay()
{
    const UWorld* World = GetWorld();
    const UGuitarScoreSubsystem* ScoreSub = World ? World->GetSubsystem<UGuitarScoreSubsystem>() : nullptr;
    if (ScoreSub)
    {
        const FGuitarPerformanceData& Data = ScoreSub->GetLoadedPerformance();
        if (SongNameValueText)
        {
            SongNameValueText->SetText(Data.Title.IsEmpty()
                ? NSLOCTEXT("GuitarMenu", "NoSong", "악보 없음")
                : FText::FromString(Data.Title));
        }
        if (LoopSongCheckBox)
        {
            bSyncingUI = true;
            LoopSongCheckBox->SetIsChecked(ScoreSub->IsLooping());
            bSyncingUI = false;
        }
    }
}

void URealGuitarMenuWidget::PushDraftSettings(bool bPersist)
{
    if (bSyncingUI)
    {
        return;
    }
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UGuitarGameSettingsSubsystem* SettingsSub = GI->GetSubsystem<UGuitarGameSettingsSubsystem>())
        {
            SettingsSub->UpdateAndApplySettings(DraftSettings, bPersist);
        }
    }
}

void URealGuitarMenuWidget::HandleResumeOrStartClicked() { CloseMenu(); }

void URealGuitarMenuWidget::HandleBackToTitleClicked()
{
    OpenMenu(ERealGuitarMenuMode::TitleScreen, 0);
}

void URealGuitarMenuWidget::HandleQuitClicked()
{
    UKismetSystemLibrary::QuitGame(GetWorld(), GetOwningPlayer(), EQuitPreference::Quit, false);
}

void URealGuitarMenuWidget::HandleTabMainClicked() { SelectTab(0); }
void URealGuitarMenuWidget::HandleTabGraphicsClicked() { SelectTab(1); }
void URealGuitarMenuWidget::HandleTabAudioClicked() { SelectTab(2); }
void URealGuitarMenuWidget::HandleTabControlsClicked() { SelectTab(3); }

void URealGuitarMenuWidget::HandleSongPrevClicked()
{
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UGuitarSongLibrarySubsystem* Lib = GI->GetSubsystem<UGuitarSongLibrarySubsystem>())
        {
            Lib->PlayPreviousSong();
            RefreshSongDisplay();
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
            RefreshSongDisplay();
        }
    }
}

void URealGuitarMenuWidget::HandlePlayPauseClicked()
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

void URealGuitarMenuWidget::HandleLoopToggled(bool bChecked)
{
    if (bSyncingUI) return;
    if (UWorld* World = GetWorld())
    {
        if (UGuitarScoreSubsystem* ScoreSub = World->GetSubsystem<UGuitarScoreSubsystem>())
        {
            ScoreSub->SetLooping(bChecked);
        }
    }
}

void URealGuitarMenuWidget::HandleJamBalladClicked()
{
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UGuitarSongLibrarySubsystem* Lib = GI->GetSubsystem<UGuitarSongLibrarySubsystem>())
        {
            Lib->GenerateAndPlayCustomChart({ TEXT("C"), TEXT("G"), TEXT("Am"), TEXT("F") }, 76.0f, false, TEXT("folk_ballad"), true);
            RefreshSongDisplay();
        }
    }
}

void URealGuitarMenuWidget::HandleJamFingerstyleClicked()
{
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UGuitarSongLibrarySubsystem* Lib = GI->GetSubsystem<UGuitarSongLibrarySubsystem>())
        {
            Lib->GenerateAndPlayCustomChart({ TEXT("D"), TEXT("A"), TEXT("Bm"), TEXT("G") }, 84.0f, true, TEXT("travis_roll"), true);
            RefreshSongDisplay();
        }
    }
}

void URealGuitarMenuWidget::HandleJamPopGrooveClicked()
{
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UGuitarSongLibrarySubsystem* Lib = GI->GetSubsystem<UGuitarSongLibrarySubsystem>())
        {
            Lib->GenerateAndPlayCustomChart({ TEXT("Em"), TEXT("C"), TEXT("G"), TEXT("D") }, 98.0f, false, TEXT("pop_sixteenth"), true);
            RefreshSongDisplay();
        }
    }
}

void URealGuitarMenuWidget::HandleResPrevClicked()
{
    if (ResolutionChoices.Num() == 0) return;
    ResolutionIndex = (ResolutionIndex - 1 + ResolutionChoices.Num()) % ResolutionChoices.Num();
    DraftSettings.Resolution = ResolutionChoices[ResolutionIndex];
    if (ResValueText) ResValueText->SetText(FText::FromString(FString::Printf(TEXT("%d x %d"), DraftSettings.Resolution.X, DraftSettings.Resolution.Y)));
}

void URealGuitarMenuWidget::HandleResNextClicked()
{
    if (ResolutionChoices.Num() == 0) return;
    ResolutionIndex = (ResolutionIndex + 1) % ResolutionChoices.Num();
    DraftSettings.Resolution = ResolutionChoices[ResolutionIndex];
    if (ResValueText) ResValueText->SetText(FText::FromString(FString::Printf(TEXT("%d x %d"), DraftSettings.Resolution.X, DraftSettings.Resolution.Y)));
}

void URealGuitarMenuWidget::HandleWinModePrevClicked()
{
    DraftSettings.WindowMode = (DraftSettings.WindowMode + 2) % 3;
    if (WinModeValueText) WinModeValueText->SetText(FormatWindowModeText(DraftSettings.WindowMode));
}

void URealGuitarMenuWidget::HandleWinModeNextClicked()
{
    DraftSettings.WindowMode = (DraftSettings.WindowMode + 1) % 3;
    if (WinModeValueText) WinModeValueText->SetText(FormatWindowModeText(DraftSettings.WindowMode));
}

void URealGuitarMenuWidget::HandleFpsPrevClicked()
{
    FpsChoiceIndex = (FpsChoiceIndex - 1 + GNumFpsChoices) % GNumFpsChoices;
    DraftSettings.FrameRateLimit = GFpsChoices[FpsChoiceIndex];
    if (FpsValueText) FpsValueText->SetText(FormatFpsText(DraftSettings.FrameRateLimit));
}

void URealGuitarMenuWidget::HandleFpsNextClicked()
{
    FpsChoiceIndex = (FpsChoiceIndex + 1) % GNumFpsChoices;
    DraftSettings.FrameRateLimit = GFpsChoices[FpsChoiceIndex];
    if (FpsValueText) FpsValueText->SetText(FormatFpsText(DraftSettings.FrameRateLimit));
}

void URealGuitarMenuWidget::HandleQualityPrevClicked()
{
    DraftSettings.GraphicsPreset = (DraftSettings.GraphicsPreset + 4) % 5;
    if (QualityValueText) QualityValueText->SetText(UGuitarGameSettingsSubsystem::GetGraphicsPresetName(DraftSettings.GraphicsPreset));
}

void URealGuitarMenuWidget::HandleQualityNextClicked()
{
    DraftSettings.GraphicsPreset = (DraftSettings.GraphicsPreset + 1) % 5;
    if (QualityValueText) QualityValueText->SetText(UGuitarGameSettingsSubsystem::GetGraphicsPresetName(DraftSettings.GraphicsPreset));
}

void URealGuitarMenuWidget::HandleVSyncToggled(bool bChecked)
{
    if (bSyncingUI) return;
    DraftSettings.bVSync = bChecked;
}

void URealGuitarMenuWidget::HandleLumenToggled(bool bChecked)
{
    if (bSyncingUI) return;
    DraftSettings.bLumenEnabled = bChecked;
}

void URealGuitarMenuWidget::HandleApplyGraphicsClicked()
{
    PushDraftSettings(true);
    if (GraphicsStatusText)
    {
        GraphicsStatusText->SetText(NSLOCTEXT("GuitarMenu", "AppliedOk", "그래픽 설정이 적용 및 저장되었습니다."));
    }
}

void URealGuitarMenuWidget::HandleResetDefaultsClicked()
{
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UGuitarGameSettingsSubsystem* SettingsSub = GI->GetSubsystem<UGuitarGameSettingsSubsystem>())
        {
            SettingsSub->ResetToDefaults();
            SyncWidgetsFromSettings();
            if (GraphicsStatusText)
            {
                GraphicsStatusText->SetText(NSLOCTEXT("GuitarMenu", "ResetOk", "기본 설정으로 복원되었습니다."));
            }
        }
    }
}

void URealGuitarMenuWidget::HandleMasterVolChanged(float Value)
{
    DraftSettings.MasterVolume = Value;
    if (MasterVolValueText) MasterVolValueText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Value * 100.0f))));
    PushDraftSettings(true);
}

void URealGuitarMenuWidget::HandleGuitarVolChanged(float Value)
{
    DraftSettings.GuitarVolume = Value;
    if (GuitarVolValueText) GuitarVolValueText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Value * 100.0f))));
    PushDraftSettings(true);
}

void URealGuitarMenuWidget::HandleAmbientVolChanged(float Value)
{
    DraftSettings.AmbientVolume = Value;
    if (AmbientVolValueText) AmbientVolValueText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Value * 100.0f))));
    PushDraftSettings(true);
}

void URealGuitarMenuWidget::HandleReverbChanged(float Value)
{
    DraftSettings.ReverbMix = Value;
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

void URealGuitarMenuWidget::HandleMouseSensChanged(float Value)
{
    DraftSettings.MouseSensitivity = Value;
    if (MouseSensValueText) MouseSensValueText->SetText(FText::FromString(FString::Printf(TEXT("%.2fx"), Value)));
    PushDraftSettings(true);
}

void URealGuitarMenuWidget::HandleFovChanged(float Value)
{
    DraftSettings.FieldOfView = Value;
    if (FovValueText) FovValueText->SetText(FText::FromString(FString::Printf(TEXT("%.0f°"), Value)));
    PushDraftSettings(true);
}

void URealGuitarMenuWidget::HandleInvertYToggled(bool bChecked)
{
    if (bSyncingUI) return;
    DraftSettings.bInvertY = bChecked;
    PushDraftSettings(true);
}
