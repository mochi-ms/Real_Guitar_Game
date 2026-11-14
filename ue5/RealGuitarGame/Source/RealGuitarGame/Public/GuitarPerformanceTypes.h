#pragma once

#include "CoreMinimal.h"
#include "GuitarPerformanceTypes.generated.h"

UENUM(BlueprintType)
enum class EGuitarRightAction : uint8
{
    PluckThumb UMETA(DisplayName = "Pluck Thumb (P)"),
    PluckIndex UMETA(DisplayName = "Pluck Index (i)"),
    PluckMiddle UMETA(DisplayName = "Pluck Middle (m)"),
    PluckRing UMETA(DisplayName = "Pluck Ring (a)"),
    PickDown UMETA(DisplayName = "Pick Down"),
    PickUp UMETA(DisplayName = "Pick Up"),
    StrumDown UMETA(DisplayName = "Strum Down"),
    StrumUp UMETA(DisplayName = "Strum Up"),
    PercussiveSlap UMETA(DisplayName = "Percussive Slap")
};

UENUM(BlueprintType)
enum class EGuitarArticulation : uint8
{
    Normal UMETA(DisplayName = "Normal"),
    LetRing UMETA(DisplayName = "Let Ring"),
    PalmMute UMETA(DisplayName = "Palm Mute"),
    DeadNote UMETA(DisplayName = "Dead Note"),
    HammerOn UMETA(DisplayName = "Hammer On"),
    PullOff UMETA(DisplayName = "Pull Off"),
    Slide UMETA(DisplayName = "Slide"),
    Harmonic UMETA(DisplayName = "Harmonic"),
    Vibrato UMETA(DisplayName = "Vibrato")
};

USTRUCT(BlueprintType)
struct REALGUITARGAME_API FGuitarEvent
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Guitar|Score")
    int32 Id = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Guitar|Score")
    float TimeSec = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Guitar|Score")
    float DurationSec = 0.25f;

    UPROPERTY(BlueprintReadOnly, Category = "Guitar|Score")
    float Beat = 0.0f;

    // 1 = High E4 .. 6 = Low E2
    UPROPERTY(BlueprintReadOnly, Category = "Guitar|Score")
    int32 StringIndex = 1;

    UPROPERTY(BlueprintReadOnly, Category = "Guitar|Score")
    int32 Fret = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Guitar|Score")
    int32 MidiNote = 64;

    UPROPERTY(BlueprintReadOnly, Category = "Guitar|Score")
    float Velocity = 0.8f;

    // 0 = Open, 1..4 = Index..Pinky, 5 = Thumb
    UPROPERTY(BlueprintReadOnly, Category = "Guitar|Score")
    int32 LeftFinger = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Guitar|Score")
    EGuitarRightAction RightAction = EGuitarRightAction::PluckIndex;

    UPROPERTY(BlueprintReadOnly, Category = "Guitar|Score")
    EGuitarArticulation Articulation = EGuitarArticulation::Normal;

    UPROPERTY(BlueprintReadOnly, Category = "Guitar|Score")
    int32 StrumGroupId = -1;

    UPROPERTY(BlueprintReadOnly, Category = "Guitar|Score")
    FString ChordSymbol;
};

USTRUCT(BlueprintType)
struct REALGUITARGAME_API FGuitarChordSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Guitar|Score")
    float TimeSec = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Guitar|Score")
    float DurationSec = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Guitar|Score")
    FString Symbol;

    // Length 6 (String 1..6), -1 = muted, 0 = open
    UPROPERTY(BlueprintReadOnly, Category = "Guitar|Score")
    TArray<int32> Frets;

    // Length 6 (String 1..6), 0 = none, 1..4 = index..pinky
    UPROPERTY(BlueprintReadOnly, Category = "Guitar|Score")
    TArray<int32> Fingers;
};

USTRUCT(BlueprintType)
struct REALGUITARGAME_API FGuitarPerformanceData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Guitar|Score")
    FString Title;

    UPROPERTY(BlueprintReadOnly, Category = "Guitar|Score")
    FString Artist;

    UPROPERTY(BlueprintReadOnly, Category = "Guitar|Score")
    FString SourceFormat;

    UPROPERTY(BlueprintReadOnly, Category = "Guitar|Score")
    FString PlayStyle;

    UPROPERTY(BlueprintReadOnly, Category = "Guitar|Score")
    float Bpm = 90.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Guitar|Score")
    int32 Capo = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Guitar|Score")
    float DurationSec = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Guitar|Score")
    TArray<int32> TuningMidi;

    UPROPERTY(BlueprintReadOnly, Category = "Guitar|Score")
    TArray<FGuitarChordSnapshot> Chords;

    UPROPERTY(BlueprintReadOnly, Category = "Guitar|Score")
    TArray<FGuitarEvent> Events;
};

USTRUCT(BlueprintType)
struct REALGUITARGAME_API FGuitarHandIKTargets
{
    GENERATED_BODY()

    // Left-hand finger world targets indexed 0..3 -> Index, Middle, Ring, Pinky
    UPROPERTY(BlueprintReadOnly, Category = "Guitar|IK")
    TArray<FVector> LeftFingerWorldTargets;

    UPROPERTY(BlueprintReadOnly, Category = "Guitar|IK")
    TArray<bool> LeftFingerActive;

    UPROPERTY(BlueprintReadOnly, Category = "Guitar|IK")
    FVector LeftPalmAnchorWorld = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "Guitar|IK")
    FVector RightHandPickTargetWorld = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "Guitar|IK")
    bool bIsStrumming = false;
};

UENUM(BlueprintType)
enum class ESanctuaryMood : uint8
{
    GoldenSunset UMETA(DisplayName = "Golden Sunset"),
    CozyNightLamp UMETA(DisplayName = "Cozy Night Lamp"),
    MorningMist UMETA(DisplayName = "Morning Mist"),
    RainyLoFi UMETA(DisplayName = "Rainy Lo-Fi")
};
