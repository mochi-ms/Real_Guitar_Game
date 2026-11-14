#include "GuitarUIStyle.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Styling/SlateTypes.h"

namespace GuitarUIStyle
{
    UTexture2D* LoadUITexture(const TCHAR* AssetPath)
    {
        if (!AssetPath)
        {
            return nullptr;
        }
        return LoadObject<UTexture2D>(nullptr, AssetPath);
    }

    FSlateBrush MakeGlassBoxBrush(const FLinearColor& Tint)
    {
        FSlateBrush Brush;
        if (UTexture2D* GlassTex = LoadUITexture(TEXT("/Game/UI/Textures/T_UI_GlassPanel_9Slice.T_UI_GlassPanel_9Slice")))
        {
            Brush.SetResourceObject(GlassTex);
            Brush.DrawAs = ESlateBrushDrawType::Box;
            Brush.Margin = FMargin(0.32f);
            Brush.ImageSize = FVector2D(64.0f, 64.0f);
            Brush.TintColor = FSlateColor(Tint);
        }
        else
        {
            Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
            Brush.TintColor = FSlateColor(Tint);
            Brush.OutlineSettings = FSlateBrushOutlineSettings(FVector4(10.0f, 10.0f, 10.0f, 10.0f));
        }
        return Brush;
    }

    static void ApplyGlassButtonStyle(UButton* Button, bool bPrimaryAccent)
    {
        if (!Button)
        {
            return;
        }
        const FLinearColor NormalTint = bPrimaryAccent ? AccentGoldDim : ButtonSecondary;
        const FLinearColor HoverTint = bPrimaryAccent
            ? FLinearColor(0.88f, 0.64f, 0.36f, 0.98f)
            : ButtonHover;
        const FLinearColor PressTint = bPrimaryAccent
            ? FLinearColor(0.50f, 0.34f, 0.18f, 1.0f)
            : FLinearColor(0.08f, 0.09f, 0.12f, 1.0f);

        FButtonStyle Style = Button->GetStyle();
        Style.SetNormal(MakeGlassBoxBrush(NormalTint));
        Style.SetHovered(MakeGlassBoxBrush(HoverTint));
        Style.SetPressed(MakeGlassBoxBrush(PressTint));
        Style.SetNormalPadding(FMargin(0.0f));
        Style.SetPressedPadding(FMargin(0.0f, 1.0f, 0.0f, -1.0f));
        Button->SetStyle(Style);
        Button->SetBackgroundColor(FLinearColor::White);
    }

