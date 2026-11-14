#pragma once

#include "CoreMinimal.h"

class UBorder;
class UButton;
class UCheckBox;
class UHorizontalBox;
class UPanelWidget;
class UProgressBar;
class USlider;
class USpacer;
class UTextBlock;
class UVerticalBox;
class UWidget;
class UWidgetTree;

namespace GuitarUIStyle
{
    inline const FLinearColor BackdropScrim(0.02f, 0.02f, 0.03f, 0.76f);
    inline const FLinearColor GlassPanel(0.06f, 0.06f, 0.08f, 0.90f);
    inline const FLinearColor CardSurface(0.11f, 0.11f, 0.14f, 0.94f);
    inline const FLinearColor ButtonSecondary(0.17f, 0.17f, 0.21f, 0.95f);
    inline const FLinearColor AccentGold(0.86f, 0.64f, 0.39f, 1.0f);
    inline const FLinearColor AccentGoldDim(0.58f, 0.41f, 0.23f, 0.95f);
    inline const FLinearColor TextPrimary(0.96f, 0.94f, 0.90f, 1.0f);
    inline const FLinearColor TextMuted(0.68f, 0.67f, 0.65f, 1.0f);

    UTextBlock* MakeText(
        UWidgetTree* Tree,
        const FText& Text,
        int32 FontSize = 14,
        FLinearColor Color = TextPrimary,
        bool bBold = false
    );

    UBorder* MakeGlassCard(
        UWidgetTree* Tree,
        UWidget* Content,
        FMargin Padding = FMargin(16.0f, 12.0f),
        FLinearColor Color = GlassPanel
    );

    UButton* MakeButton(
        UWidgetTree* Tree,
        const FText& Label,
        UTextBlock*& OutLabelText,
        bool bPrimaryAccent = false,
        int32 FontSize = 13,
        FMargin ContentPadding = FMargin(14.0f, 7.0f)
    );

    USlider* MakeStyledSlider(
        UWidgetTree* Tree,
        float MinVal,
        float MaxVal,
        float StepSize
    );

    UHorizontalBox* AddSettingSliderRow(
        UWidgetTree* Tree,
        UVerticalBox* ParentBox,
        const FText& Label,
        float MinVal,
        float MaxVal,
        float StepSize,
        USlider*& OutSlider,
        UTextBlock*& OutValueText
    );

    UHorizontalBox* AddStepperRow(
        UWidgetTree* Tree,
        UVerticalBox* ParentBox,
        const FText& Label,
        UButton*& OutPrevButton,
        UTextBlock*& OutValueText,
        UButton*& OutNextButton
    );

    UHorizontalBox* AddCheckBoxRow(
        UWidgetTree* Tree,
        UVerticalBox* ParentBox,
        const FText& Label,
        UCheckBox*& OutCheckBox
    );

    void AddSpacer(UWidgetTree* Tree, UPanelWidget* ParentBox, float Width, float Height);
}
