#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "Layout/Margin.h"
#include "Styling/SlateBrush.h"

class UBorder;
class UButton;
class UCheckBox;
class UHorizontalBox;
class UImage;
class UPanelWidget;
class UProgressBar;
class USlider;
class UTextBlock;
class UTexture2D;
class UWidget;
class UWidgetTree;

namespace GuitarUIStyle
{
    inline const FLinearColor BackdropScrim(0.015f, 0.018f, 0.028f, 0.56f);
    inline const FLinearColor TitleScrim(0.015f, 0.018f, 0.028f, 0.20f);
    inline const FLinearColor GlassPanel(0.055f, 0.062f, 0.088f, 0.90f);
    inline const FLinearColor CardSurface(0.095f, 0.105f, 0.145f, 0.94f);
    inline const FLinearColor ButtonSecondary(0.145f, 0.155f, 0.210f, 0.95f);
    inline const FLinearColor ButtonHover(0.24f, 0.22f, 0.20f, 0.98f);
    inline const FLinearColor AccentGold(0.92f, 0.69f, 0.41f, 1.0f);
    inline const FLinearColor AccentGoldDim(0.62f, 0.43f, 0.22f, 0.96f);
    inline const FLinearColor AccentTeal(0.32f, 0.72f, 0.68f, 1.0f);
    inline const FLinearColor TextPrimary(0.96f, 0.94f, 0.90f, 1.0f);
    inline const FLinearColor TextMuted(0.70f, 0.69f, 0.68f, 1.0f);

    UTexture2D* LoadUITexture(const TCHAR* AssetPath);
    FSlateBrush MakeGlassBoxBrush(const FLinearColor& Tint);

    UTextBlock* MakeText(
        UWidgetTree* Tree,
        const FText& Text,
        int32 FontSize,
        FLinearColor Color = TextPrimary,
        bool bBold = false
    );

    UImage* MakeIconImage(
        UWidgetTree* Tree,
        const TCHAR* TexturePath,
        float SizePx = 18.0f,
        FLinearColor Tint = TextPrimary
    );

    UBorder* MakeGlassCard(
        UWidgetTree* Tree,
        UWidget* ChildContent,
        FMargin Padding = FMargin(16.0f, 12.0f),
        FLinearColor Color = GlassPanel
    );

    UButton* MakeButton(
        UWidgetTree* Tree,
        UPanelWidget* ParentBox,
        const FText& LabelText,
        bool bPrimaryAccent = false,
        int32 FontSize = 14,
        FMargin ContentPadding = FMargin(14.0f, 7.0f)
    );

    UButton* MakeIconButton(
        UWidgetTree* Tree,
        UPanelWidget* ParentBox,
        const TCHAR* IconTexturePath,
        const FText& OptionalLabel,
        bool bPrimaryAccent = false,
        float IconSizePx = 18.0f,
        int32 FontSize = 13,
        FMargin ContentPadding = FMargin(11.0f, 6.0f)
    );

    UButton* MakeSimpleButton(
        UWidgetTree* Tree,
        const FText& LabelText,
        bool bPrimaryAccent = false,
        int32 FontSize = 14,
        FMargin ContentPadding = FMargin(14.0f, 7.0f)
    );

    UProgressBar* MakeProgressPill(
        UWidgetTree* Tree,
        FLinearColor FillColor = AccentGold
    );

    USlider* MakeStyledSlider(
        UWidgetTree* Tree,
        float MinVal,
        float MaxVal,
        float StepSize = 0.01f
    );

    UHorizontalBox* AddSettingSliderRow(
        UWidgetTree* Tree,
        UPanelWidget* ParentBox,
        const FText& TitleText,
        USlider*& OutSlider,
        UTextBlock*& OutValueText,
        float MinVal,
        float MaxVal,
        float StepSize = 0.01f
    );

    UHorizontalBox* AddStepperRow(
        UWidgetTree* Tree,
        UPanelWidget* ParentBox,
        const FText& TitleText,
        UButton*& OutPrevBtn,
        UTextBlock*& OutValueText,
        UButton*& OutNextBtn
    );

    UHorizontalBox* AddCheckBoxRow(
        UWidgetTree* Tree,
        UPanelWidget* ParentBox,
        const FText& TitleText,
        UCheckBox*& OutCheckBox
    );

    void AddSpacer(UWidgetTree* Tree, UPanelWidget* ParentBox, float Width, float Height);
}