    UTextBlock* MakeText(
        UWidgetTree* Tree,
        const FText& Text,
        int32 FontSize,
        FLinearColor Color,
        bool bBold)
    {
        UTextBlock* TB = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        TB->SetText(Text);
        TB->SetColorAndOpacity(FSlateColor(Color));
        FSlateFontInfo FontInfo = TB->GetFont();
        FontInfo.Size = FontSize;
        FontInfo.TypefaceFontName = bBold ? FName(TEXT("Bold")) : FName(TEXT("Regular"));
        TB->SetFont(FontInfo);
        TB->SetShadowOffset(FVector2D(1.0f, 1.0f));
        TB->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.45f));
        return TB;
    }

    UImage* MakeIconImage(
        UWidgetTree* Tree,
        const TCHAR* TexturePath,
        float SizePx,
        FLinearColor Tint)
    {
        UImage* Img = Tree->ConstructWidget<UImage>(UImage::StaticClass());
        if (UTexture2D* Tex = LoadUITexture(TexturePath))
        {
            Img->SetBrushFromTexture(Tex, false);
        }
        Img->SetDesiredSizeOverride(FVector2D(SizePx, SizePx));
        Img->SetColorAndOpacity(Tint);
        return Img;
    }

    UBorder* MakeGlassCard(
        UWidgetTree* Tree,
        UWidget* ChildContent,
        FMargin Padding,
        FLinearColor Color)
    {
        UBorder* Border = Tree->ConstructWidget<UBorder>(UBorder::StaticClass());
        Border->SetPadding(Padding);
        Border->SetBrush(MakeGlassBoxBrush(Color));
        Border->SetBrushColor(FLinearColor::White);
        if (ChildContent)
        {
            Border->SetContent(ChildContent);
        }
        return Border;
    }

    UButton* MakeSimpleButton(
        UWidgetTree* Tree,
        const FText& LabelText,
        bool bPrimaryAccent,
        int32 FontSize,
        FMargin ContentPadding)
    {
        UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass());
        ApplyGlassButtonStyle(Button, bPrimaryAccent);

        UBorder* InnerPad = Tree->ConstructWidget<UBorder>(UBorder::StaticClass());
        InnerPad->SetPadding(ContentPadding);
        InnerPad->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));

        UTextBlock* Label = MakeText(Tree, LabelText, FontSize, TextPrimary, true);
        Label->SetJustification(ETextJustify::Center);
        InnerPad->SetContent(Label);
        Button->AddChild(InnerPad);
        return Button;
    }

    UButton* MakeButton(
        UWidgetTree* Tree,
        UPanelWidget* ParentBox,
        const FText& LabelText,
        bool bPrimaryAccent,
        int32 FontSize,
        FMargin ContentPadding)
    {
        UButton* Button = MakeSimpleButton(Tree, LabelText, bPrimaryAccent, FontSize, ContentPadding);
        if (ParentBox)
        {
            ParentBox->AddChild(Button);
        }
        return Button;
    }

    UButton* MakeIconButton(
        UWidgetTree* Tree,
        UPanelWidget* ParentBox,
        const TCHAR* IconTexturePath,
        const FText& OptionalLabel,
        bool bPrimaryAccent,
        float IconSizePx,
        int32 FontSize,
        FMargin ContentPadding)
    {
        UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass());
        ApplyGlassButtonStyle(Button, bPrimaryAccent);

        UBorder* InnerPad = Tree->ConstructWidget<UBorder>(UBorder::StaticClass());
        InnerPad->SetPadding(ContentPadding);
        InnerPad->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));

        UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
        UImage* Icon = MakeIconImage(Tree, IconTexturePath, IconSizePx, TextPrimary);
        UHorizontalBoxSlot* IconSlot = Row->AddChildToHorizontalBox(Icon);
        IconSlot->SetVerticalAlignment(VAlign_Center);

        if (!OptionalLabel.IsEmpty())
        {
            AddSpacer(Tree, Row, 6.0f, 1.0f);
            UTextBlock* Label = MakeText(Tree, OptionalLabel, FontSize, TextPrimary, true);
            UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(Label);
            LabelSlot->SetVerticalAlignment(VAlign_Center);
        }

        InnerPad->SetContent(Row);
        Button->AddChild(InnerPad);
        if (ParentBox)
        {
            ParentBox->AddChild(Button);
        }
        return Button;
    }

    UProgressBar* MakeProgressPill(UWidgetTree* Tree, FLinearColor FillColor)
    {
        UProgressBar* Bar = Tree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass());
        Bar->SetFillColorAndOpacity(FillColor);
        Bar->SetPercent(0.0f);
        return Bar;
    }

    USlider* MakeStyledSlider(UWidgetTree* Tree, float MinVal, float MaxVal, float StepSize)
    {
        USlider* Slider = Tree->ConstructWidget<USlider>(USlider::StaticClass());
        Slider->SetMinValue(MinVal);
        Slider->SetMaxValue(MaxVal);
        Slider->SetStepSize(StepSize);
        Slider->SetSliderHandleColor(AccentGold);
        Slider->SetSliderBarColor(FLinearColor(0.24f, 0.25f, 0.30f, 0.95f));
        return Slider;
    }

    UHorizontalBox* AddSettingSliderRow(
        UWidgetTree* Tree,
        UPanelWidget* ParentBox,
        const FText& TitleText,
        USlider*& OutSlider,
        UTextBlock*& OutValueText,
        float MinVal,
        float MaxVal,
        float StepSize)
    {
        UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

        UTextBlock* Title = MakeText(Tree, TitleText, 13, TextPrimary, false);
        USizeBox* LabelBox = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        LabelBox->SetWidthOverride(200.0f);
        LabelBox->AddChild(Title);
        UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(LabelBox);
        LabelSlot->SetVerticalAlignment(VAlign_Center);

        OutSlider = MakeStyledSlider(Tree, MinVal, MaxVal, StepSize);
        UHorizontalBoxSlot* SliderSlot = Row->AddChildToHorizontalBox(OutSlider);
        SliderSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        SliderSlot->SetVerticalAlignment(VAlign_Center);
        SliderSlot->SetPadding(FMargin(10.0f, 0.0f));

        OutValueText = MakeText(Tree, FText::FromString(TEXT("0%")), 13, AccentGold, true);
        USizeBox* ValBox = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        ValBox->SetWidthOverride(72.0f);
        OutValueText->SetJustification(ETextJustify::Right);
        ValBox->AddChild(OutValueText);
        UHorizontalBoxSlot* ValSlot = Row->AddChildToHorizontalBox(ValBox);
        ValSlot->SetVerticalAlignment(VAlign_Center);

        if (ParentBox)
        {
            ParentBox->AddChild(Row);
        }
        return Row;
    }

    UHorizontalBox* AddStepperRow(
        UWidgetTree* Tree,
        UPanelWidget* ParentBox,
        const FText& TitleText,
        UButton*& OutPrevBtn,
        UTextBlock*& OutValueText,
        UButton*& OutNextBtn)
    {
        UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

        UTextBlock* Title = MakeText(Tree, TitleText, 13, TextPrimary, false);
        USizeBox* LabelBox = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        LabelBox->SetWidthOverride(200.0f);
        LabelBox->AddChild(Title);
        UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(LabelBox);
        LabelSlot->SetVerticalAlignment(VAlign_Center);

        OutPrevBtn = MakeButton(Tree, Row, FText::FromString(TEXT("<")), false, 13, FMargin(11.0f, 4.0f));
        OutValueText = MakeText(Tree, FText::FromString(TEXT("-")), 13, AccentGold, true);
        OutValueText->SetJustification(ETextJustify::Center);
        UHorizontalBoxSlot* ValueSlot = Row->AddChildToHorizontalBox(OutValueText);
        ValueSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        ValueSlot->SetVerticalAlignment(VAlign_Center);
        ValueSlot->SetPadding(FMargin(8.0f, 0.0f));

        OutNextBtn = MakeButton(Tree, Row, FText::FromString(TEXT(">")), false, 13, FMargin(11.0f, 4.0f));

        if (ParentBox)
        {
            ParentBox->AddChild(Row);
        }
        return Row;
    }

    UHorizontalBox* AddCheckBoxRow(
        UWidgetTree* Tree,
        UPanelWidget* ParentBox,
        const FText& TitleText,
        UCheckBox*& OutCheckBox)
    {
        UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

        UTextBlock* Title = MakeText(Tree, TitleText, 13, TextPrimary, false);
        UHorizontalBoxSlot* TitleSlot = Row->AddChildToHorizontalBox(Title);
        TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        TitleSlot->SetVerticalAlignment(VAlign_Center);

        OutCheckBox = Tree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass());
        UHorizontalBoxSlot* CheckSlot = Row->AddChildToHorizontalBox(OutCheckBox);
        CheckSlot->SetVerticalAlignment(VAlign_Center);

        if (ParentBox)
        {
            ParentBox->AddChild(Row);
        }
        return Row;
    }

    void AddSpacer(UWidgetTree* Tree, UPanelWidget* ParentBox, float Width, float Height)
    {
        USpacer* Spacer = Tree->ConstructWidget<USpacer>(USpacer::StaticClass());
        Spacer->SetSize(FVector2D(Width, Height));
        if (ParentBox)
        {
            ParentBox->AddChild(Spacer);
        }
    }
}
