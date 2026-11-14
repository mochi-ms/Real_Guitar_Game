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
    TObjectPtr<UTextBlock>& OutLabelText,
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

    UTextBlock* LabelTB = MakeText(Tree, Label, FontSize, TextPrimary, bPrimaryAccent);
    LabelTB->SetJustification(ETextJustify::Center);
    OutLabelText = LabelTB;
    InnerPad->SetContent(LabelTB);

    Button->SetContent(InnerPad);
    return Button;
}

UButton* GuitarUIStyle::MakeSimpleButton(
    UWidgetTree* Tree,
    const FText& Label,
    bool bPrimaryAccent,
    int32 FontSize,
    FMargin ContentPadding
)
{
    TObjectPtr<UTextBlock> UnusedLabel;
    return MakeButton(Tree, Label, UnusedLabel, bPrimaryAccent, FontSize, ContentPadding);
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
    TObjectPtr<USlider>& OutSlider,
    TObjectPtr<UTextBlock>& OutValueText
)
{
    UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

    UTextBlock* LabelTB = MakeText(Tree, Label, 14, TextPrimary);
    if (UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(LabelTB))
    {
        LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        LabelSlot->SetVerticalAlignment(VAlign_Center);
    }

    USlider* Slider = MakeStyledSlider(Tree, MinVal, MaxVal, StepSize);
    OutSlider = Slider;
    if (UHorizontalBoxSlot* SliderSlot = Row->AddChildToHorizontalBox(Slider))
    {
        FSlateChildSize Fill(ESlateSizeRule::Fill);
        Fill.Value = 1.25f;
        SliderSlot->SetSize(Fill);
        SliderSlot->SetVerticalAlignment(VAlign_Center);
        SliderSlot->SetPadding(FMargin(12.0f, 0.0f));
    }

    UTextBlock* ValueTB = MakeText(Tree, FText::FromString(TEXT("0%")), 13, AccentGold, true);
    ValueTB->SetMinDesiredWidth(64.0f);
    ValueTB->SetJustification(ETextJustify::Right);
    OutValueText = ValueTB;
    if (UHorizontalBoxSlot* ValSlot = Row->AddChildToHorizontalBox(ValueTB))
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
    TObjectPtr<UButton>& OutPrevButton,
    TObjectPtr<UTextBlock>& OutValueText,
    TObjectPtr<UButton>& OutNextButton
)
{
    UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

    UTextBlock* LabelTB = MakeText(Tree, Label, 14, TextPrimary);
    if (UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(LabelTB))
    {
        LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        LabelSlot->SetVerticalAlignment(VAlign_Center);
    }

    UButton* PrevBtn = MakeSimpleButton(Tree, FText::FromString(TEXT("<")), false, 13, FMargin(10.0f, 4.0f));
    OutPrevButton = PrevBtn;
    Row->AddChildToHorizontalBox(PrevBtn);

    UTextBlock* ValueTB = MakeText(Tree, FText::FromString(TEXT("-")), 14, AccentGold, true);
    ValueTB->SetMinDesiredWidth(210.0f);
    ValueTB->SetJustification(ETextJustify::Center);
    OutValueText = ValueTB;
    if (UHorizontalBoxSlot* ValSlot = Row->AddChildToHorizontalBox(ValueTB))
    {
        ValSlot->SetVerticalAlignment(VAlign_Center);
        ValSlot->SetPadding(FMargin(8.0f, 0.0f));
    }

    UButton* NextBtn = MakeSimpleButton(Tree, FText::FromString(TEXT(">")), false, 13, FMargin(10.0f, 4.0f));
    OutNextButton = NextBtn;
    Row->AddChildToHorizontalBox(NextBtn);

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
    TObjectPtr<UCheckBox>& OutCheckBox
)
{
    UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

    UTextBlock* LabelTB = MakeText(Tree, Label, 14, TextPrimary);
    if (UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(LabelTB))
    {
        LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        LabelSlot->SetVerticalAlignment(VAlign_Center);
    }

    UCheckBox* Check = Tree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass());
    OutCheckBox = Check;
    if (UHorizontalBoxSlot* CheckSlot = Row->AddChildToHorizontalBox(Check))
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
