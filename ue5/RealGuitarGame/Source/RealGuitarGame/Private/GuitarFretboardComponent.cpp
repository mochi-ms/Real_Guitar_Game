#include "GuitarFretboardComponent.h"

#include "Engine/World.h"
#include "GuitarScoreSubsystem.h"

UGuitarFretboardComponent::UGuitarFretboardComponent()
{
    PrimaryComponentTick.bCanEverTick = true;

    SmoothedIKTargets.LeftFingerWorldTargets.Init(FVector::ZeroVector, 4);
    SmoothedIKTargets.LeftFingerActive.Init(false, 4);
}

static float ComputeFretWireDistance(float ScaleLen, int32 Fret)
{
    if (Fret <= 0)
    {
        return 0.0f;
    }
    return ScaleLen * (1.0f - FMath::Pow(2.0f, -static_cast<float>(Fret) / 12.0f));
}

FVector UGuitarFretboardComponent::GetFretWorldPosition(int32 StringIndex, int32 Fret) const
{
    const int32 ClampedStr = FMath::Clamp(StringIndex, 1, 6);
    const int32 ClampedFret = FMath::Clamp(Fret, 0, 24);

    float X = 0.0f;
    if (ClampedFret > 0)
    {
        const float PrevWire = ComputeFretWireDistance(ScaleLengthCm, ClampedFret - 1);
        const float CurrWire = ComputeFretWireDistance(ScaleLengthCm, ClampedFret);
        X = FMath::Lerp(PrevWire, CurrWire, 0.62f);
    }

    const float AlphaLength = FMath::Clamp(X / FMath::Max(1.0f, ScaleLengthCm), 0.0f, 1.0f);
    const float SpanAtX = FMath::Lerp(NutSpanCm, BridgeSpanCm, AlphaLength);
    // String 1 -> -0.5 * SpanAtX, String 6 -> +0.5 * SpanAtX
    const float StringNorm = (static_cast<float>(ClampedStr - 1) / 5.0f) - 0.5f;
    const float Y = StringNorm * SpanAtX;

    return GetComponentTransform().TransformPosition(FVector(X, Y, 0.0f));
}

FVector UGuitarFretboardComponent::GetSoundholeStringWorldPosition(int32 StringIndex) const
{
    const int32 ClampedStr = FMath::Clamp(StringIndex, 1, 6);
    const float AlphaLength = FMath::Clamp(SoundholePickXCm / FMath::Max(1.0f, ScaleLengthCm), 0.0f, 1.0f);
    const float SpanAtX = FMath::Lerp(NutSpanCm, BridgeSpanCm, AlphaLength);
    const float StringNorm = (static_cast<float>(ClampedStr - 1) / 5.0f) - 0.5f;
    const float Y = StringNorm * SpanAtX;

    return GetComponentTransform().TransformPosition(FVector(SoundholePickXCm, Y, 1.2f));
}

void UGuitarFretboardComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction
)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    const UWorld* World = GetWorld();
    const UGuitarScoreSubsystem* ScoreSub = World ? World->GetSubsystem<UGuitarScoreSubsystem>() : nullptr;
    if (!ScoreSub)
    {
        return;
    }

    TArray<FGuitarEvent> ActiveEvents;
    ScoreSub->GetActiveEvents(ActiveEvents);

    TArray<FVector> DesiredTargets;
    DesiredTargets.Init(FVector::ZeroVector, 4);
    TArray<bool> DesiredActive;
    DesiredActive.Init(false, 4);

    // Pre-seed with current chord snapshot if present so fingers hover cleanly on chord shapes
    FGuitarChordSnapshot ActiveChord;
    if (ScoreSub->GetCurrentChord(ActiveChord) && ActiveChord.Frets.Num() == 6 && ActiveChord.Fingers.Num() == 6)
    {
        for (int32 SIdx = 0; SIdx < 6; ++SIdx)
        {
            const int32 FretVal = ActiveChord.Frets[SIdx];
            const int32 FingerVal = ActiveChord.Fingers[SIdx];
            if (FretVal > 0 && FingerVal >= 1 && FingerVal <= 4)
            {
                const int32 ArrIdx = FingerVal - 1;
                DesiredTargets[ArrIdx] = GetFretWorldPosition(SIdx + 1, FretVal);
                DesiredActive[ArrIdx] = true;
            }
        }
    }

    FVector DesiredPickTarget = GetSoundholeStringWorldPosition(3);
    bool bStrum = false;

    for (const FGuitarEvent& Ev : ActiveEvents)
    {
        if (Ev.Fret > 0 && Ev.LeftFinger >= 1 && Ev.LeftFinger <= 4)
        {
            const int32 ArrIdx = Ev.LeftFinger - 1;
            DesiredTargets[ArrIdx] = GetFretWorldPosition(Ev.StringIndex, Ev.Fret);
            DesiredActive[ArrIdx] = true;
        }
        DesiredPickTarget = GetSoundholeStringWorldPosition(Ev.StringIndex);
        if (Ev.RightAction == EGuitarRightAction::StrumDown || Ev.RightAction == EGuitarRightAction::StrumUp)
        {
            bStrum = true;
        }
    }

    FVector PalmAccum = FVector::ZeroVector;
    int32 PalmCount = 0;
    for (int32 i = 0; i < 4; ++i)
    {
        SmoothedIKTargets.LeftFingerActive[i] = DesiredActive[i];
        const FVector Target = DesiredActive[i] ? DesiredTargets[i] : GetFretWorldPosition(3, 2) + GetUpVector() * 2.5f;
        if (SmoothedIKTargets.LeftFingerWorldTargets[i].IsNearlyZero())
        {
            SmoothedIKTargets.LeftFingerWorldTargets[i] = Target;
        }
        else
        {
            SmoothedIKTargets.LeftFingerWorldTargets[i] = FMath::VInterpTo(
                SmoothedIKTargets.LeftFingerWorldTargets[i],
                Target,
                DeltaTime,
                FingerInterpSpeed
            );
        }
        if (DesiredActive[i])
        {
            PalmAccum += SmoothedIKTargets.LeftFingerWorldTargets[i];
            ++PalmCount;
        }
    }

    const FVector PalmGoal = (PalmCount > 0)
        ? (PalmAccum / static_cast<float>(PalmCount)) - GetUpVector() * 3.5f
        : GetFretWorldPosition(3, 2) - GetUpVector() * 3.5f;

    SmoothedIKTargets.LeftPalmAnchorWorld = FMath::VInterpTo(
        SmoothedIKTargets.LeftPalmAnchorWorld.IsNearlyZero() ? PalmGoal : SmoothedIKTargets.LeftPalmAnchorWorld,
        PalmGoal,
        DeltaTime,
        FingerInterpSpeed * 0.7f
    );

    SmoothedIKTargets.RightHandPickTargetWorld = FMath::VInterpTo(
        SmoothedIKTargets.RightHandPickTargetWorld.IsNearlyZero() ? DesiredPickTarget : SmoothedIKTargets.RightHandPickTargetWorld,
        DesiredPickTarget,
        DeltaTime,
        FingerInterpSpeed * 1.4f
    );
    SmoothedIKTargets.bIsStrumming = bStrum;
}
