#include "GuitarUIStyle.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/PanelWidget.h"
#include "Components/Slider.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

UTextBlock* GuitarUIStyle::MakeText(
    UWidgetTree* Tree,
    const FText& Text,
    int32 FontSize,
    FLinearColor Color,
    bool bBold
)
{
    UTextBlock* TB = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    TB->SetText(Text);
    TB->SetColorAndOpacity(FSlateColor(Color));

    FSlateFontInfo FontInfo = TB->GetFont();
    FontInfo.Size = FontSize;
    if (bBold)
    {
        FontInfo.TypefaceFontName = FName(TEXT("Bold"));
    }
    TB->SetFont(FontInfo);
    return TB;
}

UBorder* GuitarUIStyle::MakeGlassCard(
    UWidgetTree* Tree,
    UWidget* Content,
    FMargin Padding,
    FLinearColor Color
)
{
    UBorder* Border = Tree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Border->SetBrushColor(Color);
    Border->SetPadding(Padding);
    if (Content)
    {
        Border->SetContent(Content);
    }
    return Border;
}

UButton* GuitarUIStyle::MakeButton(
    UWidgetTree* Tree,
    const FText& Label,
    UTextBlock*& OutLabelText,
    bool bPrimaryAccent,
    int32 FontSize,
    FMargin ContentPadding
)
{
    UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass());
    Button->SetBackgroundColor(bPrimaryAccent ? AccentGoldDim : ButtonSecondary);

    UBorder* InnerPad = Tree->ConstructWidget<UBorder>(UBorder::StaticClass());
    InnerPad->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
    InnerPad->SetPadding(ContentPadding);
    InnerPad->SetHorizontalAlignment(HAlign_Center);
    InnerPad->SetVerticalAlignment(VAlign_Center);

    OutLabelText = MakeText(Tree, Label, FontSize, TextPrimary, bPrimaryAccent);
    OutLabelText->SetJustification(ETextJustify::Center);
    InnerPad->SetContent(OutLabelText);

    Button->SetContent(InnerPad);
    return Button;
}

USlider* GuitarUIStyle::MakeStyledSlider(
    UWidgetTree* Tree,
    float MinVal,
    float MaxVal,
    float StepSize
)
{
    USlider* Slider = Tree->ConstructWidget<USlider>(USlider::StaticClass());
    Slider->SetMinValue(MinVal);
    Slider->SetMaxValue(MaxVal);
    Slider->SetStepSize(StepSize);
    Slider->SetSliderBarColor(FLinearColor(0.28f, 0.28f, 0.34f, 1.0f));
    Slider->SetSliderHandleColor(AccentGold);
    return Slider;
}

UHorizontalBox* GuitarUIStyle::AddSettingSliderRow(
    UWidgetTree* Tree,
    UVerticalBox* ParentBox,
    const FText& Label,
    float MinVal,
    float MaxVal,
    float StepSize,
    USlider*& OutSlider,
    UTextBlock*& OutValueText
)
{
    UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

    UTextBlock* LabelTB = MakeText(Tree, Label, 14, TextPrimary);
    if (UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(LabelTB))
    {
        LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        LabelSlot->SetVerticalAlignment(VAlign_Center);
    }

    OutSlider = MakeStyledSlider(Tree, MinVal, MaxVal, StepSize);
    if (UHorizontalBoxSlot* SliderSlot = Row->AddChildToHorizontalBox(OutSlider))
    {
        FSlateChildSize Fill(ESlateSizeRule::Fill);
        Fill.Value = 1.25f;
        SliderSlot->SetSize(Fill);
        SliderSlot->SetVerticalAlignment(VAlign_Center);
        SliderSlot->SetPadding(FMargin(12.0f, 0.0f));
    }

    OutValueText = MakeText(Tree, FText::FromString(TEXT("0%")), 13, AccentGold, true);
    OutValueText->SetMinDesiredWidth(64.0f);
    OutValueText->SetJustification(ETextJustify::Right);
    if (UHorizontalBoxSlot* ValSlot = Row->AddChildToHorizontalBox(OutValueText))
    {
        ValSlot->SetVerticalAlignment(VAlign_Center);
    }

    if (UVerticalBoxSlot* VSlot = ParentBox->AddChildToVerticalBox(Row))
    {
        VSlot->SetPadding(FMargin(0.0f, 5.0f));
    }
    return Row;
}

UHorizontalBox* GuitarUIStyle::AddStepperRow(
    UWidgetTree* Tree,
    UVerticalBox* ParentBox,
    const FText& Label,
    UButton*& OutPrevButton,
    UTextBlock*& OutValueText,
    UButton*& OutNextButton
)
{
    UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

    UTextBlock* LabelTB = MakeText(Tree, Label, 14, TextPrimary);
    if (UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(LabelTB))
    {
        LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        LabelSlot->SetVerticalAlignment(VAlign_Center);
    }

    UTextBlock* UnusedPrevLabel = nullptr;
    OutPrevButton = MakeButton(Tree, FText::FromString(TEXT("<")), UnusedPrevLabel, false, 13, FMargin(10.0f, 4.0f));
    Row->AddChildToHorizontalBox(OutPrevButton);

    OutValueText = MakeText(Tree, FText::FromString(TEXT("-")), 14, AccentGold, true);
    OutValueText->SetMinDesiredWidth(210.0f);
    OutValueText->SetJustification(ETextJustify::Center);
    if (UHorizontalBoxSlot* ValSlot = Row->AddChildToHorizontalBox(OutValueText))
    {
        ValSlot->SetVerticalAlignment(VAlign_Center);
        ValSlot->SetPadding(FMargin(8.0f, 0.0f));
    }

    UTextBlock* UnusedNextLabel = nullptr;
    OutNextButton = MakeButton(Tree, FText::FromString(TEXT(">")), UnusedNextLabel, false, 13, FMargin(10.0f, 4.0f));
    Row->AddChildToHorizontalBox(OutNextButton);

    if (UVerticalBoxSlot* VSlot = ParentBox->AddChildToVerticalBox(Row))
    {
        VSlot->SetPadding(FMargin(0.0f, 5.0f));
    }
    return Row;
}

UHorizontalBox* GuitarUIStyle::AddCheckBoxRow(
    UWidgetTree* Tree,
    UVerticalBox* ParentBox,
    const FText& Label,
    UCheckBox*& OutCheckBox
)
{
    UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

    UTextBlock* LabelTB = MakeText(Tree, Label, 14, TextPrimary);
    if (UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(LabelTB))
    {
        LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        LabelSlot->SetVerticalAlignment(VAlign_Center);
    }

    OutCheckBox = Tree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass());
    if (UHorizontalBoxSlot* CheckSlot = Row->AddChildToHorizontalBox(OutCheckBox))
    {
        CheckSlot->SetVerticalAlignment(VAlign_Center);
        CheckSlot->SetPadding(FMargin(10.0f, 0.0f, 4.0f, 0.0f));
    }

    if (UVerticalBoxSlot* VSlot = ParentBox->AddChildToVerticalBox(Row))
    {
        VSlot->SetPadding(FMargin(0.0f, 5.0f));
    }
    return Row;
}

void GuitarUIStyle::AddSpacer(UWidgetTree* Tree, UPanelWidget* ParentBox, float Width, float Height)
{
    USpacer* Spacer = Tree->ConstructWidget<USpacer>(USpacer::StaticClass());
    Spacer->SetSize(FVector2D(Width, Height));
    ParentBox->AddChild(Spacer);
}
